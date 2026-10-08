#include "CanvasWidget.hpp"

#include <algorithm>

#include <cmath>
#include <cstdio>
#include "Profiler.hpp"

#include "Event.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

namespace tp::sketch2d {

	using ui::Event;
	using ui::PointerButton;

	namespace {
		constexpr halnf kZoomStep = 1.1f;
		constexpr halnf kStatisticsBarHeight = 22;
		constexpr halnf kLegendRowHeight = 20;
		const RGBA kRopeColor = { 0.9f, 0.2f, 0.2f, 0.85f };
		const RGBA kContourOuter = { 0.0f, 0.0f, 0.0f, 0.5f };
		const RGBA kContourInner = { 1.0f, 1.0f, 1.0f, 0.9f };

		std::string strokeLabel(const Brush& brush) {
			if (brush.erases()) return "Eraser";
			if (brush.smudges()) return "Smudge";
			return "Paint";
		}
	}

	CanvasWidget::CanvasWidget(Document& document, ViewportRenderer& renderer, Brush& brush) :
		mDocument(document),
		mRenderer(renderer),
		mBrush(brush) {
		setDebugName("canvas");
		setSizing(ui::AxisSizing::fill(), ui::AxisSizing::fill());
	}

	void CanvasWidget::onEvent(Event& event) {
		switch (event.type()) {
			case Event::Type::Wheel:
				if (event.wheelAmount() == 0) break;
				changeView([&event](Document& view) {
					view.zoomAt(event.position(), event.wheelAmount() > 0 ? kZoomStep : 1.0f / kZoomStep);
				});
				break;

			case Event::Type::PointerDown:
				trackHover(event);
				if (wantsPan(event) || (mTool == CanvasTool::Hand && event.button() == PointerButton::Primary))
					beginStroke(Stroke::Panning, event.position());
				else if (mTool == CanvasTool::Bucket && event.button() == PointerButton::Primary && !wantsPicker()) {
					fillAt(event.position());
					break;
				} else if (event.button() == PointerButton::Primary && mPerspectiveMoveArmed) {
					const Vec2F world = mDocument.screenToWorld(event.position());
					mPerspectivePoint = 0;
					for (int4 index = 1; index < mSettings.perspective.pointCount(); index++)
						if (
							(mSettings.perspective.points[index] - world).length() <
							(mSettings.perspective.points[mPerspectivePoint] - world).length()
						)
							mPerspectivePoint = index;
					beginStroke(Stroke::MovingPerspective, event.position());
				} else if (event.button() == PointerButton::Primary && mSymmetryMoveArmed) {
					mSymmetryGrab = mDocument.screenToWorld(event.position()) - mSettings.symmetry.center;
					beginStroke(Stroke::MovingSymmetry, event.position());
				} else if (mTool == CanvasTool::Transform && event.button() == PointerButton::Primary && isTransforming()) {
					beginTransformDrag(event.position(), event.modifiers().shift != mSettings.transform.proportional);
					beginStroke(Stroke::Transforming, event.position());
				} else if (mTool == CanvasTool::Wand && event.button() == PointerButton::Primary) {
					const Selection before = mDocument.selection();
					const BucketResult result = mDocument.magicWand(
						mDocument.screenToWorld(event.position()), mSettings.selection.wand, selectionMode(event.modifiers())
					);
					if (!(before == mDocument.selection())) {
						mDocument.commit("Magic wand");
						if (onSelectionChanged) onSelectionChanged();
					}
					if (onWand) onWand(result);
					break;
				} else if (
					(mTool == CanvasTool::Marquee || mTool == CanvasTool::Lasso) && event.button() == PointerButton::Primary
				) {
					mSelectMode = selectionMode(event.modifiers());
					const bool polygon = mTool == CanvasTool::Lasso && mSettings.selection.lasso == LassoShape::Polygon;
					if (polygon) {
						if (mPolygon.size() >= 3 && (event.position() - mPolygon.front()).length() <= kPolygonCloseRadius) {
							finishPolygon();
							break;
						}
						mPolygon.push_back(event.position());
						mPickLocal = event.position();
						requestPaint();
						break;
					}
					mToolOrigin = event.position();
					mPickLocal = event.position();
					mLassoPath = { event.position() };
					beginStroke(Stroke::Selecting, event.position());
				} else if (mTool == CanvasTool::Crop && event.button() == PointerButton::Primary) {
					mToolOrigin = event.position();
					mPickLocal = event.position();
					beginStroke(Stroke::Cropping, event.position());
				} else if (mTool == CanvasTool::Move && event.button() == PointerButton::Primary && !wantsPicker()) {
					const bool duplicate = mSettings.move.duplicate || event.modifiers().alt;
					if (!mDocument.beginMove(duplicate)) {
						if (onMoveFinished) onMoveFinished(false);
						break;
					}
					mToolOrigin = event.position();
					beginStroke(Stroke::Moving, event.position());
				} else if (mTool == CanvasTool::Gradient && event.button() == PointerButton::Primary && !wantsPicker()) {
					mToolOrigin = event.position();
					mPickLocal = event.position();
					beginStroke(Stroke::Gradient, event.position());
				} else if (mTool == CanvasTool::Zoom && event.button() == PointerButton::Primary) {
					mToolOrigin = event.position();
					mToolStartZoom = mDocument.zoom();
					mToolMoved = false;
					mZoomOut = event.modifiers().alt;
					beginStroke(Stroke::Zooming, event.position());
				} else if (mTool == CanvasTool::Rotate && event.button() == PointerButton::Primary) {
					const Vec2F center = size() / 2.0f;
					const Vec2F arm = event.position() - center;
					mToolStartAngle = std::atan2(arm.y, arm.x);
					mToolStartRotation = mDocument.rotation();
					beginStroke(Stroke::Rotating, event.position());
				} else if (wantsPicker() && event.button() == PointerButton::Primary) {
					mPickStart = mBrush.color;
					beginStroke(Stroke::Picking, event.position());
					pickAt(event.position());
				} else if (const auto parameter = pendingAdjustment(); parameter && event.button() == PointerButton::Primary)
					beginAdjusting(*parameter, event.position());
				else if (event.button() == PointerButton::Primary) {
					const bool finger = event.sample().tool == PointerTool::Finger;
					if (finger && (mGestureFingersDown || mSinceGestureMs < kFingerGestureGraceMs)) break;
					if (finger) {
						beginStroke(Stroke::FingerPending, event.position());
						mFingerStart = { event.position(), pressureOf(event) };
						mFingerPath.clear();
						mFingerWaitMs = 0;
						startTicking();
					} else beginPainting(event.position(), pressureOf(event));
				} else return;
				break;

			case Event::Type::PointerMove:
				trackHover(event);
				if (!mPolygon.empty()) mPickLocal = event.position();
				if (mStroke == Stroke::None) {
					requestPaint();
					return;
				}
				if (mStroke == Stroke::FingerPending) {
					mFingerPath.push_back({ event.position(), pressureOf(event) });
					if ((event.position() - mFingerStart.position).length() >= kFingerCommitTravel) commitFingerStroke();
					break;
				}
				continueStroke(event.position(), pressureOf(event));
				break;

			case Event::Type::PointerUp:
				if (mStroke == Stroke::None) return;
				if (mStroke == Stroke::FingerPending) commitFingerStroke();
				endStroke();
				break;

			case Event::Type::Gesture: handleGesture(event); break;

			default: return;
		}

		event.accept();
		requestPaint();
	}

	void CanvasWidget::paint(ui::Painter& painter) {
		TP_PROFILE_SCOPE("canvas.view");
		mDocument.submitPaint();
		TP_PROFILE_COUNT("brush.worker_ms", mDocument.takePaintWorkerMs());
		TP_PROFILE_COUNT("brush.backlog", mDocument.paintBacklog());
		mRenderer.render(mDocument, mSettings, size());
		painter.image(localRect(), mRenderer.image());
		if (isCatchingUp()) startTicking();
	}

