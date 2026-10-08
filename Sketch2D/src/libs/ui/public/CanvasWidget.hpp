#pragma once

#include "StrokePath.hpp"

#include "Event.hpp"
#include "SpringAnimations.hpp"

#include <array>
#include <vector>
#include <functional>
#include <optional>
#include <string>

#include "BrushCursor.hpp"
#include "ChannelView.hpp"
#include "Document.hpp"
#include "ViewSettings.hpp"
#include "Painter.hpp"
#include "Stabilizer.hpp"
#include "Widget.hpp"

#include <string>

namespace tp::sketch2d {

	[[nodiscard]] inline const char* adjustableName(BrushParameter parameter) {
		switch (parameter) {
			case BrushParameter::Size: return "size";
			case BrushParameter::Opacity: return "opacity";
			case BrushParameter::Flow: return "flow";
			case BrushParameter::Angle: return "angle";
			default: return "value";
		}
	}

	class ViewportRenderer {
	public:
		virtual ~ViewportRenderer() = default;

		virtual void render(Document& document, const ViewSettings& settings, Vec2F viewportSize) = 0;
		[[nodiscard]] virtual ui::ImageId image() const = 0;

		virtual void setDebugOverlay(bool enabled) = 0;
		[[nodiscard]] virtual bool debugOverlay() const = 0;
		[[nodiscard]] virtual std::string statistics() const { return {}; }
		struct LegendEntry {
			std::string label;
			RGBA color;
		};
		[[nodiscard]] virtual std::vector<LegendEntry> debugLegend() const { return {}; }
		[[nodiscard]] virtual bool isCatchingUp() const { return false; }
	};

	class CanvasWidget : public ui::Widget {
	public:
		static constexpr halnf kRotationSnapRadians = 0.07f;

		CanvasWidget(Document& document, ViewportRenderer& renderer, Brush& brush);

		[[nodiscard]] ViewSettings& settings() { return mSettings; }

		[[nodiscard]] Document& document() const { return mDocument; }
		[[nodiscard]] ViewportRenderer& renderer() const { return mRenderer; }
		[[nodiscard]] bool isPainting() const { return mStroke == Stroke::Painting; }
		[[nodiscard]] bool isWaitingForFinger() const { return mStroke == Stroke::FingerPending; }
		static constexpr halnf kFingerCommitMs = 80;
		static constexpr halnf kFingerCommitTravel = 24;
		static constexpr halnf kFingerGestureGraceMs = 250;
		[[nodiscard]] bool isPanning() const { return mStroke == Stroke::Panning; }
		[[nodiscard]] bool isAdjustingBrush() const { return mStroke == Stroke::Adjusting; }

		static constexpr halnf kSizeDoublingPixels = 100.0f;
		static constexpr halnf kAngleDegreesPerPixel = 1.0f;
		static constexpr halnf kAmountPerPixel = 0.005f;

		std::function<std::optional<BrushParameter>()> heldAdjustment;
		std::function<void()> onBrushAdjusted;
		std::function<void()> onStrokeFinished;
		std::function<void(const RGBA&)> onColorPicked;
		std::function<bool()> heldPicker;

		static constexpr halnf kZoomDoublingPixels = 150.0f;
		static constexpr halnf kZoomClickFactor = 2.0f;
		static constexpr halnf kToolClickPixels = 4.0f;
		void resetSymmetry();
		void resetPerspective();
		void armPerspectiveMove() { mPerspectiveMoveArmed = true; }
		[[nodiscard]] bool isPerspectiveMoveArmed() const { return mPerspectiveMoveArmed; }
		[[nodiscard]] bool isMovingSymmetry() const { return mStroke == Stroke::MovingSymmetry; }
		void armSymmetryMove() { mSymmetryMoveArmed = true; }
		[[nodiscard]] bool isSymmetryMoveArmed() const { return mSymmetryMoveArmed; }
		static constexpr halnf kHoldStillPixels = 3.0f;
		static constexpr halnf kMinimumLinePixels = 6.0f;
		[[nodiscard]] bool isDrawingLine() const { return mStroke == Stroke::Painting && mLineLocked; }

		static constexpr halnf kTransformHandleRadius = 18.0f;
		static constexpr halnf kMinimumTransformScale = 0.01f;

