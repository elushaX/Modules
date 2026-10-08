#pragma once

#include "Selection.hpp"
#include "Brush.hpp"
#include "History.hpp"
#include "Layer.hpp"
#include "Engine.hpp"
#include "StrokePainter.hpp"
#include "StrokeTrace.hpp"

#include "Map.hpp"

#include <atomic>
#include <memory>
#include <functional>
#include <optional>
#include <vector>

namespace tp::sketch2d {

	struct HueColor {
		halnf hueDegrees = 0;
		halnf saturation = 0;
		halnf value = 1;

		[[nodiscard]] RGBA rgba(halnf alpha = 1.0f) const;
		[[nodiscard]] bool operator==(const HueColor& other) const = default;
	};

	struct CanvasBackground {
		static constexpr halnf kDefaultValue = 0.12f;

		HueColor color = { 0, 0, kDefaultValue };
		bool transparent = false;

		[[nodiscard]] RGBA rgba() const { return color.rgba(transparent ? 0.0f : 1.0f); }
		[[nodiscard]] bool operator==(const CanvasBackground& other) const = default;
	};

	struct CanvasFrame {
		static constexpr int4 kCoordinateLimit = 0x200000;
		static constexpr int4 kMaximumSide = 50000;
		static constexpr int4 kDefaultHalfSize = 250;

		int4 left = 0;
		int4 top = 0;
		int4 right = 0;
		int4 bottom = 0;
		bool infinite = true;

		[[nodiscard]] bool isEmpty() const { return right <= left || bottom <= top; }
		[[nodiscard]] int4 width() const { return right - left; }
		[[nodiscard]] int4 height() const { return bottom - top; }
		[[nodiscard]] RectF rect() const { return { (halnf) left, (halnf) top, (halnf) width(), (halnf) height() }; }
		void clamp();
		[[nodiscard]] bool operator==(const CanvasFrame& other) const = default;
	};

	struct BucketSettings {
		halnf threshold = 0.1f;
		bool onlyAlpha = false;
		bool allLayers = true;
		int4 spread = 1;
	};

	enum class BucketResult { Filled, Leak, Empty, TooBig, Locked };

	enum class GradientShape { Linear, Radial };

	struct GradientSettings {
		GradientShape shape = GradientShape::Linear;
		bool toTransparent = false;
		bool eraser = false;
	};

	enum class GradientResult { Drawn, NoFrame, Locked };

	struct LayerTransform {
		Vec2F pivot = { 0, 0 };
		Vec2F scale = { 1, 1 };
		halnf rotation = 0;
		Vec2F offset = { 0, 0 };

		[[nodiscard]] Vec2F apply(Vec2F point) const;
		[[nodiscard]] Vec2F invert(Vec2F point) const;
		[[nodiscard]] bool isIdentity() const;
	};

	enum class CropResult { Cropped, NoRegion, AlphaLocked, Unchanged };

	enum class AdjustmentKind { BrightnessExposure, HueSaturation, ColorBalance, Colorize, Alpha };

	struct Adjustment {
		AdjustmentKind kind = AdjustmentKind::BrightnessExposure;
		halnf brightness = 0;
		halnf exposure = 0;
		halnf hueDegrees = 0;
		halnf saturation = 0;
		halnf lightness = 0;
		halnf red = 0;
		halnf green = 0;
		halnf blue = 0;
		halnf alpha = 0;

		void apply(halnf rgba[4]) const;
		[[nodiscard]] bool isNeutral() const;
	};

	struct MoveSettings {
		bool duplicate = false;
	};

	struct DocumentState {
		std::vector<Layer::State> layers;
		ualni active = 0;
		CanvasBackground background;
		CanvasFrame frame;
		Selection selection;
	};

	struct SmudgePatch {
		int4 diameter = 0;
		std::vector<halnf> rgba;

		[[nodiscard]] bool isEmpty() const { return diameter == 0; }
		[[nodiscard]] halnf* at(int4 x, int4 y) { return rgba.data() + ((size_t) y * diameter + x) * 4; }
		[[nodiscard]] const halnf* at(int4 x, int4 y) const { return rgba.data() + ((size_t) y * diameter + x) * 4; }
		void clear() {
			diameter = 0;
			rgba.clear();
		}
	};

	class Document {
	public:
		Document();
		~Document();

		void reset();

		Document(const Document&) = delete;
		Document& operator=(const Document&) = delete;

