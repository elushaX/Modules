#include "Docking.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Layout.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cctype>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kDragThreshold = 4;
	constexpr halnf kEdgeGrip = 6;
	constexpr halnf kCornerGrip = 16;
	constexpr halnf kMinimumDockedLength = 80;
	constexpr halnf kMaximumDockedFraction = 0.85f;
	constexpr halnf kDropZoneFraction = 0.3f;
	constexpr halnf kDropTargetFraction = 0.1f;
	constexpr halnf kMinimumDropTarget = 36;
	constexpr halnf kDropZoneInset = 8;
	constexpr halnf kVisibleWhenOffscreen = 40;
	constexpr halnf kFloatOpeningScale = 0.85f;
	constexpr halnf kStripeLabelPadding = 10;
	constexpr halnf kStripeIconGap = 4;
	constexpr halnf kStripeInset = 3;
	constexpr halnf kStripeButtonGap = 4;

	const Vec2F kMinimumFloatingSize = { 160, 120 };
	const Vec2F kMaximumDraggedSize = { 360, 420 };

	int indexOf(DockSide side) { return (int) side; }

	halnf clampValue(halnf value, halnf minimum, halnf maximum) {
		if (value > maximum) value = maximum;
		if (value < minimum) value = minimum;
		return value;
	}

	RGBA withAlpha(const RGBA& color, halnf alpha) { return { color.r, color.g, color.b, alpha }; }
}

StripeButton::StripeButton(std::string title, ImageId icon) :
	Button(std::move(title)) {
	setStyle(Style::Flat);
	setDebugName("stripe: " + text());
	setTooltip(text());
	if (icon.isValid()) setIcon(icon);
}

halnf StripeButton::iconLength() const { return hasIcon() ? theme().metrics.lineHeight + kStripeIconGap : 0; }

halnf StripeButton::stripeLength() const {
	const Metrics& metrics = theme().metrics;
	return textWidth(text(), metrics.smallFontSize) + iconLength() + kStripeLabelPadding * 2;
}

Extent StripeButton::measureContent(Axis axis, halnf) const {
	if (axis == Axis::Horizontal) return Extent::rigid(theme().metrics.controlHeight);
	return Extent::rigid(stripeLength());
}

RGBA StripeButton::backgroundTarget() const {
	const Palette& palette = theme().palette;
	if (isPressed()) return palette.controlPressed;
	if (isSelected()) return isHovered() ? palette.controlHovered : palette.neutralSelection;
	if (isHovered()) return palette.controlHovered;
	return palette.control;
}

RGBA StripeButton::labelColor() const {
	const Palette& palette = theme().palette;
	if (!isEffectivelyEnabled()) return palette.disabledText;
	return isSelected() || isHovered() ? palette.text : palette.mutedText;
}

void StripeButton::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF box = localRect();
	const halnf radius = style.metrics.cornerRadius;

	painter.fillRect(box, background(), radius);
	painter.strokeRect(box, isSelected() ? style.palette.controlBorder : style.palette.border, radius);
	if (showsFocusRing()) painter.strokeRect(insetRect(box, -2), style.palette.focusRing, radius + 2, 2);

	RectF label = makeRect(box.pos.x, box.pos.y + kStripeLabelPadding, box.size.x, box.size.y - kStripeLabelPadding * 2);
	if (hasIcon()) {
		const halnf iconSize = style.metrics.lineHeight;
		const RectF iconBox =
			makeRect(box.center().x - iconSize / 2, label.pos.y + label.size.y - iconSize, iconSize, iconSize);
		painter.image(iconBox, icon());
		label.size.y -= iconLength();
	}

	painter.verticalText(text(), label, style.metrics.smallFontSize, TextDirection::BottomToTop, labelColor());
}

OptionsButton::OptionsButton() {
	setStyle(Style::Flat);
	setDebugName("options button");
}

Extent OptionsButton::measureContent(Axis, halnf) const { return Extent::rigid(theme().metrics.iconButtonSize); }

void OptionsButton::paint(Painter& painter) {
	const Theme& style = theme();
	const RGBA color = isHovered() || isPopupOpen() ? style.palette.text : style.palette.mutedText;
	const Vec2F center = localRect().center();

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
	for (halnf offset : { -5.0f, 0.0f, 5.0f })
		painter.fillCircle(center + Vec2F(0.0f, offset), 1.5f, color);
}