		void setTool(CanvasTool tool);
		bool startTransform();
		void applyTransform();
		void cancelTransform();
		void flipTransform(bool horizontal);
		[[nodiscard]] bool isTransforming() const { return mDocument.isTransforming(); }
		[[nodiscard]] const LayerTransform& pendingTransform() const { return mTransform; }
		void setPendingTransform(const LayerTransform& transform);
		[[nodiscard]] std::array<Vec2F, 4> transformCorners() const;
		std::function<void(bool)> onTransformActive;
		std::function<void()> onTransformRefused;
		std::function<void(bool)> onTransformApplied;
		std::function<void(CropResult)> onCrop;
		std::function<void()> onSelectionChanged;
		std::function<void(BucketResult)> onWand;
		[[nodiscard]] bool isSelecting() const { return mStroke == Stroke::Selecting; }
		[[nodiscard]] const std::vector<Vec2F>& polygonPoints() const { return mPolygon; }
		void finishPolygon();
		void cancelPolygon();
		static constexpr halnf kPolygonCloseRadius = 12.0f;
		static constexpr halnf kAntDashPixels = 4.0f;
		[[nodiscard]] bool isCropping() const { return mStroke == Stroke::Cropping; }
		[[nodiscard]] RectF cropRect() const;
		[[nodiscard]] CanvasTool tool() const { return mTool; }
		void setPickerTool(bool active) { setTool(active ? CanvasTool::Picker : CanvasTool::Paint); }
		[[nodiscard]] bool pickerTool() const { return mTool == CanvasTool::Picker; }
		[[nodiscard]] bool isZooming() const { return mStroke == Stroke::Zooming; }
		[[nodiscard]] bool isRotating() const { return mStroke == Stroke::Rotating; }
		std::function<void()> onViewChanged;
		std::function<void(BucketResult)> onBucket;
		std::function<void(GradientResult)> onGradient;
		std::function<RGBA()> gradientEndColor;
		[[nodiscard]] bool isDrawingGradient() const { return mStroke == Stroke::Gradient; }
		[[nodiscard]] bool isMovingPixels() const { return mStroke == Stroke::Moving; }
		std::function<void(bool)> onMoveFinished;
		[[nodiscard]] Vec2F gradientStart() const { return mToolOrigin; }
		[[nodiscard]] bool isPicking() const { return mStroke == Stroke::Picking; }
		[[nodiscard]] const RGBA& pickedColor() const { return mPicked; }
		[[nodiscard]] RGBA sampleAt(Vec2F local) const;
		void setAdjustTool(std::optional<BrushParameter> parameter, bool once = false) {
			mAdjustTool = parameter;
			mAdjustOnce = once;
			refreshCursor();
		}
		[[nodiscard]] std::optional<BrushParameter> adjustTool() const { return mAdjustTool; }
		void setBrushPreview(std::optional<BrushParameter> parameter);
		[[nodiscard]] std::optional<BrushParameter> brushPreview() const { return mBrushPreview; }
		[[nodiscard]] BrushCursorShape brushPreviewShape() const;
		[[nodiscard]] RectF brushPreviewBackdrop() const;
		[[nodiscard]] std::string brushPreviewText() const;
		[[nodiscard]] std::string adjustmentText() const;
		[[nodiscard]] static halnf adjustedValue(BrushParameter parameter, halnf start, halnf dragPixels);

		std::function<void(const std::string&)> onRefused;

		[[nodiscard]] halnf activeStabilizer() const;
		void cancelStroke();
		[[nodiscard]] const Stabilizer& stabilizer() const { return mStabilizer; }
		[[nodiscard]] bool isStabilizerRopeVisible() const;
		[[nodiscard]] bool isCatchingUp() const;

		void changeView(const std::function<void(Document&)>& change);
		[[nodiscard]] bool isViewMoving() const { return mViewMoving; }
		[[nodiscard]] static SpringParameters viewSpring(halnf stiffness);

		std::function<void(int4)> onFingerTap;

		[[nodiscard]] bool isHoveringCanvas() const { return mHoverInside; }
		[[nodiscard]] Vec2F hoverPosition() const { return mHoverLocal; }
		[[nodiscard]] BrushCursorShape brushCursor() const;
		[[nodiscard]] CursorShape systemCursor() const;
		void refreshCursor();

	protected:
		void onEvent(ui::Event& event) override;
		void paint(ui::Painter& painter) override;
		void paintOverlay(ui::Painter& painter) override;
		void onTick(halnf deltaMs) override;
		void onHoverChanged() override;

	private:
		enum class Stroke {
			None,
			Painting,
			Panning,
			Adjusting,
			Picking,
			Zooming,
			Rotating,
			Gradient,
			Moving,
			Transforming,
			Cropping,
			Selecting,
			MovingSymmetry,
			MovingPerspective,
			FingerPending
		};
		struct FingerPoint {
			Vec2F position = { 0, 0 };
			halnf pressure = 1;
		};
		void commitFingerStroke();
		void walkPath(bool finished);
		[[nodiscard]] StrokeMirror strokeMirror() const;
		enum class TransformDrag { Move, Scale, Rotate };