	bool CanvasWidget::isCatchingUp() const { return mDocument.paintBacklog() > 0 || mRenderer.isCatchingUp(); }

	void CanvasWidget::paintOverlay(ui::Painter& painter) {
		paintStabilizerRope(painter);
		paintBrushCursor(painter);
		paintAdjustment(painter);
		paintBrushPreview(painter);
		paintPicker(painter);
		paintTransform(painter);
		paintSelection(painter);
		paintSymmetry(painter);
		paintPerspective(painter);
		if (!mRenderer.debugOverlay()) return;

		const RectF bar = ui::makeRect(0, 0, size().x, kStatisticsBarHeight);
		painter.fillRect(bar, RGBA(0, 0, 0, 0.55f));
		painter.text(
			mRenderer.statistics(), bar, theme().metrics.smallFontSize, ui::TextAlign::Start, theme().palette.text
		);
		halnf y = kStatisticsBarHeight + 6.0f;
		for (const ViewportRenderer::LegendEntry& entry : mRenderer.debugLegend()) {
			const RectF row = ui::makeRect(6, y, 190, kLegendRowHeight);
			painter.fillRect(row, RGBA(0, 0, 0, 0.55f));
			painter.fillRect(ui::makeRect(10, y + 3, kLegendRowHeight - 6, kLegendRowHeight - 6), entry.color);
			painter.text(
				entry.label,
				ui::makeRect(10 + kLegendRowHeight, y, 180 - kLegendRowHeight, kLegendRowHeight),
				theme().metrics.smallFontSize,
				ui::TextAlign::Start,
				theme().palette.text
			);
			y += kLegendRowHeight + 2.0f;
		}
	}

	void CanvasWidget::paintStabilizerRope(ui::Painter& painter) const {
		if (!isStabilizerRopeVisible()) return;
		const Vec2F pen = mStabilizer.pen().position;
		const Vec2F line = mStabilizer.smoothed().position;
		painter.line(line, pen, kRopeColor, 1.5f);
		painter.strokeCircle(line, 3, kRopeColor, 1.5f);
	}

	void CanvasWidget::onHoverChanged() {
		if (isHovered() || mStroke != Stroke::None) return;
		mHoverInside = false;
		requestPaint();
	}

	void CanvasWidget::trackHover(const Event& event) {
		mHoverLocal = event.position();
		mHoverInside = ui::contains(localRect(), event.position());
		mHoverStylus = event.sample().isStylus();
		refreshCursor();
	}

	BrushCursorShape CanvasWidget::brushCursor() const {
		if (mStroke == Stroke::Adjusting) {
			BrushCursorSettings contour = mSettings.cursor;
			contour.style = BrushCursorStyle::Contour;
			contour.contourForEraserAndSmudge = true;
			return BrushCursorShape::build(contour, mBrush, mAdjustOrigin, mDocument.zoom(), mDocument.rotation());
		}
		if (!mHoverInside || mStroke == Stroke::Panning || mStroke == Stroke::Picking || wantsPicker()) return {};
		if (mTool != CanvasTool::Paint) return {};
		if (mHoverStylus && !mSettings.cursor.showForPen) return {};
		return BrushCursorShape::build(mSettings.cursor, mBrush, mHoverLocal, mDocument.zoom(), mDocument.rotation());
	}

	CursorShape CanvasWidget::systemCursor() const {
		if (mStroke == Stroke::Panning) return CursorShape::Grabbing;
		if (mTool == CanvasTool::Hand) return CursorShape::Grab;
		if (mTool == CanvasTool::Move || mStroke == Stroke::Moving) return CursorShape::Move;
		if (
			mTool == CanvasTool::Transform || mTool == CanvasTool::Crop || mTool == CanvasTool::Marquee ||
			mTool == CanvasTool::Lasso || mTool == CanvasTool::Wand
		)
			return CursorShape::Crosshair;
		if (mTool == CanvasTool::Bucket || mTool == CanvasTool::Gradient) return CursorShape::Crosshair;
		if (mTool == CanvasTool::Zoom || mStroke == Stroke::Zooming) return CursorShape::ResizeHorizontal;
		if (mTool == CanvasTool::Rotate || mStroke == Stroke::Rotating) return CursorShape::Crosshair;
		if (mStroke == Stroke::Adjusting || pendingAdjustment()) return CursorShape::ResizeHorizontal;
		if (mStroke == Stroke::Picking)
			return mSettings.picker.style == PickerStyle::Cursor ? CursorShape::Crosshair : CursorShape::Hidden;
		if (wantsPicker()) return CursorShape::Crosshair;
		if (mHoverStylus && !mSettings.cursor.showForPen) return CursorShape::Hidden;
		switch (mSettings.cursor.style) {
			case BrushCursorStyle::Arrow: return CursorShape::Default;
			case BrushCursorStyle::Cross: return CursorShape::Crosshair;
			case BrushCursorStyle::Brush: return CursorShape::Pointer;
			case BrushCursorStyle::Contour: break;
		}
		return brushCursor().contour ? CursorShape::Hidden : CursorShape::Crosshair;
	}

	void CanvasWidget::refreshCursor() { setCursor(systemCursor()); }

	void CanvasWidget::paintBrushCursor(ui::Painter& painter) const {
		const BrushCursorShape shape = brushCursor();
		if (shape.spotlight && shape.contour) {
			const RGBA color = mSettings.cursor.spotlightColor.rgba(mSettings.cursor.spotlightAlpha);
			painter.fillPolygon(shape.outline, color);
		}
		if (shape.contour && shape.outline.size() > 1) {
			painter.polyline(shape.outline, kContourOuter, 3.0f);
			painter.polyline(shape.outline, kContourInner, 1.0f);
		}
		if (shape.crosshair && shape.contour) {
			const halnf arm = BrushCursorShape::kCrossArm;
			const Vec2F c = shape.center;
			for (const auto& [color, width] : { std::pair{ kContourOuter, 3.0f }, std::pair{ kContourInner, 1.0f } }) {
				painter.line({ c.x - arm, c.y }, { c.x + arm, c.y }, color, width);
				painter.line({ c.x, c.y - arm }, { c.x, c.y + arm }, color, width);
			}
		}
	}

	std::optional<BrushParameter> CanvasWidget::pendingAdjustment() const {
		if (heldAdjustment)
			if (const auto held = heldAdjustment()) return held;
		return mAdjustTool;
	}

	halnf CanvasWidget::adjustedValue(BrushParameter parameter, halnf start, halnf dragPixels) {
		switch (parameter) {
			case BrushParameter::Size: return std::round(start * std::exp2(dragPixels / kSizeDoublingPixels));
			case BrushParameter::Angle:
				{
					const halnf angle = std::fmod(start + dragPixels * kAngleDegreesPerPixel, 360.0f);
					return angle < 0 ? angle + 360.0f : angle;
				}
			default: return start + dragPixels * kAmountPerPixel;
		}
	}

	void CanvasWidget::beginAdjusting(BrushParameter parameter, Vec2F local) {
		mAdjusting = parameter;
		mAdjustOrigin = local;
		mAdjustStart = mBrush.get(parameter);
		beginStroke(Stroke::Adjusting, local);
		refreshCursor();
	}

	std::string CanvasWidget::adjustmentText() const {
		const halnf value = mBrush.get(mAdjusting);
		char text[64];
		switch (mAdjusting) {
			case BrushParameter::Size: std::snprintf(text, sizeof(text), "Size %d px", (int) std::round(value)); break;
			case BrushParameter::Angle: std::snprintf(text, sizeof(text), "Angle %d°", (int) std::round(value)); break;
			case BrushParameter::Flow: std::snprintf(text, sizeof(text), "Flow %d%%", (int) std::round(value * 100)); break;
			default: std::snprintf(text, sizeof(text), "Opacity %d%%", (int) std::round(value * 100)); break;
		}
		return text;
	}