		void pan(const Vec2F& screenDelta);
		void zoomAt(const Vec2F& screenPoint, halnf factor);
		void rotateAt(const Vec2F& screenPoint, halnf radians);
		void setRotationAt(const Vec2F& screenPoint, halnf radians);
		void resetView();
		[[nodiscard]] const Vec2F& panWorld() const { return mPan; }
		void setPanWorld(const Vec2F& pan) { mPan = pan; }
		[[nodiscard]] halnf zoom() const { return mZoom; }
		void setZoom(halnf z) { mZoom = z < 0.02f ? 0.02f : (z > 64.0f ? 64.0f : z); }
		[[nodiscard]] halnf rotation() const { return mRotation; }
		void setRotation(halnf radians);
		[[nodiscard]] bool isRotated() const { return mRotation != 0; }

		[[nodiscard]] Vec2F screenToWorld(const Vec2F& screen) const;
		[[nodiscard]] Vec2F worldToScreen(const Vec2F& world) const;
		[[nodiscard]] Vec2F worldToView(const Vec2F& world) const { return (world - mPan) * mZoom; }
		[[nodiscard]] Vec2F screenToView(const Vec2F& screen) const;
		[[nodiscard]] RectF viewBoundsOfScreen(Vec2F screenSize) const;

		[[nodiscard]] ualni tileCount() const;

		template <typename Fn>
		void forEachVisibleTile(Fn&& fn) const {
			for (const auto& layer : mLayers) {
				if (!layer->visible()) continue;
				layer->forEachTile(fn);
			}
		}

		template <typename Fn>
		void forEachTile(Fn&& fn) const {
			for (const auto& layer : mLayers)
				layer->forEachTile(fn);
		}

		void setPaintWorkers(size_t count);
		static constexpr double kEngineSliceMs = 8.0;
		static constexpr halnf kPreviewDabStep = 0.25f;
		static constexpr halnf kPreviewPathStep = 0.5f;
		void setEngineThreads(size_t count) { mEngine.setThreads(count); }
		[[nodiscard]] Engine& engine() const { return mEngine; }
		[[nodiscard]] size_t paintWorkers() const { return mPainter.workers(); }
		void submitPaint();
		void settlePaint() const;
		[[nodiscard]] size_t deferredDabs() const { return mOpenTrace ? mOpenTrace->recordedDabs() : 0; }
		[[nodiscard]] bool hasOpenPathRange() const { return mPathRangeOpen; }
		[[nodiscard]] int4 previewLevel() const { return mPreviewLevel; }

		[[nodiscard]] RGBA sampleColor(Vec2F world, bool allLayers) const;
		static constexpr int4 kBucketMaximumPixels = 16 * 1024 * 1024;
		static constexpr int4 kBucketOpenMargin = 256;
		BucketResult bucketFill(Vec2F world, const RGBA& color, const BucketSettings& settings);
		BucketResult magicWand(Vec2F world, const BucketSettings& settings, SelectionMode mode);
		struct FloodRegion {
			BucketResult result = BucketResult::Empty;
			int4 left = 0;
			int4 top = 0;
			int4 width = 0;
			int4 height = 0;
			std::vector<uint1> mask;
		};
		[[nodiscard]] FloodRegion floodRegion(Vec2F world, const BucketSettings& settings, bool refuseLeaks = true) const;
		GradientResult
		drawGradient(Vec2F fromWorld, Vec2F toWorld, const RGBA& start, const RGBA& end, const GradientSettings& settings);
		[[nodiscard]] static halnf gradientPosition(Vec2F point, Vec2F from, Vec2F to, GradientShape shape);
		bool beginMove(bool duplicate);
		bool beginTransform();
		void previewTransform(const LayerTransform& transform);
		bool finishTransform();
		void cancelTransform();
		[[nodiscard]] bool isTransforming() const { return mTransformSource.has_value(); }
		[[nodiscard]] bool transformBounds(int4& left, int4& top, int4& right, int4& bottom) const;
		bool flipActiveLayer(bool horizontal);
		[[nodiscard]] const Selection& selection() const { return mSelection; }
		Selection& editSelection() { return mSelection; }
		[[nodiscard]] bool hasSelection() const { return mSelection.isActive(); }
		bool deselect();
		bool selectAllCanvas();
		bool selectAllAsRectangle();
		bool selectAllAsLasso();
		enum class PixelFilter { Invert, Desaturate, ValueToAlpha, ValueToAlphaInverted };
		enum class FilterResult { Done, Empty, Locked, AlphaLocked };
		FilterResult applyFilter(PixelFilter filter);
		struct PixelClip {
			int4 left = 0;
			int4 top = 0;
			int4 width = 0;
			int4 height = 0;
			std::vector<uint1> pixels;
			[[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; }
		};
		[[nodiscard]] PixelClip copyPixels() const;
		Layer* pasteAsLayer(const PixelClip& clip, const std::string& name);
		FilterResult applyPixelFunction(const std::function<void(halnf rgba[4])>& function, bool changesAlpha);
		[[nodiscard]] static halnf pixelValue(const halnf rgba[4]);
		bool beginAdjustment();
		void previewAdjustment(const Adjustment& adjustment);
		bool finishAdjustment();
		void cancelAdjustment();
		[[nodiscard]] bool isAdjusting() const { return mAdjustSource.has_value(); }
		bool invertSelection();
		enum class SelectionEditResult { Done, NoSelection, Inverted, AlphaLocked, Locked };
		SelectionEditResult fillSelection(const RGBA& color);
		SelectionEditResult fillSelectionUnderneath(const RGBA& color);
		SelectionEditResult clearSelection();
		CropResult cropTo(int4 left, int4 top, int4 right, int4 bottom);
		[[nodiscard]] bool layerBounds(const Layer& layer, int4& left, int4& top, int4& right, int4& bottom) const;
		void moveTo(int4 dx, int4 dy);
		bool finishMove();
		void cancelMove();
		[[nodiscard]] bool isMoving() const { return mMoveSource.has_value(); }
		[[nodiscard]] size_t paintBacklog() const { return mPainter.backlog() + mEngine.pending(); }
		[[nodiscard]] double takePaintWorkerMs() { return mPainter.takeWorkerMs(); }