RGBA OptionsButton::backgroundTarget() const {
	const Palette& palette = theme().palette;
	if (isPopupOpen() || isPressed()) return palette.controlPressed;
	if (isHovered()) return palette.controlHovered;
	return withAlpha(palette.controlHovered, 0);
}

class DockPanel::PanelLayout : public Layout {
public:
	explicit PanelLayout(DockPanel& panel) :
		mPanel(panel) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		Extent header = context.measure(mPanel.mHeader, axis, crossLength);
		Extent body = context.measure(mPanel.mBody, axis, crossLength);
		if (axis == Axis::Horizontal)
			return { std::max(header.minimum, body.minimum), std::max(header.preferred, body.preferred) };
		return { header.minimum + body.minimum, header.preferred + body.preferred };
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		const halnf headerHeight = context.measure(mPanel.mHeader, Axis::Vertical, contentArea.size.x).preferred;
		context.place(mPanel.mHeader, makeRect(contentArea.pos, Vec2F(contentArea.size.x, headerHeight)));
		context.place(
			mPanel.mBody,
			makeRect(
				contentArea.pos.x, contentArea.pos.y + headerHeight, contentArea.size.x, contentArea.size.y - headerHeight
			)
		);

		if (!mPanel.mCornerGrip.isVisible()) return;
		const Vec2F corner = contentArea.pos + contentArea.size;
		context.place(
			mPanel.mCornerGrip, makeRect(corner - Vec2F(kCornerGrip, kCornerGrip), Vec2F(kCornerGrip, kCornerGrip))
		);
	}

private:
	DockPanel& mPanel;
};

DockPanel::DockPanel(std::string title, DockSide side) :
	Panel(Surface::Base),
	mSide(side),
	mTitle(std::move(title)) {
	setDebugName("dock panel: " + mTitle.text());
	setVisible(false);
	setLayout(std::make_unique<PanelLayout>(*this));

	BoxLayout& headerLayout = mHeader.emplaceLayout<RowLayout>(2);
	headerLayout.setPadding(Insets::symmetric(8, 1));
	headerLayout.setCrossAlign(CrossAlign::Center);
	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());

	mHideButton.setTooltip("Hide");
	mHideButton.setOnActivate([this]() {
		if (mArea) mArea->hidePanel(*this);
	});
	buildOptionsMenu();

	mHeader.addChild(mTitle);
	mHeader.addChild(mOptionsButton);
	mHeader.addChild(mHideButton);

	mBody.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mBody.emplaceLayout<StackLayout>().setPadding(Insets::uniform(6));

	addChild(mHeader);
	addChild(mBody);
	addChild(mCornerGrip);
	refreshChrome();
}

DockPanel::~DockPanel() {
	if (mArea) mArea->removePanel(*this);
}

void DockPanel::setContent(Widget* content) {
	if (mContent) mBody.removeChild(*mContent);

	mContent = content;
	if (mContent) mBody.addChild(*mContent);
}

void DockPanel::setDockedLength(halnf length) {
	mDockedLength = length > kMinimumDockedLength ? length : kMinimumDockedLength;
	if (parent()) parent()->requestLayout();
}

void DockPanel::setFloatingRect(const RectF& rect) {
	mFloatingRect = makeRect(
		rect.pos, { std::max(rect.size.x, kMinimumFloatingSize.x), std::max(rect.size.y, kMinimumFloatingSize.y) }
	);
	if (parent()) parent()->requestLayout();
}

RectF DockPanel::resizeGripRect() const { return mCornerGrip.rect(); }

void DockPanel::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary || !mArea) return;

				if (!contains(headerRect(), event.position())) return;
				beginInteraction(Interaction::PendingDrag, event);
				break;
			}

		case Event::Type::PointerMove:
			if (mInteraction == Interaction::None) return;
			updateInteraction(event);
			break;

		case Event::Type::PointerUp:
			if (mInteraction == Interaction::None) return;
			endInteraction(event);
			break;

		default: return;
	}

	event.accept();
	requestPaint();
}

void DockPanel::paint(Painter& painter) { Panel::paint(painter); }

void DockPanel::paintOverlay(Painter&) {}

void DockPanel::onActiveWithinChanged() {
	if (!containsActiveWidget() && mArea) mArea->panelLostActivity(*this);
}

DockPanel::CornerGrip::CornerGrip(DockPanel& panel) :
	mPanel(panel) {
	setDebugName("corner grip");
	setGeometryAnimated(false);
	setSizing(AxisSizing::fixed(kCornerGrip), AxisSizing::fixed(kCornerGrip));
}