	void CanvasWidget::paintAdjustment(ui::Painter& painter) const {
		if (mStroke != Stroke::Adjusting) return;
		const halnf fontSize = theme().metrics.smallFontSize;
		const std::string text = adjustmentText();
		const halnf width = painter.textWidth(text, fontSize) + 16;
		const RectF box = ui::makeRect(mAdjustOrigin.x - width * 0.5f, mAdjustOrigin.y - 40, width, fontSize + 10);
		painter.fillRect(box, RGBA(0, 0, 0, 0.65f), 4);
		painter.text(text, box, fontSize, ui::TextAlign::Center, theme().palette.text);
	}

	void CanvasWidget::setBrushPreview(std::optional<BrushParameter> parameter) {
		mBrushPreview = parameter;
		requestPaint();
	}

	BrushCursorShape CanvasWidget::brushPreviewShape() const {
		if (!mBrushPreview) return {};
		BrushCursorSettings contour = mSettings.cursor;
		contour.style = BrushCursorStyle::Contour;
		contour.contourForEraserAndSmudge = true;
		contour.spotlight = false;
		return BrushCursorShape::build(contour, mBrush, localRect().center(), mDocument.zoom(), mDocument.rotation());
	}

	RectF CanvasWidget::brushPreviewBackdrop() const {
		const BrushCursorShape shape = brushPreviewShape();
		const halnf padding = theme().metrics.controlHeight;
		const halnf half = std::min(shape.radius + padding, std::min(size().x, size().y) * 0.5f);
		const halnf textRoom = theme().metrics.lineHeight + 8;
		return ui::makeRect(shape.center.x - half, shape.center.y - half, half * 2, half * 2 + textRoom);
	}

	std::string CanvasWidget::brushPreviewText() const {
		if (!mBrushPreview) return {};
		if (*mBrushPreview == BrushParameter::Opacity)
			return std::to_string((int) std::round(mBrush.opacity() * 100)) + "%";
		return std::to_string((int) std::round(mBrush.size())) + " px";
	}

	void CanvasWidget::paintBrushPreview(ui::Painter& painter) const {
		if (!mBrushPreview) return;
		const BrushCursorShape shape = brushPreviewShape();
		const RectF backdrop = brushPreviewBackdrop();
		painter.fillRect(backdrop, RGBA(0.08f, 0.08f, 0.09f, 0.82f), std::min(backdrop.size.x * 0.5f, 24.0f));
		const RGBA color = { mBrush.color.r, mBrush.color.g, mBrush.color.b, mBrush.erases() ? 0.0f : mBrush.opacity() };
		if (shape.outline.size() > 2 && color.a > 0) painter.fillPolygon(shape.outline, color);
		if (shape.outline.size() > 1) {
			painter.polyline(shape.outline, kContourOuter, 3.0f);
			painter.polyline(shape.outline, kContourInner, 1.0f);
		}
		const halnf fontSize = theme().metrics.fontSize;
		const RectF label =
			ui::makeRect(backdrop.pos.x, backdrop.pos.y + backdrop.size.y - fontSize - 12, backdrop.size.x, fontSize + 8);
		painter.text(brushPreviewText(), label, fontSize, ui::TextAlign::Center, RGBA(1, 1, 1, 1));
	}

	bool CanvasWidget::isStabilizerRopeVisible() const {
		return mStroke == Stroke::Painting && !mLineLocked && mStabilizer.isSmoothing() && !mStabilizer.isSettled();
	}

	bool CanvasWidget::wantsPan(const Event& event) const {
		const bool middle = event.button() == PointerButton::Middle;
		const bool spaceDrag = event.button() == PointerButton::Primary && ui() && ui()->isKeyDown(InputID::SPACE);
		return middle || spaceDrag;
	}

	halnf CanvasWidget::pressureOf(const Event& event) { return event.sample().isStylus() ? event.pressure() : 1.0f; }

	Vec2F SymmetrySettings::mirror(Vec2F point) const {
		const Vec2F direction = { std::cos(angle), std::sin(angle) };
		const Vec2F offset = point - center;
		const halnf along = offset.x * direction.x + offset.y * direction.y;
		const Vec2F foot = center + direction * along;
		return foot * 2.0f - point;
	}

	int4 PerspectiveSettings::pointCount() const {
		switch (type) {
			case PerspectiveType::OnePoint:
			case PerspectiveType::Radial:
			case PerspectiveType::Isometric: return 1;
			case PerspectiveType::TwoPoint: return 2;
			case PerspectiveType::ThreePoint: return 3;
		}
		return 1;
	}

	std::vector<Vec2F> PerspectiveSettings::directions(Vec2F from) const {
		std::vector<Vec2F> result;
		const auto toward = [&result, from](Vec2F point) {
			const Vec2F direction = point - from;
			const halnf length = direction.length();
			if (length > 1e-3f) result.push_back(direction / length);
		};
		switch (type) {
			case PerspectiveType::OnePoint:
				toward(points[0]);
				result.push_back({ 1, 0 });
				result.push_back({ 0, 1 });
				break;
			case PerspectiveType::TwoPoint:
				toward(points[0]);
				toward(points[1]);
				result.push_back({ 0, 1 });
				break;
			case PerspectiveType::ThreePoint:
				toward(points[0]);
				toward(points[1]);
				toward(points[2]);
				break;
			case PerspectiveType::Isometric:
				{
					const halnf angle = isometricDegrees * 3.14159265f / 180.0f;
					result.push_back({ std::cos(angle), std::sin(angle) });
					result.push_back({ std::cos(angle), -std::sin(angle) });
					result.push_back({ 0, 1 });
					break;
				}
			case PerspectiveType::Radial: toward(points[0]); break;
		}
		return result;
	}

	Vec2F PerspectiveSettings::snap(Vec2F from, Vec2F to) const {
		const Vec2F drag = to - from;
		const halnf length = drag.length();
		if (length <= 1e-3f) return to;
		Vec2F best = to;
		halnf bestAlignment = -1;
		for (const Vec2F& direction : directions(from)) {
			const halnf along = drag.x * direction.x + drag.y * direction.y;
			const halnf alignment = std::abs(along) / length;
			if (alignment > bestAlignment) {
				bestAlignment = alignment;
				best = from + direction * along;
			}
		}
		return best;
	}

	void CanvasWidget::resetPerspective() {
		PerspectiveSettings& perspective = mSettings.perspective;
		const Vec2F center = mDocument.screenToWorld(size() / 2.0f);
		const halnf reach = size().x / std::max(mDocument.zoom(), 1e-3f) * 0.4f;
		perspective.points[0] = perspective.pointCount() == 1 ? center : center + Vec2F{ -reach, 0.0f };
		perspective.points[1] = center + Vec2F{ reach, 0.0f };
		perspective.points[2] = center + Vec2F{ 0.0f, reach * 1.5f };
		perspective.placed = true;
		requestPaint();
	}

	void CanvasWidget::paintPerspective(ui::Painter& painter) const {
		if (mSettings.line.mode != LineMode::Perspective) return;
		const PerspectiveSettings& perspective = mSettings.perspective;
		const RGBA guide = { 1.0f, 0.55f, 0.2f, 0.35f };
		const RGBA point = { 1.0f, 0.55f, 0.2f, 0.95f };
		const halnf reach = size().length();
		if (perspective.type == PerspectiveType::Isometric) {
			const Vec2F origin = mDocument.worldToScreen(perspective.points[0]);
			for (const Vec2F& direction : perspective.directions(perspective.points[0])) {
				const Vec2F screen = mDocument.worldToScreen(perspective.points[0] + direction) - origin;
				const halnf length = screen.length();
				if (length <= 0) continue;
				for (int4 offset = -6; offset <= 6; offset++) {
					const Vec2F normal = Vec2F{ -screen.y, screen.x } / length * (halnf) offset * 60.0f;
					painter.line(
						origin + normal - screen / length * reach, origin + normal + screen / length * reach, guide, 1.0f
					);
				}
			}
			painter.fillCircle(origin, 5, point);
			return;
		}
		for (int4 index = 0; index < perspective.pointCount(); index++) {
			const Vec2F vanishing = mDocument.worldToScreen(perspective.points[index]);
			for (int4 ray = 0; ray < 24; ray++) {
				const halnf angle = 2.0f * 3.14159265f * (halnf) ray / 24.0f;
				painter.line(vanishing, vanishing + Vec2F{ std::cos(angle), std::sin(angle) } * reach, guide, 1.0f);
			}
			painter.fillCircle(vanishing, 7, kContourOuter);
			painter.fillCircle(vanishing, 5, point);
		}
	}