		template <typename Fn>
		void whilePaintPaused(Fn&& fn) const {
			mPainter.whilePaused([&]() { mEngine.whilePaused(fn); });
		}

		[[nodiscard]] std::string strokeRefusal(const Brush& brush) const;
		void beginPathStroke(const Brush& brush, StrokeMirror mirror, const StrokePath::Sample& first);
		void addPathSample(const StrokePath::Sample& sample);
		void advancePathStroke(bool finished);
		void stampBrushDab(const Vec2F& worldCenter, const Brush& brush, halnf pressure = 1.0f);
		void stampDab(const Vec2F& worldCenter, const Brush& brush, const DabShape& dab);
		void restoreStrokeBase();
		void smudgeDab(const Vec2F& worldCenter, const Brush& brush, const DabShape& dab);
		void finishStroke(const std::string& label = "Paint");
		void discardStroke();

		static constexpr size_t kMaximumLayerNameLength = 63;
		static constexpr size_t kMaximumLayers = 256;

		Layer* addLayer(const std::string& name);
		Layer* duplicateLayer(const Layer& source);
		void removeActiveLayer();
		bool renameLayer(Layer& layer, const std::string& name);
		bool moveActiveLayer(int4 offset);
		[[nodiscard]] bool canAddLayer() const { return mLayers.size() < kMaximumLayers; }
		[[nodiscard]] std::string uniqueLayerName(const std::string& name) const;
		[[nodiscard]] size_t indexOf(const Layer& layer) const;
		[[nodiscard]] Layer* activeLayer() const { return mActive; }
		void setActiveLayer(Layer* layer);
		bool selectOnly(Layer& layer);
		bool selectAdjacent(int4 offset);
		bool selectAllLayers();
		bool toggleLayerSelection(Layer& layer);
		bool selectRangeTo(Layer& layer);
		[[nodiscard]] size_t selectedCount() const;
		[[nodiscard]] const std::vector<std::unique_ptr<Layer>>& layers() const { return mLayers; }
		[[nodiscard]] ualni layerCount() const { return mLayers.size(); }
		[[nodiscard]] std::string nextLayerName() const { return uniqueLayerName("Layer"); }

		void clearActiveLayer();
		[[nodiscard]] std::vector<Layer*> mergeCandidates() const;
		bool mergeLayers(const std::vector<Layer*>& layers);
		bool mergeVisibleLayers();
		bool bakeOpacity(Layer& layer);
		void setLayerVisible(Layer& layer, bool visible);
		void setLayerOpacity(Layer& layer, halnf opacity);

		[[nodiscard]] std::vector<Layer*> targetLayers() const;
		[[nodiscard]] Layer* layerBelow(const Layer& layer) const;
		[[nodiscard]] Layer* layerAbove(const Layer& layer) const;
		[[nodiscard]] bool canEdit(const Layer& layer) const { return !layer.locked(); }

		bool toggleVisible();
		bool toggleClip();
		bool toggleLock();
		bool toggleAlphaLock();
		bool cycleBlend(int4 direction);
		bool resetBlend();
		bool setBlend(BlendMode blend);
		bool changeOpacity(halnf delta);
		bool setOpacity(halnf opacity);