void DockPanel::CornerGrip::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary || !mPanel.mArea) return;
			mDragging = true;
			mGrabOffset = event.position() - size();
			ui()->capturePointer(*this);
			mPanel.mArea->beginDirectResize(mPanel);
			break;

		case Event::Type::PointerMove:
			if (!mDragging) return;
			mPanel.mArea->floatResized(mPanel, mPanel.mArea->toLocal(event.windowPosition() - mGrabOffset));
			break;

		case Event::Type::PointerUp:
			if (!mDragging || event.button() != PointerButton::Primary) return;
			mDragging = false;
			ui()->releasePointer(*this);
			mPanel.mArea->endDirectResize(mPanel);
			break;

		default: return;
	}

	event.accept();
	refreshReveal();
}

void DockPanel::CornerGrip::onHoverChanged() { refreshReveal(); }

void DockPanel::CornerGrip::refreshReveal() {
	mReveal.animateTo(isHovered() || mDragging ? 1.0f : 0.0f);
	startTicking();
}

void DockPanel::CornerGrip::onTick(halnf deltaMs) {
	if (!mReveal.advance(deltaMs)) stopTicking();
	requestPaint();
}

void DockPanel::CornerGrip::paint(Painter& painter) {
	const halnf reveal = mReveal.value();
	if (reveal <= 0.01f) return;

	const Palette& palette = theme().palette;
	const RGBA base = mDragging ? palette.text : palette.mutedText;
	const Vec2F corner = size() - Vec2F(5.0f, 5.0f);
	painter.polyline(
		{ corner - Vec2F(8.0f, 0.0f), corner, corner - Vec2F(0.0f, 8.0f) }, withAlpha(base, base.a * reveal), 1.5f
	);
}

void DockPanel::buildOptionsMenu() {
	const struct {
		const char* text;
		ViewMode mode;
	} modes[] = { { "Dock Pinned", ViewMode::Docked },
								{ "Dock Unpinned", ViewMode::Undocked },
								{ "Float", ViewMode::Float } };

	for (const auto& option : modes) {
		const ViewMode mode = option.mode;
		mViewModeMenu.addToggle(
			option.text,
			[this, mode]() { return mViewMode == mode; },
			[this, mode]() {
				if (mArea) mArea->setViewMode(*this, mode);
			}
		);
	}

	const struct {
		const char* text;
		DockSide side;
	} sides[] = { { "Left", DockSide::Left }, { "Right", DockSide::Right }, { "Bottom", DockSide::Bottom } };

	for (const auto& option : sides) {
		const DockSide side = option.side;
		mMoveToMenu.addToggle(
			option.text,
			[this, side]() { return mSide == side && mViewMode != ViewMode::Float; },
			[this, side]() {
				if (mArea) mArea->dockPanel(*this, side);
			}
		);
	}

	mOptionsMenu.addSubmenu("View Mode", mViewModeMenu);
	mOptionsMenu.addSubmenu("Move To", mMoveToMenu);
	mOptionsMenu.addSeparator();
	mOptionsMenu.addItem("Hide", [this]() {
		if (mArea) mArea->hidePanel(*this);
	});

	mOptionsButton.setTooltip("Options");
	mOptionsButton.setPopupContent(&mOptionsMenu);
	mOptionsButton.popupOptions().align = PlacementAlign::End;
}

void DockPanel::refreshChrome() {
	const bool detached = mViewMode != ViewMode::Docked || mDragging;

	const Insets padding = detached ? Insets::uniform(1) : Insets();
	layout()->setPadding(padding);
	mCornerGrip.setVisible(mViewMode == ViewMode::Float && !mDragging);

	setSurface(detached ? Surface::Raised : Surface::Base);
	setBorder(detached);
	setCornerRadius(detached ? theme().metrics.popupCornerRadius : theme().metrics.surfaceCornerRadius);
}

void DockPanel::beginInteraction(Interaction interaction, const Event& event) {
	mInteraction = interaction;
	mPressPosition = event.windowPosition();
	mGrabOffset = event.position();
	ui()->capturePointer(*this);
}

void DockPanel::updateInteraction(const Event& event) {
	if (mInteraction == Interaction::PendingDrag) {
		if ((event.windowPosition() - mPressPosition).length() < kDragThreshold) return;

		mInteraction = Interaction::Dragging;
		mDragging = true;
		refreshChrome();
		mArea->panelDragStarted(*this);
	}

	mArea->panelDragged(*this, event.windowPosition());
}