	void CanvasWidget::stamp(Vec2F world, halnf pressure) {
		mDocument.stampBrushDab(world, mBrush, pressure);
		if (mSettings.symmetry.enabled && !mBrush.smudges())
			mDocument.stampBrushDab(mSettings.symmetry.mirror(world), mBrush, pressure);
	}

	void CanvasWidget::resetSymmetry() {
		mSettings.symmetry.center = mDocument.screenToWorld(size() / 2.0f);
		mSettings.symmetry.angle = 1.5707963f - mDocument.rotation();
		requestPaint();
	}

	void CanvasWidget::paintSymmetry(ui::Painter& painter) const {
		const SymmetrySettings& symmetry = mSettings.symmetry;
		if (!symmetry.enabled || !symmetry.visible) return;
		const Vec2F center = mDocument.worldToScreen(symmetry.center);
		Vec2F along =
			mDocument.worldToScreen(symmetry.center + Vec2F{ std::cos(symmetry.angle), std::sin(symmetry.angle) }) - center;
		const halnf length = along.length();
		if (length <= 0) return;
		along = along / length;
		const halnf reach = size().length();
		const RGBA color = symmetry.locked ? RGBA(0.4f, 0.6f, 1.0f, 0.6f) : RGBA(0.4f, 0.6f, 1.0f, 0.9f);
		painter.line(center - along * reach, center + along * reach, kContourOuter, 3.0f);
		painter.line(center - along * reach, center + along * reach, color, 1.5f);
	}

	void CanvasWidget::beginPainting(Vec2F local, halnf pressure) {
		const std::string refusal = mDocument.strokeRefusal(mBrush);
		if (!refusal.empty()) {
			if (onRefused) onRefused(refusal);
			return;
		}

		mDabCarry = 0;
		mStabilizer.begin({ local, pressure }, activeStabilizer());
		mStabilizer.takeOutput();
		mLastPainted = { mDocument.screenToWorld(local), pressure };
		mStrokeStart = mLastPainted;
		mHoldAnchor = local;
		mHoldMs = 0;
		mLineLocked = mSettings.line.mode != LineMode::Normal;
		if (mSettings.line.mode == LineMode::Perspective && !mSettings.perspective.placed) resetPerspective();
		mEllipseStroke = mSettings.line.mode == LineMode::Ellipse;
		if (!mEllipseStroke) mDocument.beginPathStroke(mBrush, strokeMirror(), { mLastPainted.position, pressure });
		beginStroke(Stroke::Painting, local);
		if (mStabilizer.isSmoothing() || mSettings.line.holdForLine) startTicking();
	}

	halnf CanvasWidget::activeStabilizer() const {
		return mSettings.useBrushStabilizer ? mBrush.stabilizer() : mSettings.globalStabilizer;
	}

	void CanvasWidget::beginStroke(Stroke stroke, Vec2F local) {
		mStroke = stroke;
		mLastLocal = local;
		ui()->capturePointer(*this);
	}

	void CanvasWidget::continueStroke(Vec2F local, halnf pressure) {
		TP_PROFILE_COUNT("stroke.input_points", mStroke == Stroke::Painting ? 1 : 0);
		if (mStroke == Stroke::Panning) {
			const Vec2F delta = local - mLastLocal;
			changeView([delta](Document& view) { view.pan(delta); });
		} else if (mStroke == Stroke::Adjusting) {
			mBrush.set(mAdjusting, adjustedValue(mAdjusting, mAdjustStart, local.x - mAdjustOrigin.x));
			if (onBrushAdjusted) onBrushAdjusted();
		} else if (mStroke == Stroke::Picking) {
			pickAt(local);
		} else if (mStroke == Stroke::Gradient || mStroke == Stroke::Cropping) {
			mPickLocal = local;
			requestPaint();
		} else if (mStroke == Stroke::Selecting) {
			mPickLocal = local;
			if (mTool == CanvasTool::Lasso) mLassoPath.push_back(local);
			requestPaint();
		} else if (mStroke == Stroke::Transforming) {
			continueTransformDrag(local);
		} else if (mStroke == Stroke::MovingPerspective) {
			mSettings.perspective.points[mPerspectivePoint] = mDocument.screenToWorld(local);
			requestPaint();
		} else if (mStroke == Stroke::MovingSymmetry) {
			mSettings.symmetry.center = mDocument.screenToWorld(local) - mSymmetryGrab;
			requestPaint();
		} else if (mStroke == Stroke::Moving) {
			const Vec2F from = mDocument.screenToWorld(mToolOrigin);
			const Vec2F to = mDocument.screenToWorld(local);
			mDocument.moveTo((int4) std::lround(to.x - from.x), (int4) std::lround(to.y - from.y));
			requestPaint();
		} else if (mStroke == Stroke::Zooming) {
			const halnf dragged = local.x - mToolOrigin.x;
			if (std::abs(dragged) > kToolClickPixels) mToolMoved = true;
			if (mToolMoved) {
				const halnf target = mToolStartZoom * std::exp2(dragged / kZoomDoublingPixels);
				const Vec2F origin = mToolOrigin;
				changeView([origin, target](Document& view) { view.zoomAt(origin, target / view.zoom()); });
			}
		} else if (mStroke == Stroke::Rotating) {
			const Vec2F center = size() / 2.0f;
			const Vec2F arm = local - center;
			if (arm.length() > kToolClickPixels) {
				halnf rotation = mToolStartRotation + std::atan2(arm.y, arm.x) - mToolStartAngle;
				rotation = std::remainder(rotation, 2.0f * 3.14159265f);
				if (std::abs(rotation) < kRotationSnapRadians) rotation = 0;
				changeView([center, rotation](Document& view) { view.setRotationAt(center, rotation); });
			}
		} else if (mEllipseStroke) {
			redrawEllipse(local, ui() && (ui()->isKeyDown(InputID::LEFT_SHIFT) || ui()->isKeyDown(InputID::RIGHT_SHIFT)));
		} else if (mLineLocked) {
			redrawLine(local);
		} else {
			if ((local - mHoldAnchor).length() > kHoldStillPixels) {
				mHoldAnchor = local;
				mHoldMs = 0;
			}
			mStabilizer.moveTo({ local, pressure });
			paintStabilized();
		}
		mLastLocal = local;
	}

	void CanvasWidget::commitFingerStroke() {
		if (mStroke != Stroke::FingerPending) return;
		mStroke = Stroke::None;
		const std::vector<FingerPoint> path = std::move(mFingerPath);
		mFingerPath.clear();
		beginPainting(mFingerStart.position, mFingerStart.pressure);
		if (mStroke != Stroke::Painting) return;
		for (const FingerPoint& point : path)
			continueStroke(point.position, point.pressure);
	}

	void CanvasWidget::onTick(halnf deltaMs) {
		if (mSinceGestureMs < kFingerGestureGraceMs) mSinceGestureMs += deltaMs;
		if (mStroke == Stroke::FingerPending) {
			mFingerWaitMs += deltaMs;
			if (mFingerWaitMs >= kFingerCommitMs) commitFingerStroke();
		}
		advanceView(deltaMs);
		const bool smoothing = mStroke == Stroke::Painting && mStabilizer.isActive() && !mLineLocked;
		if (smoothing) {
			mStabilizer.advance(deltaMs);
			paintStabilized();
		}
		const bool holding = mStroke == Stroke::Painting && mSettings.line.holdForLine && !mLineLocked;
		if (holding) {
			mHoldMs += deltaMs;
			const Vec2F start = mDocument.worldToScreen(mStrokeStart.position);
			if (mHoldMs >= mSettings.line.holdSeconds * 1000.0f && (mHoldAnchor - start).length() >= kMinimumLinePixels) {
				mLineLocked = true;
				mStabilizer.finish();
				mStabilizer.takeOutput();
				redrawLine(mHoldAnchor);
			}
		}
		const bool catchingUp = isCatchingUp();
		if (catchingUp) requestPaint();
		const bool waiting = mStroke == Stroke::FingerPending || mSinceGestureMs < kFingerGestureGraceMs;
		if (!smoothing && !holding && !catchingUp && !mViewMoving && !waiting) stopTicking();
	}