		[[nodiscard]] const CanvasFrame& frame() const { return mFrame; }
		[[nodiscard]] bool isInfinite() const { return mFrame.infinite; }
		bool setInfinite(bool infinite, Vec2F viewCenterWorld = { 0, 0 });
		[[nodiscard]] CanvasFrame proposedFrame(Vec2F viewCenterWorld) const;
		bool setFrame(int4 left, int4 top, int4 right, int4 bottom);
		[[nodiscard]] bool contentBounds(int4& left, int4& top, int4& right, int4& bottom) const;

		[[nodiscard]] const CanvasBackground& background() const { return mBackground; }
		bool setBackground(const CanvasBackground& background);
		void previewBackground(const CanvasBackground& background) { mBackground = background; }

		[[nodiscard]] DocumentState state() const;
		[[nodiscard]] DocumentState stateWithoutSettling() const;
		void restore(const DocumentState& state);
		void load(const DocumentState& state, const std::string& label);
		void commit(const std::string& label, const std::string& mergeKey = {});

		[[nodiscard]] History<DocumentState>& history() { return mHistory; }
		[[nodiscard]] const History<DocumentState>& history() const { return mHistory; }
		bool undo();
		bool redo();
		bool jumpToHistory(size_t index);

	private:
		Selection mSelection;
		std::shared_ptr<const Selection> mStrokeSelection;
		std::optional<Layer::State> mMoveSource;
		std::optional<Layer::State> mTransformSource;
		std::optional<Layer::State> mAdjustSource;
		bool mAdjustChanged = false;
		int4 mTransformBounds[4] = { 0, 0, 0, 0 };
		bool mTransformChanged = false;
		bool mMoveDuplicate = false;
		bool mMoveSelectionOnly = false;
		Selection mMoveSelection;
		int4 mMoveX = 0;
		int4 mMoveY = 0;
		[[nodiscard]] Layer* layerWithId(ualni id) const;
		void writeSmudgePatch(const SmudgePatch& patch, int4 originX, int4 originY, bool keepAlpha, bool linear);

		std::vector<std::unique_ptr<Layer>> mLayers;
		Layer* mActive = nullptr;
		CanvasBackground mBackground;
		CanvasFrame mFrame;
		ualni mNextLayerId = 1;

		mutable StrokePainter mPainter;
		mutable Engine mEngine;
		Layer* mStrokeLayer = nullptr;
		Map<ualni, TileCoord> mStrokeTiles;
		Map<ualni, std::shared_ptr<StrokeMask>> mStrokeMasks;
		mutable Map<ualni, std::shared_ptr<StrokeMask>> mPreviewMasks;
		mutable Map<ualni, std::shared_ptr<void>> mPendingSlots;
		mutable Map<ualni, std::shared_ptr<void>> mStrokeCoverage;
		struct RangeStart {
			std::optional<StrokePath::Walk> walk = StrokePath::Walk{};
			std::shared_ptr<StrokeTrace> after;
		};
		mutable std::shared_ptr<StrokeTrace> mOpenTrace;
		StrokeStyle mPathStyle;
		StrokePath mStrokePath;
		std::optional<StrokePath::Walk> mDenseWalk = StrokePath::Walk{};
		StrokePath::Walk mPreviewWalk;
		mutable RangeStart mRangeStart;
		mutable bool mPathRangeOpen = false;
		mutable halnf mPathRangeTo = 0;
		bool mPathActive = false;
		mutable std::shared_ptr<std::atomic<bool>> mStrokeCancelled;
		StrokeMask* strokeMask(const TileCoord& coord);
		std::vector<std::shared_ptr<ualni>> mPendingStrokeEntries;
		int4 mPreviewLevel = -1;
		std::shared_ptr<const DabWork> mLastPreviewDab;
		void stagePreviewDab(const TracedDab& dab);
		void paintPreviewDab(const TracedDab& dab);
		[[nodiscard]] int4 deferredLevel(bool marker) const;
		void usePreviewLevel(int4 level);
		[[nodiscard]] DabTarget strokeTarget();
		[[nodiscard]] StrokeTrace& openTrace();
		void stageEagerDab(const TracedDab& dab);
		void advanceDensePath(halnf end);
		void advancePreviewPath(int4 level, halnf end);
		void resetPathStroke();
		void stageBasePreview(const DabWork& work, const TileCoord& coord, int4 minX, int4 minY, int4 maxX, int4 maxY);
		[[nodiscard]] bool previewNeedsDab(const DabWork& work, int4 level);
		void flushDeferredDabs() const;
		void closePathRange() const;
		void releaseFinishedPreviews() const;
		void dropOpenStroke();
		[[nodiscard]] bool isLayerEmpty(const Layer& layer) const;
		SmudgePatch mSmudgeCarry;

		History<DocumentState> mHistory;

		Vec2F mPan = { 0, 0 };
		halnf mZoom = 1.0f;
		halnf mRotation = 0;
	};
}