void DockPanel::endInteraction(const Event& event) {
	const Interaction finished = mInteraction;

	mInteraction = Interaction::None;
	mDragging = false;
	refreshChrome();
	ui()->releasePointer(*this);

	if (finished == Interaction::Dragging) mArea->panelDragEnded(*this, event.windowPosition());
}

class DockArea::DockLayout : public Layout {
public:
	explicit DockLayout(DockArea& area) :
		mArea(area) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext&, Axis, halnf) const override { return {}; }

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		const Geometry geometry = mArea.computeGeometry(contentArea);

		if (mArea.mCenter && mArea.mCenter->isVisible()) context.place(*mArea.mCenter, geometry.center);

		for (DockPanel* panel : mArea.mPanels) {
			if (panel->isVisible()) context.place(*panel, mArea.panelRect(*panel, geometry, contentArea));
		}

		for (DockSide side : { DockSide::Left, DockSide::Right, DockSide::Bottom }) {
			ResizeHandle& handle = *mArea.mEdgeHandles[indexOf(side)];
			if (handle.isVisible()) context.place(handle, geometry.gaps[indexOf(side)]);
		}

		placeStripeButtons(context, geometry);
	}

private:
	void placeStripeButtons(LayoutContext& context, const Geometry& geometry) const {
		const RectF& leftRail = geometry.stripes[indexOf(DockSide::Left)];
		const RectF& rightRail = geometry.stripes[indexOf(DockSide::Right)];

		halnf leftTop = leftRail.pos.y + kStripeInset;
		halnf leftBottom = leftRail.pos.y + leftRail.size.y - kStripeInset;
		halnf rightTop = rightRail.pos.y + kStripeInset;

		for (const Stripe& stripe : mArea.mStripes) {
			const DockSide side = stripe.panel->side();
			const RectF& rail = side == DockSide::Right ? rightRail : leftRail;
			const halnf width = rail.size.x - kStripeInset * 2;
			const halnf length = stripe.button->stripeLength();

			halnf top = 0;
			if (side == DockSide::Left) {
				top = leftTop;
				leftTop += length + kStripeButtonGap;
			} else if (side == DockSide::Bottom) {
				leftBottom -= length;
				top = leftBottom;
				leftBottom -= kStripeButtonGap;
			} else {
				top = rightTop;
				rightTop += length + kStripeButtonGap;
			}

			context.place(*stripe.button, makeRect(rail.pos.x + kStripeInset, top, width, length));
		}
	}

	DockArea& mArea;
};

DockArea::DockArea() {
	setDebugName("dock area");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	setLayout(std::make_unique<DockLayout>(*this));

	for (DockSide side : { DockSide::Left, DockSide::Right, DockSide::Bottom }) {
		const Axis axis = side == DockSide::Bottom ? Axis::Vertical : Axis::Horizontal;
		auto handle = std::make_unique<ResizeHandle>(axis);
		handle->setDebugName("dock edge handle");
		handle->setVisible(false);
		handle->setOnDragStarted([this, side]() {
			if (DockPanel* panel = openPanelOn(side)) beginDirectResize(*panel);
		});
		handle->setOnDragged([this, side](halnf coordinate) { edgeDragged(side, coordinate); });
		handle->setOnDragFinished([this, side]() {
			if (DockPanel* panel = openPanelOn(side)) endDirectResize(*panel);
		});
		addChild(*handle);
		mEdgeHandles[indexOf(side)] = std::move(handle);
	}
}

DockArea::~DockArea() {
	for (DockPanel* panel : mPanels)
		panel->mArea = nullptr;
	removeAllChildren();
}

void DockArea::setCenter(Widget* center) {
	if (mCenter) removeChild(*mCenter);

	mCenter = center;
	if (mCenter) insertChild(0, *mCenter);
}

void DockArea::addPanel(DockPanel& panel) {
	if (panel.mArea == this) return;
	if (panel.mArea) panel.mArea->removePanel(panel);

	panel.mArea = this;
	mPanels.push_back(&panel);
	addChild(panel);
	panel.setVisible(panel.mOpen);
	rebuildStripes();
}

void DockArea::removePanel(DockPanel& panel) {
	const auto position = std::find(mPanels.begin(), mPanels.end(), &panel);
	if (position == mPanels.end()) return;

	mPanels.erase(position);
	panel.mArea = nullptr;
	removeChild(panel);
	rebuildStripes();
}

