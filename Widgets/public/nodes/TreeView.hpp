#pragma once

#include "Animation.hpp"
#include "FullViewFrame.hpp"
#include "InputCodes.hpp"
#include "Layout.hpp"
#include "Painter.hpp"
#include "ScrollPhysics.hpp"
#include "TreeLayout.hpp"
#include "Widget.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tp::ui {

	class TreeView;

	// One card in a TreeView. It is an ordinary widget: the header (title, badge) is painted by the card itself and
	// anything added to content() is laid out in the card body and receives input like any other widget.
	class TreeCard : public Widget {
	public:
		explicit TreeCard(TreeItemId id);

		[[nodiscard]] TreeItemId id() const { return mId; }

		void setTitle(std::string title);
		void setLabel(std::string label);
		void setAccent(std::optional<RGBA> accent);
		void setBadge(std::optional<RGBA> color, bool pulsing = false);
		void setHeaderShown(bool shown);
		[[nodiscard]] bool isHeaderShown() const { return mHeaderShown; }

		[[nodiscard]] const std::string& title() const { return mTitle; }
		[[nodiscard]] const std::string& label() const { return mLabel; }
		[[nodiscard]] Widget& content() { return mContent; }

	protected:
		void paint(Painter& painter) override;
		void paintOverlay(Painter& painter) override;
		void onTick(halnf deltaMs) override;

	private:
		friend class TreeView;

		[[nodiscard]] RGBA accent() const;
		void paintHeaderedBody(Painter& painter, const RectF& rect, halnf radius);
		void paintPlainBody(Painter& painter, const RectF& rect, halnf radius);

		TreeItemId mId;
		std::string mTitle;
		std::string mLabel;
		std::optional<RGBA> mAccent;
		std::optional<RGBA> mBadge;
		bool mBadgePulsing = false;
		bool mHeaderShown = true;
		Widget mContent;
		halnf mDetail = 0;
		halnf mOpacity = 1;
		halnf mStack = 0;
		size_t mHidden = 0;
		bool mSelected = false;
		bool mCurrent = false;
		halnf mHover = 0;
		halnf mClockMs = 0;
	};

	struct TreeViewStyle {
		TreeLayoutMetrics layout;
		halnf linkThickness = 1.5f;
		halnf portRadius = 4.5f;
		halnf portHitRadius = 11;
		halnf dragThreshold = 4;
		halnf tugReach = 60;
		halnf minimumScale = 0.08f;
		halnf maximumScale = 3;
		halnf compactBelowScale = 0.45f;
		halnf detailedAboveScale = 0.7f;
		halnf frameMargin = 32;
		halnf wheelStep = 1.12f;
		halnf levelGap = 0.34f;
		MagneticStepTuning step;
		halnf fullViewFill = 0.5f;
	};

	// Shows a TreeModel as cards. Each item is open or closed; an open item shows its children in a column to its
	// right. Zoom has two independent axes:
	// - vertical (wheel, pinch) is the camera: it scales the view around the selected item, or the item under the
	//   pointer, and never changes which items are open. At compactBelowScale and below cards show a compact label,
	//   at detailedAboveScale and above the full card with its widgets. The camera never rests between the two:
	//   zooming into that range resists and springs back, and pushing past step.commitFraction of it moves to the
	//   other form.
	// - horizontal (horizontal wheel, or Shift+wheel) is topology: it opens the next level of the selected item's
	//   subtree (the whole tree without a selection), or folds the deepest open level. Each step is magnetic: input
	//   pulls toward it with growing resistance and springs back when it stops; pulling past step.commitFraction of
	//   levelGap commits the step.
	// Past the detailed range lies one more magnetic level, the full view: once a card fills fullViewFill of the
	// view, zooming further in pulls it toward the whole view and pushing past step.commitFraction of levelGap
	// commits. The fullViewFrame() then grows out of the card to cover the view while the whole graph (cards, links,
	// selection, ports) fades behind it. The frame holds the most detailed widget the host gives it. While an item is
	// maximized the view takes no wheel or pinch input, so the frame's content scrolls freely; Back, or Escape
	// while the frame itself has keyboard focus, leaves.
	// Tapping the selected item opens or closes it; opening scales the camera out as needed so the whole opened
	// subtree fits the view. Cards spring between their places and fade as they are shown or folded into their
	// parent. Camera zoom animates on a spring and keeps the selected item in place, or the point under the pointer
	// when nothing is selected.
	class TreeView : public Widget {
	public:
		using CardFactory = std::function<std::unique_ptr<TreeCard>(TreeItemId)>;
		using CardUpdater = std::function<void(TreeCard&)>;

		enum class Port { Parent, Children };

		struct PortHit {
			TreeItemId item = 0;
			Port port = Port::Children;
		};

		explicit TreeView(const TreeModel& model);
		~TreeView() override;

		[[nodiscard]] TreeViewStyle& style() { return mStyle; }
		void setCardFactory(CardFactory factory);
		void setCardUpdater(CardUpdater updater);

		void setOnSelected(std::function<void(TreeItemId)> callback) { mOnSelected = std::move(callback); }
		void setOnSelectionChanged(std::function<void()> callback) { mOnSelectionChanged = std::move(callback); }
		void setOnDeleteRequested(std::function<void(const std::vector<TreeItemId>&)> callback) {
			mOnDeleteRequested = std::move(callback);
		}
		void setOnReparent(std::function<void(TreeItemId child, TreeItemId parent)> callback) {
			mOnReparent = std::move(callback);
		}
		void setOnCreateChild(std::function<void(TreeItemId parent)> callback) { mOnCreateChild = std::move(callback); }
		void setOnFullViewChanged(std::function<void(std::optional<TreeItemId>)> callback) {
			mOnFullViewChanged = std::move(callback);
		}

		void modelChanged();
		void refreshCards();

		[[nodiscard]] std::optional<TreeItemId> selection() const { return mSelection; }
		[[nodiscard]] std::vector<TreeItemId> selectedItems() const;
		[[nodiscard]] bool isSelected(TreeItemId id) const { return mSelected.contains(id); }
		void select(std::optional<TreeItemId> id);
		void setSelection(const std::vector<TreeItemId>& ids, std::optional<TreeItemId> current);
		void selectAll();
		void requestDelete();
		[[nodiscard]] std::optional<RectF> selectionBox() const;

		[[nodiscard]] bool isOpen(TreeItemId id) const { return mExpansion.isOpen(id); }
		void setOpen(TreeItemId id, bool open);
		void openTo(std::optional<TreeItemId> scope, size_t depth);
		[[nodiscard]] size_t openDepth(std::optional<TreeItemId> scope) const;
		[[nodiscard]] size_t scopeHeight(std::optional<TreeItemId> scope) const;
		[[nodiscard]] bool isPullingLevel() const { return mCandidate.has_value(); }
		[[nodiscard]] bool isPullingCamera() const { return mCameraRest.has_value(); }

		[[nodiscard]] std::optional<TreeItemId> fullViewItem() const { return mFull; }
		[[nodiscard]] bool isPullingFullView() const { return mFullPull.has_value(); }
		[[nodiscard]] halnf fullViewAmount() const { return mFrame.progress(); }
		[[nodiscard]] FullViewFrame& fullViewFrame() { return mFrame; }
		[[nodiscard]] halnf fullViewScale() const;
		void enterFullView(TreeItemId id, bool animated = true);
		void leaveFullView(bool animated = true);

		[[nodiscard]] halnf scale() const { return mScale.value(); }
		[[nodiscard]] halnf targetScale() const { return mScale.target(); }
		[[nodiscard]] Vec2F pan() const;
		void setView(Vec2F pan, halnf scale, bool animated);
		void frame(bool animated = true);
		void centerOn(TreeItemId id, bool animated = true);

		[[nodiscard]] const TreeStructure& structure() const { return mStructure; }
		[[nodiscard]] halnf detailAmount() const;
		[[nodiscard]] std::optional<TreeItemState> stateOf(TreeItemId id) const;
		[[nodiscard]] halnf emergence(TreeItemId id) const;
		[[nodiscard]] TreeCard* card(TreeItemId id) const;
		[[nodiscard]] std::optional<RectF> cardRect(TreeItemId id) const;
		[[nodiscard]] std::optional<Vec2F> portPosition(TreeItemId id, Port port) const;
		[[nodiscard]] std::optional<TreeItemId> itemAt(Vec2F local) const;
		[[nodiscard]] std::optional<PortHit> portAt(Vec2F local) const;
		[[nodiscard]] size_t hiddenDescendants(TreeItemId id) const;
		[[nodiscard]] bool isAnimating() const;

	protected:
		void onEvent(Event& event) override;
		void filterDescendantEvent(Event& event) override;
		void onTick(halnf deltaMs) override;
		void paint(Painter& painter) override;
		void paintOverlay(Painter& painter) override;
		void onHoverChanged() override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return true; }

	private:
		friend class TreeCard;
		friend class TreeViewLayout;

		enum class Gesture { None, Pan, Tug, Link, Box };

		struct Configuration {
			TreeExpansion expansion;
		};

		struct LinkDrag {
			PortHit from;
			Vec2F pointer = { 0, 0 };
			std::optional<TreeItemId> target;
			bool valid = false;
		};

		using AnimatedVec2 = Animated<SpringVec2, Vec2F>;

		[[nodiscard]] std::optional<size_t> scopeIndex(std::optional<TreeItemId> scope) const;
		[[nodiscard]] std::optional<Configuration> neighbour(int direction) const;
		[[nodiscard]] bool isShownIndex(size_t item) const { return mExpansion.isShown(mStructure, item); }
		[[nodiscard]] bool isInteractive(size_t item) const;
		[[nodiscard]] bool canLink(const PortHit& from, TreeItemId target) const;
		[[nodiscard]] halnf displayedOpacity(size_t item) const;
		[[nodiscard]] halnf clampScale(halnf scale) const;
		[[nodiscard]] halnf levelPreview() const;
		void bakePreview();
		void bakePreviewAt(halnf preview);
		void fitInitially(Vec2F viewSize);
		[[nodiscard]] RectF rectOf(size_t item) const { return mRects[item]; }
		[[nodiscard]] std::optional<size_t> nearestShown(Vec2F local) const;
		[[nodiscard]] Vec2F fitExtent(const TreeExpansion& expansion) const;

		void syncCards();
		void syncItems(bool jump);
		void repack();
		void retarget(bool jump);
		void arrangeCards(LayoutContext& context, const RectF& area);
		void updateRects();
		void viewChanged();

		enum class WheelAxis { Camera, Topology };
		static constexpr halnf kWheelAxisReleaseMs = 200;
		[[nodiscard]] WheelAxis wheelAxisFor(halnf topology, halnf camera);
		void pull(Vec2F local, halnf amount);
		void zoomCamera(Vec2F local, halnf amount);
		void settleCamera(Vec2F local, halnf scale);
		void releaseCamera();
		[[nodiscard]] bool zoomFullView(Vec2F local, halnf amount);
		[[nodiscard]] bool pullIntoFullView(Vec2F local, halnf amount);
		void releaseFullView();
		void advanceSteps(halnf deltaMs);
		void settleOutOfFullView(TreeItemId item);
		[[nodiscard]] std::optional<TreeItemId> fullViewCandidate(Vec2F local) const;
		[[nodiscard]] halnf backdropOpacity() const;
		[[nodiscard]] bool takesViewInput() const;
		[[nodiscard]] bool isFullViewShown() const { return mFull.has_value() || mFullPull.has_value(); }
		[[nodiscard]] std::optional<TreeItemId> morphingItem() const;
		void fullViewChanged();
		[[nodiscard]] halnf snappedScale(halnf scale) const;
		void releasePull();
		void commit(const Configuration& configuration, halnf preview);
		void scaleAround(Vec2F local, halnf scale);
		void pivotOn(std::optional<TreeItemId> id, std::optional<Vec2F> screen, bool animated);
		void pivotOnPoint(Vec2F local);
		void dropPivot();
		void followSubtree(TreeItemId id);

		void pressAt(Event& event, Vec2F local);
		void moveTo(Event& event, Vec2F local);
		void releaseAt(Event& event, Vec2F local);
		void updateLink(Vec2F local);
		void finishLink();
		void setHovered(std::optional<TreeItemId> id);
		void selectByUser(TreeItemId id);
		void toggleByUser(TreeItemId id);
		void updateBox(Vec2F local);
		void selectionChanged();
		void reveal(TreeItemId id);
		void moveSelection(InputID key);

		void paintLinks(Painter& painter) const;
		void paintStacks(Painter& painter) const;
		void paintCurve(Painter& painter, Vec2F from, Vec2F to, const RGBA& color, halnf thickness) const;

		const TreeModel& mModel;
		TreeViewStyle mStyle;
		CardFactory mFactory;
		CardUpdater mUpdater;
		std::unordered_map<TreeItemId, std::unique_ptr<TreeCard>> mCards;

		TreeStructure mStructure;
		TreeExpansion mExpansion;
		std::optional<Configuration> mCandidate;
		int mCandidateDirection = 0;
		int mCandidateDirectionCommitted = 0;
		MagneticStep mLevelStep{ mStyle.step };
		std::optional<halnf> mCameraRest;
		MagneticStep mCameraStep{ mStyle.step };
		Vec2F mCameraPointer = { 0, 0 };
		std::optional<TreeItemId> mFull;
		std::optional<TreeItemId> mFullPull;
		MagneticStep mFullStep{ mStyle.step };
		std::optional<WheelAxis> mWheelAxis;
		halnf mWheelIdleMs = 0;
		std::optional<TreeItemId> mFullSettling;
		FullViewFrame mFrame;
		TreePacking mCurrent;
		TreePacking mNext;
		bool mFitPending = true;
		Vec2F mLastSize = { 0, 0 };

		std::unordered_map<TreeItemId, AnimatedVec2> mPositions;
		std::unordered_map<TreeItemId, AnimatedFloat> mOpacity;
		std::vector<RectF> mRects;

		AnimatedFloat mScale{ 1, Motion::smooth(), 0.0005f };
		AnimatedVec2 mPan{ Vec2F(0.0f, 0.0f), Motion::smooth(), 0.1f };
		std::optional<TreeItemId> mPivot;
		std::optional<Vec2F> mPivotWorld;
		AnimatedVec2 mPivotScreen{ Vec2F(0.0f, 0.0f), Motion::smooth(), 0.1f };

		std::optional<TreeItemId> mSelection;
		std::unordered_set<TreeItemId> mSelected;
		std::unordered_set<TreeItemId> mBoxBase;
		std::optional<TreeItemId> mHovered;
		std::unordered_map<TreeItemId, AnimatedFloat> mHover;

		Gesture mGesture = Gesture::None;
		Vec2F mPressLocal = { 0, 0 };
		Vec2F mBoxCorner = { 0, 0 };
		bool mDragged = false;
		bool mPressedSelected = false;
		std::optional<TreeItemId> mTugged;
		AnimatedVec2 mTug{ Vec2F(0.0f, 0.0f), Motion::fluid(), 0.1f };
		std::optional<LinkDrag> mLink;

		std::function<void(TreeItemId)> mOnSelected;
		std::function<void()> mOnSelectionChanged;
		std::function<void(const std::vector<TreeItemId>&)> mOnDeleteRequested;
		std::function<void(TreeItemId, TreeItemId)> mOnReparent;
		std::function<void(TreeItemId)> mOnCreateChild;
		std::function<void(std::optional<TreeItemId>)> mOnFullViewChanged;
	};
}