	void CanvasWidget::redrawLine(Vec2F local) {
		mDocument.restoreStrokeBase();
		mDabCarry = 0;
		Vec2F end = mDocument.screenToWorld(local);
		if (mSettings.line.mode == LineMode::Perspective) end = mSettings.perspective.snap(mStrokeStart.position, end);
		stamp(mStrokeStart.position, mStrokeStart.pressure);
		paintSegment(mStrokeStart.position, end, mStrokeStart.pressure, mStrokeStart.pressure);
		mLastPainted = { end, mStrokeStart.pressure };
		requestPaint();
	}

	void CanvasWidget::redrawEllipse(Vec2F local, bool circle) {
		mDocument.restoreStrokeBase();
		mDabCarry = 0;
		const Vec2F a = mStrokeStart.position;
		Vec2F b = mDocument.screenToWorld(local);
		if (circle) {
			const halnf side = std::max(std::abs(b.x - a.x), std::abs(b.y - a.y));
			b = { a.x + (b.x < a.x ? -side : side), a.y + (b.y < a.y ? -side : side) };
		}
		const Vec2F center = (a + b) / 2.0f;
		const Vec2F radius = { std::abs(b.x - a.x) / 2.0f, std::abs(b.y - a.y) / 2.0f };
		if (radius.x < 0.5f && radius.y < 0.5f) return;
		Vec2F previous = { center.x + radius.x, center.y };
		stamp(previous, mStrokeStart.pressure);
		for (int4 step = 1; step <= kEllipseSegments; step++) {
			const halnf angle = 2.0f * 3.14159265f * (halnf) step / (halnf) kEllipseSegments;
			const Vec2F next = { center.x + std::cos(angle) * radius.x, center.y + std::sin(angle) * radius.y };
			paintSegment(previous, next, mStrokeStart.pressure, mStrokeStart.pressure);
			previous = next;
		}
		mLastPainted = { previous, mStrokeStart.pressure };
		requestPaint();
	}

	void CanvasWidget::paintStabilized() {
		TP_PROFILE_SCOPE("stroke.paint");
		bool painted = false;
		for (const StrokePoint& screen : mStabilizer.takeOutput()) {
			const StrokePoint point = { mDocument.screenToWorld(screen.position), screen.pressure };
			mDocument.addPathSample({ point.position, point.pressure });
			mLastPainted = point;
			painted = true;
		}
		walkPath(false);
		if (painted || mStabilizer.isSmoothing()) requestPaint();
	}

	void CanvasWidget::walkPath(bool finished) { mDocument.advancePathStroke(finished); }

	StrokeMirror CanvasWidget::strokeMirror() const {
		if (!mSettings.symmetry.enabled || mBrush.smudges()) return {};
		const SymmetrySettings symmetry = mSettings.symmetry;
		return [symmetry](Vec2F point) { return symmetry.mirror(point); };
	}

	void CanvasWidget::handleGesture(const Event& event) {
		const TouchGesture& gesture = event.touchGesture();
		if (gesture.phase == GesturePhase::Begin) {
			mGestureFingersDown = true;
			if (mStroke == Stroke::FingerPending) {
				mStroke = Stroke::None;
				ui()->releasePointer(*this);
			} else if (mStroke == Stroke::Painting) discardStroke();
			else if (mStroke == Stroke::Panning) endStroke();
			return;
		}
		if (gesture.phase == GesturePhase::End) {
			mGestureFingersDown = false;
			mSinceGestureMs = 0;
			startTicking();
			changeView([&event](Document& view) {
				if (std::abs(view.rotation()) < kRotationSnapRadians) view.setRotationAt(event.position(), 0);
			});
			if (gesture.tapFingers > 0 && onFingerTap) onFingerTap(gesture.tapFingers);
			return;
		}

		changeView([&event, &gesture](Document& view) {
			view.pan(gesture.pan);
			if (gesture.scale != 1) view.zoomAt(event.position(), gesture.scale);
			if (gesture.rotation != 0) view.rotateAt(event.position(), gesture.rotation);
		});
	}

	SpringParameters CanvasWidget::viewSpring(halnf stiffness) {
		return SpringParameters::criticallyDamped(40.0f + 360.0f * (1.0f - std::clamp(stiffness, 0.0f, 1.0f)));
	}

	void CanvasWidget::changeView(const std::function<void(Document&)>& change) {
		if (mSettings.lockView) return;
		const bool instant = mSettings.viewStiffness >= 1.0f;
		if (mViewMoving && !showsSpringView()) mViewMoving = false;
		if (!mViewMoving) mViewTarget = { mDocument.panWorld(), mDocument.zoom(), mDocument.rotation() };

		const ViewTarget shown = { mDocument.panWorld(), mDocument.zoom(), mDocument.rotation() };
		mDocument.setPanWorld(mViewTarget.pan);
		mDocument.setZoom(mViewTarget.zoom);
		mDocument.setRotation(mViewTarget.rotation);
		change(mDocument);
		mViewTarget = { mDocument.panWorld(), mDocument.zoom(), mDocument.rotation() };
		if (instant) {
			mViewMoving = false;
			requestPaint();
			return;
		}

		const SpringParameters spring = viewSpring(mSettings.viewStiffness);
		if (!mViewMoving) {
			mPanSpring.snapTo(shown.pan);
			mZoomSpring.snapTo(std::log(shown.zoom));
			mRotationSpring.snapTo(shown.rotation);
		}
		mPanSpring.setParameters(spring);
		mZoomSpring.setParameters(spring);
		mRotationSpring.setParameters(spring);
		mPanSpring.setTarget(mViewTarget.pan);
		mZoomSpring.setTarget(std::log(mViewTarget.zoom));
		constexpr halnf kHalfTurn = 3.14159265f;
		halnf turn = std::fmod(mViewTarget.rotation - mRotationSpring.value(), 2 * kHalfTurn);
		if (turn > kHalfTurn) turn -= 2 * kHalfTurn;
		if (turn < -kHalfTurn) turn += 2 * kHalfTurn;
		mRotationSpring.setTarget(mRotationSpring.value() + turn);

		mDocument.setPanWorld(shown.pan);
		mDocument.setZoom(shown.zoom);
		mDocument.setRotation(shown.rotation);
		mShownView = { mDocument.panWorld(), mDocument.zoom(), mDocument.rotation() };
		mViewMoving = true;
		startTicking();
		requestPaint();
	}

	bool CanvasWidget::showsSpringView() const {
		return mDocument.panWorld() == mShownView.pan && mDocument.zoom() == mShownView.zoom &&
					 mDocument.rotation() == mShownView.rotation;
	}

	void CanvasWidget::advanceView(halnf deltaMs) {
		if (!mViewMoving) return;
		if (!showsSpringView()) {
			mViewMoving = false;
			return;
		}
		mPanSpring.step(deltaMs);
		mZoomSpring.step(deltaMs);
		mRotationSpring.step(deltaMs);

		const halnf zoom = std::exp(mZoomSpring.value());
		const bool settled =
			mPanSpring.isSettled(0.05f / zoom) && mZoomSpring.isSettled(0.0005f) && mRotationSpring.isSettled(0.0005f);
		if (settled) {
			mDocument.setPanWorld(mViewTarget.pan);
			mDocument.setZoom(mViewTarget.zoom);
			mDocument.setRotation(mViewTarget.rotation);
			mViewMoving = false;
		} else {
			mDocument.setPanWorld(mPanSpring.value());
			mDocument.setZoom(zoom);
			mDocument.setRotation(mRotationSpring.value());
		}
		mShownView = { mDocument.panWorld(), mDocument.zoom(), mDocument.rotation() };
		requestPaint();
	}