void DockArea::openPanel(DockPanel& panel) {
	const bool wasOpen = panel.mOpen;

	panel.mOpen = true;
	panel.setVisible(true);
	closeOthersOnSide(panel);
	if (!wasOpen) panel.appearFrom(openingRect(panel));

	if (panel.mViewMode != DockPanel::ViewMode::Docked) panel.bringToFront();
	refreshStripeSelection();
	refreshEdgeHandles();
	requestLayout();

	if (panel.mViewMode == DockPanel::ViewMode::Undocked && ui()) ui()->setActiveWidget(&panel);
	if (!wasOpen && panel.mOnOpenChanged) panel.mOnOpenChanged(true);
}

void DockArea::hidePanel(DockPanel& panel) {
	if (!panel.mOpen) return;

	panel.mOpen = false;
	panel.setVisible(false);
	refreshStripeSelection();
	refreshEdgeHandles();
	requestLayout();
	if (panel.mOnOpenChanged) panel.mOnOpenChanged(false);
}

void DockArea::togglePanel(DockPanel& panel) {
	if (panel.mOpen) hidePanel(panel);
	else openPanel(panel);
}

void DockArea::dockPanel(DockPanel& panel, DockSide side) {
	const bool movedToAnotherSide = panel.mSide != side;

	panel.mSide = side;
	if (panel.mViewMode == DockPanel::ViewMode::Float) panel.mViewMode = DockPanel::ViewMode::Docked;
	panel.refreshChrome();

	if (movedToAnotherSide) rebuildStripes();
	openPanel(panel);
}

void DockArea::setViewMode(DockPanel& panel, DockPanel::ViewMode mode) {
	const bool startsFloating = mode == DockPanel::ViewMode::Float && panel.mViewMode != DockPanel::ViewMode::Float;
	if (startsFloating && panel.mOpen) {
		const Vec2F current = panel.size();
		const Vec2F floating = { std::min(current.x, kMaximumDraggedSize.x), std::min(current.y, kMaximumDraggedSize.y) };
		panel.setFloatingRect(makeRect(panel.rect().pos + Vec2F(24.0f, 24.0f), floating));
	}

	panel.mViewMode = mode;
	panel.refreshChrome();
	openPanel(panel);
}

DockPanel* DockArea::openPanelOn(DockSide side) const {
	for (DockPanel* panel : mPanels) {
		const bool takesZone = panel->mOpen && !panel->mDragging && panel->mViewMode != DockPanel::ViewMode::Float;
		if (takesZone && panel->mSide == side) return panel;
	}
	return nullptr;
}

StripeButton* DockArea::stripeButton(const DockPanel& panel) const {
	for (const Stripe& stripe : mStripes) {
		if (stripe.panel == &panel) return stripe.button.get();
	}
	return nullptr;
}

RectF DockArea::stripeRect(DockSide side) const {
	return computeGeometry(localRect()).stripes[side == DockSide::Right ? 1 : 0];
}

RectF DockArea::centerRect() const { return computeGeometry(localRect()).center; }

void DockArea::paint(Painter&) {}

void DockArea::paintOverlay(Painter& painter) {
	if (!mDragged) return;

	const Palette& palette = theme().palette;
	const halnf radius = theme().metrics.popupCornerRadius;

	for (DockSide side : { DockSide::Left, DockSide::Right, DockSide::Bottom }) {
		if (indexOf(side) == mDropSide) continue;
		const RectF target = dropTarget(side);
		painter.fillRect(target, withAlpha(palette.text, 0.08f), radius);
		painter.strokeRect(target, withAlpha(palette.mutedText, 0.6f), radius);
	}

	const halnf opacity = mDropPreviewOpacity.value();
	if (opacity <= 0.01f) return;

	const RectF preview = mDropPreview.value();
	painter.fillRect(preview, withAlpha(palette.text, 0.10f * opacity), radius);
	painter.strokeRect(preview, withAlpha(palette.mutedText, 0.9f * opacity), radius, 1.5f);
}

void DockArea::onTick(halnf deltaMs) {
	const bool previewMoving = mDropPreview.advance(deltaMs);
	const bool opacityMoving = mDropPreviewOpacity.advance(deltaMs);

	requestPaint();
	if (!previewMoving && !opacityMoving && !mDragged) stopTicking();
}

bool DockArea::hitTest(Vec2F localPoint) const {
	const Geometry geometry = computeGeometry(localRect());
	return contains(geometry.stripes[0], localPoint) || contains(geometry.stripes[1], localPoint);
}

