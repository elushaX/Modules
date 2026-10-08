#pragma once

#include "Animation.hpp"
#include "NodeGraph.hpp"
#include "Painter.hpp"
#include "Shortcut.hpp"
#include "Widget.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	struct NodeEditorStyle {
		halnf headerHeight = 26;
		halnf subtitleHeight = 24;
		halnf portRowHeight = 22;
		halnf portRadius = 5;
		halnf portHitRadius = 10;
		halnf cornerRadius = 8;
		halnf linkThickness = 2;
		halnf linkHitRadius = 6;
		halnf dragThreshold = 4;
		halnf resizeGrip = 6;
		halnf minimumWidth = 120;
		halnf minimumBodyHeight = 0;
		halnf minimumZoom = 0.1f;
		halnf textZoom = 0.6f;
		halnf outlineZoom = 0.35f;
		halnf maximumZoom = 3.0f;
		Vec2F pasteOffset = { 24, 24 };
	};

	class NodeEditor : public Widget {
	public:
		enum class Command { Copy, Cut, Paste, Duplicate, Delete, SelectAll, FrameAll };

		struct NodeBadge {
			std::string text;
			RGBA color;
			bool pulsing = false;
		};

		struct NodePresentation {
			std::string title;
			std::string subtitle;
			std::optional<RGBA> accent;
			std::string body;
			std::optional<NodeBadge> badge;
			std::string shortTitle;
		};

		enum class ResizeEdges : unsigned { None = 0, Width = 1, Height = 2, Both = 3 };
		enum class Detail { Full, Simplified, Outline };

		using Presenter = std::function<NodePresentation(const Node&)>;
		using ContextMenuRequest = std::function<void(Vec2F windowPoint, Vec2F graphPoint, std::optional<NodeId> node)>;

		explicit NodeEditor(NodeGraph& graph);

		[[nodiscard]] NodeGraph& graph() { return mGraph; }
		[[nodiscard]] NodeEditorStyle& style() { return mStyle; }

		void setPresenter(Presenter presenter) { mPresenter = std::move(presenter); }
		void setOnSelectionChanged(std::function<void()> callback) { mOnSelectionChanged = std::move(callback); }
		void setOnNodeOpened(std::function<void(NodeId)> callback) { mOnNodeOpened = std::move(callback); }
		void setOnNodeToggled(std::function<void(NodeId)> callback) { mOnNodeToggled = std::move(callback); }
		void setOnEmptyDoubleClicked(std::function<void()> callback) { mOnEmptyDoubleClicked = std::move(callback); }
		void refreshPresentation();
		void setOnLinkDroppedOnEmpty(std::function<void(const PortRef& from, Vec2F graphPoint)> callback) {
			mOnLinkDroppedOnEmpty = std::move(callback);
		}

		void setOnContextMenu(ContextMenuRequest callback) { mOnContextMenu = std::move(callback); }

		void setNodeFilter(std::function<bool(NodeId)> filter);
		void refreshNodeFilter();
		[[nodiscard]] bool isShown(NodeId id) const { return !mNodeFilter || mNodeFilter(id); }
		[[nodiscard]] bool isShown(const Link& link) const { return isShown(link.from.node) && isShown(link.to.node); }

		[[nodiscard]] const std::vector<NodeId>& selection() const { return mSelection; }
		[[nodiscard]] bool isSelected(NodeId id) const;
		void select(const std::vector<NodeId>& ids);
		void clearSelection() { select({}); }

		[[nodiscard]] const std::optional<Link>& selectedLink() const { return mSelectedLink; }
		[[nodiscard]] bool isSelected(const Link& link) const;
		void selectLink(const Link& link);

		void execute(Command command);
		[[nodiscard]] ShortcutMap<Command>& shortcuts() { return mShortcuts; }
		[[nodiscard]] bool hasClipboard() const { return !mClipboard.isEmpty(); }

		[[nodiscard]] Vec2F pan() const { return mPan.value(); }
		[[nodiscard]] halnf zoom() const { return mZoom.value(); }
		[[nodiscard]] Vec2F targetPan() const;
		[[nodiscard]] halnf targetZoom() const { return mZoom.target(); }
		void setView(Vec2F pan, halnf zoom);
		void jumpToView(Vec2F pan, halnf zoom);
		// Two-finger pinch/pan: the graph point under the fingers stays under them, applied without animation.
		void applyPinch(Vec2F localCenter, Vec2F pan, halnf scale);
		void zoomAround(Vec2F localAnchor, halnf zoom);
		void frameAll();

		[[nodiscard]] Vec2F toGraph(Vec2F localPoint) const { return (localPoint - pan()) / zoom(); }
		[[nodiscard]] Vec2F toLocal(Vec2F graphPoint) const { return graphPoint * zoom() + pan(); }

		[[nodiscard]] Detail detail() const;
		[[nodiscard]] bool isVisibleOnScreen(const Node& node) const;
		[[nodiscard]] size_t paintedNodeCount() const { return mPaintedNodes; }
		[[nodiscard]] bool isFramePending() const { return mFramePending; }
		[[nodiscard]] RectF nodeRect(const Node& node) const;
		[[nodiscard]] RectF headerRect(const Node& node) const;
		[[nodiscard]] RectF bodyRect(const Node& node) const;
		[[nodiscard]] ResizeEdges resizeEdgesAt(const Node& node, Vec2F graphPoint) const;
		[[nodiscard]] Vec2F portPosition(const PortRef& port) const;
		void toggleCollapsed(NodeId id);
		[[nodiscard]] std::optional<NodeId> nodeAt(Vec2F graphPoint) const;
		[[nodiscard]] std::optional<PortRef> portAt(Vec2F graphPoint) const;
		[[nodiscard]] std::optional<Link> linkAt(Vec2F graphPoint) const;

	protected:
		void onEvent(Event& event) override;
		void onTick(halnf deltaMs) override;
		void paint(Painter& painter) override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return true; }

	private:
		static constexpr halnf kZoomTolerance = 0.0005f;
		inline static const SpringParameters kViewMotion = SpringParameters::criticallyDamped(90);

		enum class Gesture { None, Panning, MovingNodes, Linking, Marquee, Resizing };

		struct NodeResize {
			NodeId node = 0;
			ResizeEdges edges = ResizeEdges::None;
			halnf startWidth = 0;
			halnf startBodyHeight = 0;
		};

		struct ZoomAnchor {
			Vec2F local = { 0, 0 };
			Vec2F graph = { 0, 0 };
		};

		using AnimatedVec2 = Animated<SpringVec2, Vec2F>;

		struct LinkDrag {
			PortRef anchor;
			Vec2F pointer = { 0, 0 };
			std::optional<PortRef> hovered;
		};

		void bindDefaultShortcuts();
		[[nodiscard]] std::optional<std::pair<Vec2F, halnf>> framingView() const;
		void viewChanged();
		void holdZoomAnchor(bool zooming);
		[[nodiscard]] halnf clampedZoom(halnf zoom) const;

		void handlePointerDown(Event& event);
		void handlePointerMove(Event& event);
		void handlePointerUp(Event& event);
		void handleWheel(Event& event);
		void handleGesture(Event& event);
		void handleKey(Event& event);
		void beginGesture(Gesture gesture);

		void pressOnNode(NodeId id, bool additive);
		void pressOnPort(const PortRef& port);
		bool pressOnResizeEdge(NodeId id);
		void resizeTo(Vec2F graphPoint);
		void refreshHoverCursor(Vec2F graphPoint);
		void finishLink();
		void finishMarquee(bool additive);

		void copySelection();
		void pasteClipboard(Vec2F offset);
		void deleteSelection();

		void paintLinks(Painter& painter) const;
		void paintLink(Painter& painter, Vec2F from, Vec2F to, const RGBA& color, halnf thickness) const;
		[[nodiscard]] std::vector<Vec2F> linkCurvePoints(Vec2F from, Vec2F to) const;
		void paintNode(Painter& painter, const Node& node) const;
		void paintSimplifiedNode(Painter& painter, const Node& node, const NodePresentation& presentation) const;
		void paintTextLine(Painter& painter, const RectF& box, halnf textWidth, const RGBA& color) const;
		void paintShortTitle(Painter& painter, const RectF& rect, const std::string& text, const RGBA& color) const;
		[[nodiscard]] bool isVisibleOnScreen(const Link& link) const;
		void paintPortHandles(Painter& painter, const Node& node) const;
		void paintPortLabels(Painter& painter, const Node& node, PortDirection direction) const;
		void paintMarquee(Painter& painter) const;

		[[nodiscard]] NodePresentation present(const Node& node) const;
		[[nodiscard]] halnf nodeHeight(const Node& node) const;
		[[nodiscard]] halnf portsHeight(const Node& node) const;
		void paintNodeBody(Painter& painter, const Node& node, const std::string& text) const;
		[[nodiscard]] halnf paintBadge(Painter& painter, const RectF& header, const NodeBadge& badge) const;
		[[nodiscard]] RectF marqueeRect() const;
		[[nodiscard]] bool isMarqueeVisible() const;
		void selectionChanged();

		NodeGraph& mGraph;
		NodeEditorStyle mStyle;
		Presenter mPresenter;
		std::function<bool(NodeId)> mNodeFilter;
		std::function<void()> mOnEmptyDoubleClicked;
		size_t mPaintedNodes = 0;
		bool mFramePending = false;
		ShortcutMap<Command> mShortcuts;

		std::vector<NodeId> mSelection;
		std::optional<Link> mSelectedLink;
		NodeGraphFragment mClipboard;
		ualni mPasteCount = 0;

		AnimatedVec2 mPan{ Vec2F(0.0f, 0.0f), kViewMotion, AnimatedRect::kGeometryTolerance };
		AnimatedFloat mZoom{ 1, kViewMotion, kZoomTolerance };
		std::optional<ZoomAnchor> mZoomAnchor;

		Gesture mGesture = Gesture::None;
		Vec2F mPressGraphPoint = { 0, 0 };
		Vec2F mPointerGraphPoint = { 0, 0 };
		bool mDraggedPastThreshold = false;
		std::vector<std::pair<NodeId, Vec2F>> mMoveStart;
		std::optional<LinkDrag> mLinkDrag;
		std::optional<NodeResize> mResize;
		halnf mBadgeClockMs = 0;
		mutable bool mHasPulsingBadge = false;

		std::function<void()> mOnSelectionChanged;
		std::function<void(NodeId)> mOnNodeOpened;
		std::function<void(NodeId)> mOnNodeToggled;
		std::function<void(const PortRef&, Vec2F)> mOnLinkDroppedOnEmpty;
		ContextMenuRequest mOnContextMenu;
	};
}