	void CanvasWidget::discardStroke() {
		mStabilizer.finish();
		mStabilizer.takeOutput();
		stopTicking();
		mDocument.discardStroke();
		mStroke = Stroke::None;
		ui()->releasePointer(*this);
		refreshCursor();
	}

	void CanvasWidget::cancelStroke() {
		if (mStroke == Stroke::None) return;
		endStroke();
	}

	void CanvasWidget::endStroke() {
		if (mStroke == Stroke::Gradient) finishGradient(mPickLocal);
		if (mStroke == Stroke::MovingSymmetry) mSymmetryMoveArmed = false;
		if (mStroke == Stroke::MovingPerspective) mPerspectiveMoveArmed = false;
		if (mStroke == Stroke::Selecting) finishSelection(mPickLocal);
		if (mStroke == Stroke::Cropping) {
			const Vec2F a = mDocument.screenToWorld(mToolOrigin);
			const Vec2F b = mDocument.screenToWorld(mPickLocal);
			CropResult result = CropResult::NoRegion;
			if ((mPickLocal - mToolOrigin).length() >= kToolClickPixels) {
				result = mDocument.cropTo(
					(int4) std::lround(std::min(a.x, b.x)),
					(int4) std::lround(std::min(a.y, b.y)),
					(int4) std::lround(std::max(a.x, b.x)),
					(int4) std::lround(std::max(a.y, b.y))
				);
				if (result == CropResult::Cropped) mDocument.commit("Crop pixels");
			}
			if (onCrop) onCrop(result);
		}
		if (mStroke == Stroke::Moving) {
			const bool moved = mDocument.finishMove();
			if (moved) mDocument.commit(mSettings.move.duplicate ? "Duplicate pixels" : "Move pixels");
			if (onMoveFinished) onMoveFinished(moved);
		}
		if (mStroke == Stroke::Zooming && !mToolMoved) {
			const Vec2F origin = mToolOrigin;
			const halnf factor = mZoomOut ? 1.0f / kZoomClickFactor : kZoomClickFactor;
			changeView([origin, factor](Document& view) { view.zoomAt(origin, factor); });
		}
		if (mStroke == Stroke::Painting) {
			mStabilizer.finish();
			if (mLineLocked) mStabilizer.takeOutput();
			else {
				paintStabilized();
				walkPath(true);
			}
			stopTicking();
			mDocument.finishStroke(
				mEllipseStroke ? std::string("Ellipse")
				: mLineLocked  ? std::string("Line")
											 : strokeLabel(mBrush)
			);
			mLineLocked = false;
			mEllipseStroke = false;
		}
		const bool painted = mStroke == Stroke::Painting;
		if (mStroke == Stroke::Adjusting && mAdjustOnce) mAdjustTool.reset();
		mStroke = Stroke::None;
		ui()->releasePointer(*this);
		refreshCursor();
		if (painted && onStrokeFinished) onStrokeFinished();
	}

	void CanvasWidget::fillAt(Vec2F local) {
		RGBA color = mBrush.color;
		color.a = mBrush.opacity();
		const BucketResult result = mDocument.bucketFill(mDocument.screenToWorld(local), color, mSettings.bucket);
		if (result == BucketResult::Filled) mDocument.commit("Paint bucket");
		if (onBucket) onBucket(result);
		requestPaint();
	}

	void CanvasWidget::setTool(CanvasTool tool) {
		if (tool == CanvasTool::Transform && mTool != CanvasTool::Transform) {
			if (!startTransform()) {
				if (onTransformRefused) onTransformRefused();
				return;
			}
		} else if (tool != CanvasTool::Transform && mTool == CanvasTool::Transform) {
			applyTransform();
		}
		mTool = tool;
		refreshCursor();
		requestPaint();
	}

	bool CanvasWidget::startTransform() {
		if (!mDocument.beginTransform()) return false;
		int4 left = 0, top = 0, right = 0, bottom = 0;
		mDocument.transformBounds(left, top, right, bottom);
		mTransform = LayerTransform();
		mTransform.pivot = { (left + right) / 2.0f, (top + bottom) / 2.0f };
		if (onTransformActive) onTransformActive(true);
		return true;
	}

	void CanvasWidget::setPendingTransform(const LayerTransform& transform) {
		mTransform = transform;
		mDocument.previewTransform(mTransform);
		requestPaint();
	}

	void CanvasWidget::applyTransform() {
		if (!isTransforming()) return;
		if (mStroke == Stroke::Transforming) endStroke();
		const bool changed = mDocument.finishTransform();
		if (changed) mDocument.commit("Transform");
		mTool = CanvasTool::Paint;
		refreshCursor();
		requestPaint();
		if (onTransformActive) onTransformActive(false);
		if (onTransformApplied) onTransformApplied(changed);
	}

	void CanvasWidget::cancelTransform() {
		if (!isTransforming()) return;
		if (mStroke == Stroke::Transforming) endStroke();
		mDocument.cancelTransform();
		mTool = CanvasTool::Paint;
		refreshCursor();
		requestPaint();
		if (onTransformActive) onTransformActive(false);
	}

	LayerTransform CanvasWidget::rebased(const LayerTransform& transform, Vec2F pivot) {
		LayerTransform result = transform;
		const Vec2F image = transform.apply(pivot);
		result.pivot = pivot;
		result.offset = image - pivot;
		return result;
	}

	Vec2F CanvasWidget::transformCenterWorld(const LayerTransform& transform) const {
		int4 left = 0, top = 0, right = 0, bottom = 0;
		mDocument.transformBounds(left, top, right, bottom);
		return transform.apply({ (left + right) / 2.0f, (top + bottom) / 2.0f });
	}

	void CanvasWidget::flipTransform(bool horizontal) {
		if (!isTransforming()) return;
		int4 left = 0, top = 0, right = 0, bottom = 0;
		mDocument.transformBounds(left, top, right, bottom);
		LayerTransform flipped = rebased(mTransform, { (left + right) / 2.0f, (top + bottom) / 2.0f });
		if (horizontal) flipped.scale.x = -flipped.scale.x;
		else flipped.scale.y = -flipped.scale.y;
		setPendingTransform(flipped);
	}

	std::array<Vec2F, 4> CanvasWidget::transformCorners() const {
		int4 left = 0, top = 0, right = 0, bottom = 0;
		mDocument.transformBounds(left, top, right, bottom);
		const Vec2F source[4] = {
			{ (halnf) left, (halnf) top },
			{ (halnf) right, (halnf) top },
			{ (halnf) right, (halnf) bottom },
			{ (halnf) left, (halnf) bottom },
		};
		std::array<Vec2F, 4> corners;
		for (int4 index = 0; index < 4; index++)
			corners[(size_t) index] = mDocument.worldToScreen(mTransform.apply(source[index]));
		return corners;
	}

	void CanvasWidget::beginTransformDrag(Vec2F local, bool proportional) {
		const std::array<Vec2F, 4> corners = transformCorners();
		mDragTransform = mTransform;
		mDragStartWorld = mDocument.screenToWorld(local);
		mScaleProportional = proportional;
		for (int4 index = 0; index < 4; index++) {
			if ((corners[(size_t) index] - local).length() <= kTransformHandleRadius) {
				mTransformDrag = TransformDrag::Scale;
				mScaleCorner = index;
				return;
			}
		}
		bool inside = true;
		halnf sign = 0;
		for (int4 index = 0; index < 4; index++) {
			const Vec2F a = corners[(size_t) index];
			const Vec2F b = corners[(size_t) ((index + 1) % 4)];
			const halnf cross = (b.x - a.x) * (local.y - a.y) - (b.y - a.y) * (local.x - a.x);
			if (cross == 0) continue;
			if (sign == 0) sign = cross > 0 ? 1.0f : -1.0f;
			else if ((cross > 0 ? 1.0f : -1.0f) != sign) inside = false;
		}
		mTransformDrag = inside ? TransformDrag::Move : TransformDrag::Rotate;
	}