DockArea::Geometry DockArea::computeGeometry(const RectF& area) const {
	Geometry geometry;
	const halnf thickness = stripeThickness();

	RectF inner = area;
	geometry.stripes[0] = makeRect(area.pos.x, area.pos.y, 0, area.size.y);
	geometry.stripes[1] = makeRect(area.pos.x + area.size.x, area.pos.y, 0, area.size.y);

	if (hasStripe(DockSide::Left)) {
		geometry.stripes[0].size.x = thickness;
		inner.pos.x += thickness;
		inner.size.x -= thickness;
	}
	if (hasStripe(DockSide::Right)) {
		geometry.stripes[1].pos.x -= thickness;
		geometry.stripes[1].size.x = thickness;
		inner.size.x -= thickness;
	}
	geometry.inner = inner;

	const DockPanel* left = openPanelOn(DockSide::Left);
	const DockPanel* right = openPanelOn(DockSide::Right);
	const DockPanel* bottom = openPanelOn(DockSide::Bottom);

	const halnf leftWidth = left ? clampDockedLength(left->mDockedLength, inner.size.x) : 0;
	const halnf rightWidth = right ? clampDockedLength(right->mDockedLength, inner.size.x) : 0;

	geometry.zones[indexOf(DockSide::Left)] = makeRect(inner.pos.x, inner.pos.y, leftWidth, inner.size.y);
	geometry.zones[indexOf(DockSide::Right)] =
		makeRect(inner.pos.x + inner.size.x - rightWidth, inner.pos.y, rightWidth, inner.size.y);

	RectF center = inner;
	const halnf gap = theme().metrics.surfaceGap;
	for (RectF& unused : geometry.gaps)
		unused = makeRect(0, 0, 0, 0);

	if (left && left->mViewMode != DockPanel::ViewMode::Float) {
		geometry.gaps[indexOf(DockSide::Left)] = makeRect(inner.pos.x + leftWidth, inner.pos.y, gap, inner.size.y);
	}
	if (left && left->mViewMode == DockPanel::ViewMode::Docked) {
		center.pos.x += leftWidth + gap;
		center.size.x -= leftWidth + gap;
	}
	if (right && right->mViewMode != DockPanel::ViewMode::Float) {
		geometry.gaps[indexOf(DockSide::Right)] =
			makeRect(inner.pos.x + inner.size.x - rightWidth - gap, inner.pos.y, gap, inner.size.y);
	}
	if (right && right->mViewMode == DockPanel::ViewMode::Docked) center.size.x -= rightWidth + gap;

	const halnf bottomHeight = bottom ? clampDockedLength(bottom->mDockedLength, inner.size.y) : 0;
	geometry.zones[indexOf(DockSide::Bottom)] =
		makeRect(center.pos.x, inner.pos.y + inner.size.y - bottomHeight, center.size.x, bottomHeight);
	if (bottom && bottom->mViewMode != DockPanel::ViewMode::Float) {
		geometry.gaps[indexOf(DockSide::Bottom)] =
			makeRect(center.pos.x, inner.pos.y + inner.size.y - bottomHeight - gap, center.size.x, gap);
	}
	if (bottom && bottom->mViewMode == DockPanel::ViewMode::Docked) center.size.y -= bottomHeight + gap;

	geometry.center = center;
	return geometry;
}

RectF DockArea::panelRect(const DockPanel& panel, const Geometry& geometry, const RectF& area) const {
	if (!panel.mDragging && panel.mViewMode != DockPanel::ViewMode::Float) return geometry.zones[indexOf(panel.mSide)];

	RectF rect = panel.mFloatingRect;
	rect.pos.x = clampValue(
		rect.pos.x, area.pos.x - rect.size.x + kVisibleWhenOffscreen, area.pos.x + area.size.x - kVisibleWhenOffscreen
	);
	rect.pos.y = clampValue(rect.pos.y, area.pos.y, area.pos.y + area.size.y - kVisibleWhenOffscreen);
	return rect;
}

bool DockArea::hasStripe(DockSide side) const {
	for (const DockPanel* panel : mPanels) {
		const bool onLeftRail = panel->mSide == DockSide::Left || panel->mSide == DockSide::Bottom;
		if (side == DockSide::Right ? panel->mSide == DockSide::Right : onLeftRail) return true;
	}
	return false;
}

halnf DockArea::stripeThickness() const { return theme().metrics.controlHeight + kStripeInset * 2; }