		[[nodiscard]] bool wantsPan(const ui::Event& event) const;
		[[nodiscard]] static halnf pressureOf(const ui::Event& event);
		void beginPainting(Vec2F local, halnf pressure);
		void beginStroke(Stroke stroke, Vec2F local);
		void continueStroke(Vec2F local, halnf pressure);
		void endStroke();
		void discardStroke();
		void handleGesture(const ui::Event& event);
		void paintSegment(Vec2F fromWorld, Vec2F toWorld, halnf fromPressure, halnf toPressure);
		void paintStabilized();
		void paintStabilizerRope(ui::Painter& painter) const;
		void paintBrushCursor(ui::Painter& painter) const;
		void paintAdjustment(ui::Painter& painter) const;
		void paintBrushPreview(ui::Painter& painter) const;
		[[nodiscard]] std::optional<BrushParameter> pendingAdjustment() const;
		void beginAdjusting(BrushParameter parameter, Vec2F local);
		void trackHover(const ui::Event& event);
		void advanceView(halnf deltaMs);
		[[nodiscard]] bool showsSpringView() const;

		struct ViewTarget {
			Vec2F pan;
			halnf zoom = 1;
			halnf rotation = 0;
		};
		ViewTarget mViewTarget;
		ViewTarget mShownView;
		SpringVec2 mPanSpring;
		SpringValue mZoomSpring;
		SpringValue mRotationSpring;
		bool mViewMoving = false;

		Document& mDocument;
		ViewportRenderer& mRenderer;
		Brush& mBrush;
		ViewSettings mSettings;

		Stroke mStroke = Stroke::None;
		Vec2F mLastLocal = { 0, 0 };
		halnf mDabCarry = 0;
		Stabilizer mStabilizer;
		StrokePoint mLastPainted;
		StrokePoint mStrokeStart;
		Vec2F mHoldAnchor = { 0, 0 };
		halnf mHoldMs = 0;
		FingerPoint mFingerStart;
		std::vector<FingerPoint> mFingerPath;
		halnf mFingerWaitMs = 0;
		bool mGestureFingersDown = false;
		halnf mSinceGestureMs = kFingerGestureGraceMs;
		bool mLineLocked = false;
		void redrawLine(Vec2F local);
		void stamp(Vec2F world, halnf pressure);
		void paintSymmetry(ui::Painter& painter) const;
		Vec2F mSymmetryGrab = { 0, 0 };
		bool mSymmetryMoveArmed = false;
		bool mPerspectiveMoveArmed = false;
		int4 mPerspectivePoint = 0;
		void paintPerspective(ui::Painter& painter) const;
		void redrawEllipse(Vec2F local, bool circle);
		bool mEllipseStroke = false;
		static constexpr int4 kEllipseSegments = 96;
		Vec2F mHoverLocal = { 0, 0 };
		bool mHoverInside = false;
		bool mHoverStylus = false;
		std::optional<BrushParameter> mAdjustTool;
		std::optional<BrushParameter> mBrushPreview;
		CanvasTool mTool = CanvasTool::Paint;
		Vec2F mToolOrigin = { 0, 0 };
		halnf mToolStartZoom = 1;
		halnf mToolStartAngle = 0;
		halnf mToolStartRotation = 0;
		bool mToolMoved = false;
		bool mZoomOut = false;
		RGBA mPicked = { 0, 0, 0, 0 };
		RGBA mPickStart = { 0, 0, 0, 0 };
		Vec2F mPickLocal = { 0, 0 };

		[[nodiscard]] bool wantsPicker() const;
		void pickAt(Vec2F local);
		void paintPicker(ui::Painter& painter) const;
		void fillAt(Vec2F local);
		void finishGradient(Vec2F local);
		void finishSelection(Vec2F local);
		void paintSelection(ui::Painter& painter) const;
		[[nodiscard]] SelectionMode selectionMode(const ui::Modifiers& modifiers) const;
		SelectionMode mSelectMode = SelectionMode::Replace;
		std::vector<Vec2F> mPolygon;
		std::vector<Vec2F> mLassoPath;
		halnf mAntPhase = 0;
		void beginTransformDrag(Vec2F local, bool proportional);
		void continueTransformDrag(Vec2F local);
		void paintTransform(ui::Painter& painter) const;
		[[nodiscard]] Vec2F transformCenterWorld(const LayerTransform& transform) const;
		[[nodiscard]] static LayerTransform rebased(const LayerTransform& transform, Vec2F pivot);
		LayerTransform mTransform;
		LayerTransform mDragTransform;
		TransformDrag mTransformDrag = TransformDrag::Move;
		int4 mScaleCorner = 0;
		bool mScaleProportional = false;
		Vec2F mDragStartWorld = { 0, 0 };
		bool mAdjustOnce = false;
		BrushParameter mAdjusting = BrushParameter::Size;
		Vec2F mAdjustOrigin = { 0, 0 };
		halnf mAdjustStart = 0;
	};
}