	void CanvasWidget::continueTransformDrag(Vec2F local) {
		const Vec2F world = mDocument.screenToWorld(local);
		LayerTransform next = mDragTransform;
		if (mTransformDrag == TransformDrag::Move) {
			next.offset = mDragTransform.offset + (world - mDragStartWorld);
		} else if (mTransformDrag == TransformDrag::Rotate) {
			const Vec2F center = transformCenterWorld(mDragTransform);
			const Vec2F from = mDragStartWorld - center;
			const Vec2F to = world - center;
			const halnf delta = std::atan2(to.y, to.x) - std::atan2(from.y, from.x);
			const halnf c = std::cos(delta);
			const halnf s = std::sin(delta);
			const Vec2F image = mDragTransform.pivot + mDragTransform.offset - center;
			next.rotation = mDragTransform.rotation + delta;
			next.offset = center + Vec2F{ image.x * c - image.y * s, image.x * s + image.y * c } - mDragTransform.pivot;
		} else {
			int4 left = 0, top = 0, right = 0, bottom = 0;
			mDocument.transformBounds(left, top, right, bottom);
			const Vec2F source[4] = {
				{ (halnf) left, (halnf) top },
				{ (halnf) right, (halnf) top },
				{ (halnf) right, (halnf) bottom },
				{ (halnf) left, (halnf) bottom },
			};
			const Vec2F corner = source[mScaleCorner];
			const Vec2F opposite = source[(mScaleCorner + 2) % 4];
			next = rebased(mDragTransform, opposite);
			const Vec2F anchor = next.apply(opposite);
			const halnf c = std::cos(next.rotation);
			const halnf s = std::sin(next.rotation);
			const Vec2F reach = world - anchor;
			const Vec2F turned = { reach.x * c + reach.y * s, -reach.x * s + reach.y * c };
			const Vec2F span = corner - opposite;
			halnf sx = span.x != 0 ? turned.x / span.x : 1.0f;
			halnf sy = span.y != 0 ? turned.y / span.y : 1.0f;
			if (mScaleProportional) {
				const halnf ratio =
					std::abs(mDragTransform.scale.y) > 0 ? std::abs(mDragTransform.scale.x / mDragTransform.scale.y) : 1.0f;
				const halnf uniform = (std::abs(sx) / std::max(ratio, kMinimumTransformScale) + std::abs(sy)) / 2.0f;
				sx = (sx < 0 ? -1.0f : 1.0f) * uniform * ratio;
				sy = (sy < 0 ? -1.0f : 1.0f) * uniform;
			}
			const auto limit = [](halnf value) {
				return std::abs(value) < kMinimumTransformScale ? (value < 0 ? -kMinimumTransformScale : kMinimumTransformScale)
																												: value;
			};
			next.scale = { limit(sx), limit(sy) };
		}
		setPendingTransform(next);
	}

	SelectionMode CanvasWidget::selectionMode(const ui::Modifiers& modifiers) const {
		if (modifiers.alt) return SelectionMode::Subtract;
		if (modifiers.shift) return SelectionMode::Add;
		return SelectionMode::Replace;
	}

	void CanvasWidget::finishSelection(Vec2F local) {
		Selection& selection = mDocument.editSelection();
		const Selection before = selection;
		if ((local - mToolOrigin).length() < kToolClickPixels) {
			if (mSelectMode == SelectionMode::Replace && mSettings.selection.clickToDeselect) selection.clear();
		} else if (mTool == CanvasTool::Lasso) {
			std::vector<Vec2F> world;
			world.reserve(mLassoPath.size());
			for (const Vec2F& point : mLassoPath)
				world.push_back(mDocument.screenToWorld(point));
			selection.addPolygon(world, mSelectMode);
		} else {
			const Vec2F a = mDocument.screenToWorld(mToolOrigin);
			const Vec2F b = mDocument.screenToWorld(local);
			const Vec2F low = { std::min(a.x, b.x), std::min(a.y, b.y) };
			const Vec2F high = { std::max(a.x, b.x), std::max(a.y, b.y) };
			if (mSettings.selection.marquee == MarqueeShape::Ellipse)
				selection.addEllipse((low + high) / 2.0f, (high - low) / 2.0f, mSelectMode);
			else
				selection.addRectangle(
					(int4) std::lround(low.x),
					(int4) std::lround(low.y),
					(int4) std::lround(high.x),
					(int4) std::lround(high.y),
					mSelectMode
				);
		}
		mLassoPath.clear();
		if (!(before == selection)) {
			if (mSettings.selection.autoFill && selection.isActive() && mSelectMode != SelectionMode::Subtract) {
				RGBA color = mBrush.color;
				color.a = 1;
				mDocument.fillSelection(color);
			}
			mDocument.commit(selection.isActive() ? "Select" : "Deselect");
			if (onSelectionChanged) onSelectionChanged();
		}
		requestPaint();
	}

	void CanvasWidget::finishPolygon() {
		if (mPolygon.size() >= 3) {
			Selection& selection = mDocument.editSelection();
			const Selection before = selection;
			std::vector<Vec2F> world;
			for (const Vec2F& point : mPolygon)
				world.push_back(mDocument.screenToWorld(point));
			selection.addPolygon(world, mSelectMode);
			if (!(before == selection)) {
				if (mSettings.selection.autoFill && selection.isActive() && mSelectMode != SelectionMode::Subtract) {
					RGBA color = mBrush.color;
					color.a = 1;
					mDocument.fillSelection(color);
				}
				mDocument.commit("Select");
				if (onSelectionChanged) onSelectionChanged();
			}
		}
		mPolygon.clear();
		requestPaint();
	}

	void CanvasWidget::cancelPolygon() {
		mPolygon.clear();
		requestPaint();
	}

	void CanvasWidget::paintSelection(ui::Painter& painter) const {
		const Selection& selection = mDocument.selection();
		if (selection.isActive()) {
			const RGBA dark = { 0, 0, 0, 0.9f };
			const RGBA light = { 1, 1, 1, 0.9f };
			for (const auto& [from, to] : selection.outline()) {
				const Vec2F a = mDocument.worldToScreen(from);
				const Vec2F b = mDocument.worldToScreen(to);
				painter.line(a, b, dark, 1.0f);
				const halnf phase = std::fmod(from.x + from.y + mAntPhase, kAntDashPixels * 2);
				if (phase < kAntDashPixels) painter.line(a, b, light, 1.0f);
			}
		}
		if (mStroke == Stroke::Selecting) {
			if (mTool == CanvasTool::Lasso) {
				painter.polyline(mLassoPath, kContourOuter, 3.0f);
				painter.polyline(mLassoPath, kContourInner, 1.0f);
			} else {
				const RectF area = cropRect();
				if (mSettings.selection.marquee == MarqueeShape::Ellipse) {
					std::vector<Vec2F> points;
					for (int4 step = 0; step <= 48; step++) {
						const halnf angle = 2.0f * 3.14159265f * (halnf) step / 48.0f;
						points.push_back(
							area.pos + area.size / 2.0f +
							Vec2F{ std::cos(angle) * area.size.x / 2.0f, std::sin(angle) * area.size.y / 2.0f }
						);
					}
					painter.polyline(points, kContourOuter, 3.0f);
					painter.polyline(points, kContourInner, 1.0f);
				} else {
					painter.strokeRect(area, kContourOuter, 0, 3.0f);
					painter.strokeRect(area, kContourInner, 0, 1.0f);
				}
			}
		}
		if (!mPolygon.empty()) {
			std::vector<Vec2F> points = mPolygon;
			points.push_back(mPickLocal);
			painter.polyline(points, kContourOuter, 3.0f);
			painter.polyline(points, kContourInner, 1.0f);
			painter.strokeCircle(mPolygon.front(), kPolygonCloseRadius, kContourInner, 1.0f);
		}
	}