halnf DockArea::clampDockedLength(halnf length, halnf extent) const {
	const halnf maximum = extent * kMaximumDockedFraction;
	return clampValue(length, std::min(kMinimumDockedLength, maximum), maximum);
}

RectF DockArea::dropTarget(DockSide side) const {
	const RectF center = computeGeometry(localRect()).inner;
	const halnf size = std::max(kMinimumDropTarget, std::min(center.size.x, center.size.y) * kDropTargetFraction);
	const RectF zone = insetRect(
		[&]() {
			switch (side) {
				case DockSide::Left:
					return makeRect(center.pos.x, center.pos.y, center.size.x * kDropZoneFraction, center.size.y);
				case DockSide::Right:
					return makeRect(
						center.pos.x + center.size.x * (1 - kDropZoneFraction),
						center.pos.y,
						center.size.x * kDropZoneFraction,
						center.size.y
					);
				case DockSide::Bottom:
					return makeRect(
						center.pos.x,
						center.pos.y + center.size.y * (1 - kDropZoneFraction),
						center.size.x,
						center.size.y * kDropZoneFraction
					);
			}
			return center;
		}(),
		kDropZoneInset
	);

	return makeRect(zone.center() - Vec2F(size, size) / 2.0f, Vec2F(size, size));
}

long DockArea::dropSideAt(Vec2F localPoint) const {
	for (DockSide side : { DockSide::Left, DockSide::Right, DockSide::Bottom }) {
		if (contains(dropTarget(side), localPoint)) return indexOf(side);
	}
	return -1;
}

RectF DockArea::zonePreview(const DockPanel& panel, DockSide side) const {
	const Geometry geometry = computeGeometry(localRect());
	const RectF& inner = geometry.inner;
	RectF zone;

	if (side == DockSide::Bottom) {
		const halnf height = clampDockedLength(panel.mDockedLength, inner.size.y);
		zone = makeRect(geometry.center.pos.x, inner.pos.y + inner.size.y - height, geometry.center.size.x, height);
	} else {
		const halnf width = clampDockedLength(panel.mDockedLength, inner.size.x);
		const halnf left = side == DockSide::Left ? inner.pos.x : inner.pos.x + inner.size.x - width;
		zone = makeRect(left, inner.pos.y, width, inner.size.y);
	}

	return insetRect(zone, kDropZoneInset / 2);
}

void DockArea::updateDropPreview(const DockPanel& panel, Vec2F localPointer) {
	const long side = dropSideAt(localPointer);

	if (side >= 0) {
		const RectF zone = zonePreview(panel, (DockSide) side);
		if (mDropSide < 0) mDropPreview.jumpTo(dropTarget((DockSide) side));
		mDropPreview.animateTo(zone);
		mDropPreviewOpacity.animateTo(1);
	} else {
		if (mDropSide >= 0) mDropPreview.animateTo(dropTarget((DockSide) mDropSide));
		mDropPreviewOpacity.animateTo(0);
	}

	mDropSide = side;
	startTicking();
}

void DockArea::closeOthersOnSide(const DockPanel& panel) {
	if (panel.mViewMode == DockPanel::ViewMode::Float) return;

	for (DockPanel* other : mPanels) {
		const bool sharesZone =
			other != &panel && other->mSide == panel.mSide && other->mViewMode != DockPanel::ViewMode::Float;
		if (sharesZone && other->mOpen) hidePanel(*other);
	}
}

void DockArea::rebuildStripes() {
	for (Stripe& stripe : mStripes)
		removeChild(*stripe.button);
	mStripes.clear();

	for (DockPanel* panel : mPanels) {
		auto button = std::make_unique<StripeButton>(panel->title(), panel->mIcon);
		button->setOnActivate([this, panel]() { togglePanel(*panel); });

		addChild(*button);
		mStripes.push_back({ std::move(button), panel });
	}

	refreshStripeSelection();
	refreshEdgeHandles();
	requestLayout();
}

void DockArea::refreshEdgeHandles() {
	for (DockSide side : { DockSide::Left, DockSide::Right, DockSide::Bottom }) {
		const DockPanel* panel = openPanelOn(side);
		ResizeHandle& handle = *mEdgeHandles[indexOf(side)];
		handle.setVisible(panel && panel->mViewMode != DockPanel::ViewMode::Float);
		handle.bringToFront();
	}
}

void DockArea::refreshStripeSelection() {
	for (Stripe& stripe : mStripes)
		stripe.button->setSelected(stripe.panel->mOpen);
}

void DockArea::panelDragStarted(DockPanel& panel) {
	const RectF current = panel.rect();
	const Vec2F size =
		panel.mViewMode == DockPanel::ViewMode::Float
			? panel.mFloatingRect.size
			: Vec2F(std::min(current.size.x, kMaximumDraggedSize.x), std::min(current.size.y, kMaximumDraggedSize.y));

	panel.mGrabOffset.x = std::min(panel.mGrabOffset.x, size.x - kVisibleWhenOffscreen);
	panel.mFloatingRect = makeRect(current.pos, size);
	panel.bringToFront();

	mDragged = &panel;
	mDropSide = -1;
	mDropPreviewOpacity.jumpTo(0);
	refreshEdgeHandles();
	startTicking();
	requestLayout();
}

void DockArea::panelDragged(DockPanel& panel, Vec2F windowPointer) {
	const Vec2F local = toLocal(windowPointer);

	panel.mFloatingRect.pos = local - panel.mGrabOffset;
	updateDropPreview(panel, local);
	requestLayout();
}

void DockArea::panelDragEnded(DockPanel& panel, Vec2F windowPointer) {
	panelDragged(panel, windowPointer);

	const long side = mDropSide;
	mDragged = nullptr;
	mDropSide = -1;
	mDropPreviewOpacity.animateTo(0);

	if (side >= 0) {
		dockPanel(panel, (DockSide) side);
		return;
	}

	panel.mViewMode = DockPanel::ViewMode::Float;
	panel.refreshChrome();
	openPanel(panel);
}

RectF DockArea::openingRect(const DockPanel& panel) const {
	const RectF target = panelRect(panel, computeGeometry(localRect()), localRect());

	if (panel.mViewMode == DockPanel::ViewMode::Float) {
		const Vec2F size = target.size * kFloatOpeningScale;
		return makeRect(target.center() - size / 2.0f, size);
	}

	switch (panel.mSide) {
		case DockSide::Left: return makeRect(target.pos.x, target.pos.y, 0, target.size.y);
		case DockSide::Right: return makeRect(target.pos.x + target.size.x, target.pos.y, 0, target.size.y);
		case DockSide::Bottom: return makeRect(target.pos.x, target.pos.y + target.size.y, target.size.x, 0);
	}
	return target;
}

void DockArea::beginDirectResize(DockPanel& panel) {
	panel.setGeometryAnimated(false);
	if (mCenter && panel.mViewMode == DockPanel::ViewMode::Docked) mCenter->setGeometryAnimated(false);
}

void DockArea::endDirectResize(DockPanel& panel) {
	panel.setGeometryAnimated(true);
	if (mCenter) mCenter->setGeometryAnimated(true);
	if (panel.mViewMode == DockPanel::ViewMode::Undocked && panel.mOpen && ui()) ui()->setActiveWidget(&panel);
}

void DockArea::floatResized(DockPanel& panel, Vec2F localBottomRight) {
	panel.setFloatingRect(makeRect(panel.mFloatingRect.pos, localBottomRight - panel.mFloatingRect.pos));
}

void DockArea::edgeDragged(DockSide side, halnf windowCoordinate) {
	DockPanel* panel = openPanelOn(side);
	if (!panel) return;

	const RectF inner = computeGeometry(localRect()).inner;
	const halnf halfGap = theme().metrics.surfaceGap / 2;
	const Vec2F origin = windowPosition();

	switch (side) {
		case DockSide::Left: panel->setDockedLength(windowCoordinate - origin.x - inner.pos.x - halfGap); break;
		case DockSide::Right:
			panel->setDockedLength(inner.pos.x + inner.size.x - (windowCoordinate - origin.x) - halfGap);
			break;
		case DockSide::Bottom:
			panel->setDockedLength(inner.pos.y + inner.size.y - (windowCoordinate - origin.y) - halfGap);
			break;
	}
}

void DockArea::panelLostActivity(DockPanel& panel) {
	if (panel.mViewMode != DockPanel::ViewMode::Undocked || !panel.mOpen || panel.mDragging) return;

	Widget* active = ui() ? ui()->activeWidget() : nullptr;
	const Button* button = stripeButton(panel);
	if (active && button && button->isSelfOrAncestorOf(*active)) return;
	const ResizeHandle& handle = *mEdgeHandles[indexOf(panel.mSide)];
	if (active && handle.isSelfOrAncestorOf(*active)) return;

	hidePanel(panel);
}