	RectF CanvasWidget::cropRect() const {
		const Vec2F low = { std::min(mToolOrigin.x, mPickLocal.x), std::min(mToolOrigin.y, mPickLocal.y) };
		const Vec2F high = { std::max(mToolOrigin.x, mPickLocal.x), std::max(mToolOrigin.y, mPickLocal.y) };
		return ui::makeRect(low, high - low);
	}

	void CanvasWidget::paintTransform(ui::Painter& painter) const {
		if (mStroke == Stroke::Cropping) {
			const RectF area = cropRect();
			const RGBA shade = { 0, 0, 0, 0.45f };
			painter.fillRect(ui::makeRect(0, 0, size().x, area.pos.y), shade);
			painter.fillRect(ui::makeRect(0, area.pos.y + area.size.y, size().x, size().y - area.pos.y - area.size.y), shade);
			painter.fillRect(ui::makeRect(0, area.pos.y, area.pos.x, area.size.y), shade);
			painter.fillRect(
				ui::makeRect(area.pos.x + area.size.x, area.pos.y, size().x - area.pos.x - area.size.x, area.size.y), shade
			);
			painter.strokeRect(area, kContourInner, 0, 1.5f);
		}
		if (!isTransforming()) return;
		const std::array<Vec2F, 4> corners = transformCorners();
		std::vector<Vec2F> outline(corners.begin(), corners.end());
		outline.push_back(corners[0]);
		painter.polyline(outline, kContourOuter, 3.0f);
		painter.polyline(outline, kContourInner, 1.0f);
		for (const Vec2F& corner : corners) {
			painter.fillCircle(corner, 7, kContourOuter);
			painter.fillCircle(corner, 5, kContourInner);
		}
	}

	void CanvasWidget::finishGradient(Vec2F local) {
		if ((local - mToolOrigin).length() < kToolClickPixels) return;
		RGBA start = mBrush.color;
		start.a = mBrush.opacity();
		RGBA end = gradientEndColor ? gradientEndColor() : RGBA(1, 1, 1, 1);
		end.a = mBrush.opacity();
		const GradientResult result = mDocument.drawGradient(
			mDocument.screenToWorld(mToolOrigin), mDocument.screenToWorld(local), start, end, mSettings.gradient
		);
		if (result == GradientResult::Drawn) mDocument.commit("Gradient");
		if (onGradient) onGradient(result);
	}

	bool CanvasWidget::wantsPicker() const { return mTool == CanvasTool::Picker || (heldPicker && heldPicker()); }

	RGBA CanvasWidget::sampleAt(Vec2F local) const {
		RGBA color = mDocument.sampleColor(mDocument.screenToWorld(local), mSettings.picker.sampleAllLayers);
		if (!mSettings.picker.sampleOpacity) color.a = color.a > 0 ? 1.0f : 0.0f;
		return color;
	}

	void CanvasWidget::pickAt(Vec2F local) {
		mPickLocal = local;
		const RGBA color = sampleAt(local);
		if (color.a <= 0) {
			requestPaint();
			return;
		}
		mPicked = color;
		if (onColorPicked) onColorPicked(color);
		requestPaint();
	}

	void CanvasWidget::paintPicker(ui::Painter& painter) const {
		if (mStroke == Stroke::Gradient) {
			painter.line(mToolOrigin, mPickLocal, kContourOuter, 3.0f);
			painter.line(mToolOrigin, mPickLocal, kContourInner, 1.0f);
			if (mSettings.gradient.shape == GradientShape::Radial) {
				const halnf radius = (mPickLocal - mToolOrigin).length();
				painter.strokeCircle(mToolOrigin, radius, kContourOuter, 3.0f);
				painter.strokeCircle(mToolOrigin, radius, kContourInner, 1.0f);
			}
			painter.fillCircle(mToolOrigin, 4, kContourInner);
			painter.fillCircle(mPickLocal, 4, kContourInner);
			return;
		}
		if (mStroke != Stroke::Picking) return;
		const Vec2F at = mPickLocal;
		const RGBA now = { mPicked.r, mPicked.g, mPicked.b, 1 };
		const RGBA before = { mPickStart.r, mPickStart.g, mPickStart.b, 1 };
		switch (mSettings.picker.style) {
			case PickerStyle::Cursor: break;
			case PickerStyle::Ring:
				{
					constexpr halnf kRadius = 56;
					constexpr halnf kWidth = 18;
					painter.strokeCircle(at, kRadius, RGBA(0, 0, 0, 0.5f), kWidth + 4);
					std::vector<Vec2F> top;
					std::vector<Vec2F> bottom;
					for (int4 step = 0; step <= 24; step++) {
						const halnf angle = 3.14159265f * (halnf) step / 24.0f;
						top.push_back(at + Vec2F{ -std::cos(angle) * kRadius, -std::sin(angle) * kRadius });
						bottom.push_back(at + Vec2F{ std::cos(angle) * kRadius, std::sin(angle) * kRadius });
					}
					painter.polyline(top, now, kWidth);
					painter.polyline(bottom, before, kWidth);
					break;
				}
			case PickerStyle::Balloon:
				{
					const Vec2F center = at + Vec2F{ 0, -70 };
					painter.fillPolygon({ at, center + Vec2F{ -12, 22 }, center + Vec2F{ 12, 22 } }, RGBA(0, 0, 0, 0.5f));
					painter.fillCircle(center, 34, RGBA(0, 0, 0, 0.5f));
					painter.fillCircle(center, 31, before);
					painter.fillCircle(center, 22, now);
					break;
				}
			case PickerStyle::Magnifier:
				{
					const Vec2F center = at + Vec2F{ 0, -90 };
					constexpr halnf kCell = 8;
					constexpr int4 kSpan = 4;
					const halnf half = kCell * (kSpan + 0.5f);
					painter.fillRect(
						ui::makeRect(center - Vec2F{ half + 3, half + 3 }, Vec2F{ (half + 3) * 2, (half + 3) * 2 }),
						RGBA(0, 0, 0, 0.6f),
						6
					);
					for (int4 y = -kSpan; y <= kSpan; y++) {
						for (int4 x = -kSpan; x <= kSpan; x++) {
							const RGBA sample = sampleAt(at + Vec2F{ (halnf) x, (halnf) y });
							const RGBA shown = sample.a > 0 ? RGBA(sample.r, sample.g, sample.b, 1) : RGBA(0.5f, 0.5f, 0.5f, 1);
							painter.fillRect(
								ui::makeRect(
									center + Vec2F{ (halnf) x * kCell - kCell / 2, (halnf) y * kCell - kCell / 2 }, Vec2F{ kCell, kCell }
								),
								shown
							);
						}
					}
					painter.strokeRect(
						ui::makeRect(center - Vec2F{ kCell / 2, kCell / 2 }, Vec2F{ kCell, kCell }), RGBA(1, 1, 1, 1), 0, 1.5f
					);
					painter.fillRect(ui::makeRect(center + Vec2F{ -half, half + 6.0f }, Vec2F{ half * 2.0f, 14.0f }), now, 3);
					break;
				}
		}
	}

	void CanvasWidget::paintSegment(Vec2F fromWorld, Vec2F toWorld, halnf fromPressure, halnf toPressure) {
		const Vec2F delta = toWorld - fromWorld;
		const halnf length = (halnf) delta.length();
		if (length <= 0.0f) return;

		halnf position = 0;
		halnf sinceDab = mDabCarry;
		while (true) {
			const halnf pressure = fromPressure + (toPressure - fromPressure) * (position / length);
			const halnf needed = mBrush.dabDistance(pressure) - sinceDab;
			if (position + needed > length) {
				mDabCarry = sinceDab + (length - position);
				return;
			}
			position += needed > 0.0f ? needed : 0.0f;
			sinceDab = 0;
			const halnf at = position / length;
			stamp(fromWorld + delta * at, fromPressure + (toPressure - fromPressure) * at);
		}
	}
}
