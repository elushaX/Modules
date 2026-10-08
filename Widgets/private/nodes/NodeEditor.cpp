#include "NodeEditor.hpp"

#include "Event.hpp"
#include "Label.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr int kLinkSegments = 24;
	constexpr int kSimplifiedLinkSegments = 8;
	constexpr halnf kCullMargin = 16;
	constexpr halnf kMinimumShortTitleSize = 7;
	constexpr halnf kShortTitleFontSize = 22;
	constexpr halnf kWheelZoomStep = 1.1f;
	constexpr halnf kFrameMargin = 40;

	halnf distance(Vec2F first, Vec2F second) {
		const Vec2F difference = first - second;
		return std::sqrt(difference.x * difference.x + difference.y * difference.y);
	}

	halnf distanceToSegment(Vec2F point, Vec2F start, Vec2F end) {
		const Vec2F segment = end - start;
		const halnf lengthSquared = segment.x * segment.x + segment.y * segment.y;
		if (lengthSquared <= 0) return distance(point, start);
		const Vec2F toPoint = point - start;
		const halnf t = std::clamp((toPoint.x * segment.x + toPoint.y * segment.y) / lengthSquared, 0.0f, 1.0f);
		return distance(point, start + segment * t);
	}

	RectF normalized(Vec2F first, Vec2F second) {
		const Vec2F low = { std::min(first.x, second.x), std::min(first.y, second.y) };
		const Vec2F high = { std::max(first.x, second.x), std::max(first.y, second.y) };
		return makeRect(low, high - low);
	}

	RectF boundingRect(const RectF& first, const RectF& second) {
		const Vec2F low = { std::min(first.pos.x, second.pos.x), std::min(first.pos.y, second.pos.y) };
		const Vec2F high = {
			std::max(first.pos.x + first.size.x, second.pos.x + second.size.x),
			std::max(first.pos.y + first.size.y, second.pos.y + second.size.y),
		};
		return makeRect(low, high - low);
	}
}

NodeEditor::NodeEditor(NodeGraph& graph) :
	mGraph(graph) {
	setDebugName("node editor");
	setClipsChildren(true);
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	bindDefaultShortcuts();
}

void NodeEditor::bindDefaultShortcuts() {
	mShortcuts.bind(ctrl(InputID::C), Command::Copy);
	mShortcuts.bind(ctrl(InputID::X), Command::Cut);
	mShortcuts.bind(ctrl(InputID::V), Command::Paste);
	mShortcuts.bind(ctrl(InputID::D), Command::Duplicate);
	mShortcuts.bind(ctrl(InputID::A), Command::SelectAll);
	mShortcuts.bind(plain(InputID::DELETE_KEY), Command::Delete);
	mShortcuts.bind(plain(InputID::BACKSPACE), Command::Delete);
	mShortcuts.bind(plain(InputID::HOME), Command::FrameAll);
}

bool NodeEditor::isSelected(NodeId id) const {
	return std::find(mSelection.begin(), mSelection.end(), id) != mSelection.end();
}

void NodeEditor::select(const std::vector<NodeId>& ids) {
	mSelection = ids;
	mSelectedLink.reset();
	selectionChanged();
}

bool NodeEditor::isSelected(const Link& link) const { return mSelectedLink && *mSelectedLink == link; }

void NodeEditor::selectLink(const Link& link) {
	mSelection.clear();
	mSelectedLink = link;
	selectionChanged();
}

void NodeEditor::selectionChanged() {
	requestPaint();
	if (mOnSelectionChanged) mOnSelectionChanged();
}

void NodeEditor::execute(Command command) {
	switch (command) {
		case Command::Copy: copySelection(); break;
		case Command::Cut:
			copySelection();
			deleteSelection();
			break;
		case Command::Paste: pasteClipboard(mStyle.pasteOffset * halnf(++mPasteCount)); break;
		case Command::Duplicate:
			copySelection();
			pasteClipboard(mStyle.pasteOffset);
			break;
		case Command::Delete: deleteSelection(); break;
		case Command::SelectAll:
			{
				std::vector<NodeId> all;
				for (const Node& node : mGraph.nodes())
					if (isShown(node.id)) all.push_back(node.id);
				select(all);
				break;
			}
		case Command::FrameAll: frameAll(); break;
	}
	requestPaint();
}

void NodeEditor::copySelection() {
	if (mSelection.empty()) return;

	mClipboard = mGraph.extract(mSelection);
	mPasteCount = 0;
}

void NodeEditor::pasteClipboard(Vec2F offset) {
	if (mClipboard.isEmpty()) return;

	select(mGraph.insert(mClipboard, offset));
}

void NodeEditor::deleteSelection() {
	if (mSelection.empty() && !mSelectedLink) return;

	if (mSelectedLink) {
		if (mGraph.hasLink(*mSelectedLink)) mGraph.disconnect(*mSelectedLink);
		mSelectedLink.reset();
	}
	if (!mSelection.empty()) mGraph.removeNodes(std::exchange(mSelection, {}));
	selectionChanged();
}

halnf NodeEditor::clampedZoom(halnf zoom) const { return std::clamp(zoom, mStyle.minimumZoom, mStyle.maximumZoom); }

void NodeEditor::setView(Vec2F pan, halnf zoom) {
	mZoomAnchor.reset();
	mPan.animateTo(pan);
	mZoom.animateTo(clampedZoom(zoom));
	viewChanged();
}

void NodeEditor::jumpToView(Vec2F pan, halnf zoom) {
	mZoomAnchor.reset();
	mPan.jumpTo(pan);
	mZoom.jumpTo(clampedZoom(zoom));
	viewChanged();
}

void NodeEditor::applyPinch(Vec2F localCenter, Vec2F pan, halnf scale) {
	const halnf zoom = targetZoom();
	const Vec2F previousCenter = localCenter - pan;
	const Vec2F graphUnderFingers = (previousCenter - targetPan()) / zoom;
	const halnf nextZoom = clampedZoom(zoom * scale);

	jumpToView(localCenter - graphUnderFingers * nextZoom, nextZoom);
}

void NodeEditor::zoomAround(Vec2F localAnchor, halnf zoom) {
	const Vec2F graphAnchor = (localAnchor - targetPan()) / targetZoom();

	mZoomAnchor = ZoomAnchor{ localAnchor, graphAnchor };
	mZoom.animateTo(clampedZoom(zoom));
	viewChanged();
}

Vec2F NodeEditor::targetPan() const {
	if (!mZoomAnchor) return mPan.target();
	return mZoomAnchor->local - mZoomAnchor->graph * mZoom.target();
}

void NodeEditor::viewChanged() {
	startTicking();
	requestPaint();
}

void NodeEditor::onTick(halnf deltaMs) {
	const bool zooming = mZoom.advance(deltaMs);
	if (mZoomAnchor) holdZoomAnchor(zooming);

	const bool panning = mPan.advance(deltaMs);
	mBadgeClockMs += deltaMs;
	requestPaint();
	if (!zooming && !panning && !mHasPulsingBadge) stopTicking();
}

void NodeEditor::refreshPresentation() {
	mHasPulsingBadge = false;
	for (const Node& node : mGraph.nodes()) {
		const NodePresentation presentation = present(node);
		if (presentation.badge && presentation.badge->pulsing) mHasPulsingBadge = true;
	}
	if (mHasPulsingBadge) startTicking();
	requestPaint();
}

void NodeEditor::holdZoomAnchor(bool zooming) {
	const Vec2F anchoredPan = mZoomAnchor->local - mZoomAnchor->graph * mZoom.value();
	mPan.jumpTo(anchoredPan);
	if (!zooming) mZoomAnchor.reset();
}

void NodeEditor::setNodeFilter(std::function<bool(NodeId)> filter) {
	mNodeFilter = std::move(filter);
	refreshNodeFilter();
}

void NodeEditor::refreshNodeFilter() {
	std::vector<NodeId> shown;
	for (NodeId id : mSelection)
		if (isShown(id)) shown.push_back(id);
	if (shown != mSelection || (mSelectedLink && !isShown(*mSelectedLink))) select(shown);
	requestPaint();
}

void NodeEditor::frameAll() {
	if (size().x <= 0 || size().y <= 0) {
		mFramePending = true;
		requestPaint();
		return;
	}
	mFramePending = false;
	if (const std::optional<std::pair<Vec2F, halnf>> view = framingView()) setView(view->first, view->second);
}

std::optional<std::pair<Vec2F, halnf>> NodeEditor::framingView() const {
	std::optional<RectF> shownBounds;
	for (const Node& node : mGraph.nodes()) {
		if (!isShown(node.id)) continue;
		shownBounds = shownBounds ? boundingRect(*shownBounds, nodeRect(node)) : nodeRect(node);
	}
	if (!shownBounds) return std::nullopt;

	const RectF bounds = *shownBounds;
	const Vec2F available = size() - Vec2F(kFrameMargin * 2, kFrameMargin * 2);
	const halnf zoom =
		std::clamp(std::min(available.x / bounds.size.x, available.y / bounds.size.y), mStyle.minimumZoom, 1.0f);
	const Vec2F center = bounds.pos + bounds.size / 2.0f;
	return std::make_pair(size() / 2.0f - center * zoom, zoom);
}

halnf NodeEditor::portsHeight(const Node& node) const {
	const NodeType& type = mGraph.typeOf(node);
	const size_t rows = std::max(type.inputs.size(), type.outputs.size());
	return halnf(rows) * mStyle.portRowHeight;
}

halnf NodeEditor::nodeHeight(const Node& node) const {
	const halnf summary = mStyle.headerHeight + mStyle.subtitleHeight + portsHeight(node);
	return node.collapsed ? summary : summary + node.bodyHeight;
}

RectF NodeEditor::nodeRect(const Node& node) const { return makeRect(node.position, { node.width, nodeHeight(node) }); }

RectF NodeEditor::headerRect(const Node& node) const {
	return makeRect(node.position, { node.width, mStyle.headerHeight });
}

RectF NodeEditor::bodyRect(const Node& node) const {
	const halnf top = mStyle.headerHeight + mStyle.subtitleHeight + portsHeight(node);
	return makeRect(node.position + Vec2F(0.0f, top), { node.width, node.collapsed ? 0.0f : node.bodyHeight });
}

NodeEditor::ResizeEdges NodeEditor::resizeEdgesAt(const Node& node, Vec2F graphPoint) const {
	const RectF rect = nodeRect(node);
	const halnf grip = mStyle.resizeGrip / std::max(zoom(), 1.0f);
	const halnf right = rect.pos.x + rect.size.x;
	const halnf bottom = rect.pos.y + rect.size.y;

	const bool withinX = graphPoint.x >= rect.pos.x && graphPoint.x <= right + grip;
	const bool withinY = graphPoint.y >= rect.pos.y && graphPoint.y <= bottom + grip;
	if (!withinX || !withinY) return ResizeEdges::None;

	const bool onRight = graphPoint.x >= right - grip;
	const bool onBottom = !node.collapsed && graphPoint.y >= bottom - grip;
	if (onRight && onBottom) return ResizeEdges::Both;
	if (onRight) return ResizeEdges::Width;
	if (onBottom) return ResizeEdges::Height;
	return ResizeEdges::None;
}

Vec2F NodeEditor::portPosition(const PortRef& port) const {
	const Node* node = mGraph.findNode(port.node);
	if (!node) return { 0, 0 };

	const halnf x = port.direction == PortDirection::Input ? node->position.x : node->position.x + node->width;
	const halnf rowsTop = node->position.y + mStyle.headerHeight + mStyle.subtitleHeight;
	return { x, rowsTop + mStyle.portRowHeight * (halnf(port.index) + 0.5f) };
}

void NodeEditor::toggleCollapsed(NodeId id) {
	const Node* node = mGraph.findNode(id);
	if (!node) return;

	mGraph.setCollapsed(id, !node->collapsed);
	if (mOnNodeToggled) mOnNodeToggled(id);
	requestPaint();
}

std::optional<NodeId> NodeEditor::nodeAt(Vec2F graphPoint) const {
	const std::vector<Node>& nodes = mGraph.nodes();
	for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
		if (isShown(node->id) && contains(nodeRect(*node), graphPoint)) return node->id;
	}
	return std::nullopt;
}

std::optional<PortRef> NodeEditor::portAt(Vec2F graphPoint) const {
	const halnf radius = mStyle.portHitRadius / std::max(zoom(), 1.0f);
	const std::vector<Node>& nodes = mGraph.nodes();

	for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
		if (!isShown(node->id)) continue;
		for (PortDirection direction : { PortDirection::Input, PortDirection::Output }) {
			const size_t count = mGraph.typeOf(*node).ports(direction).size();
			for (ualni index = 0; index < count; index++) {
				const PortRef port = { node->id, direction, index };
				if (distance(portPosition(port), graphPoint) <= radius) return port;
			}
		}
	}
	return std::nullopt;
}

std::optional<Link> NodeEditor::linkAt(Vec2F graphPoint) const {
	const Vec2F local = toLocal(graphPoint);
	const halnf tolerance = (mStyle.linkHitRadius + mStyle.linkThickness * 0.5f) * std::max(zoom(), 0.5f);

	std::optional<Link> closest;
	halnf closestDistance = tolerance;
	for (const Link& link : mGraph.links()) {
		if (!isShown(link)) continue;
		const std::vector<Vec2F> points = linkCurvePoints(toLocal(portPosition(link.from)), toLocal(portPosition(link.to)));
		for (size_t step = 1; step < points.size(); step++) {
			const halnf d = distanceToSegment(local, points[step - 1], points[step]);
			if (d <= closestDistance) {
				closestDistance = d;
				closest = link;
			}
		}
	}
	return closest;
}

void NodeEditor::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown: handlePointerDown(event); break;
		case Event::Type::PointerMove: handlePointerMove(event); break;
		case Event::Type::PointerUp: handlePointerUp(event); break;
		case Event::Type::Wheel: handleWheel(event); break;
		case Event::Type::Gesture: handleGesture(event); break;
		case Event::Type::KeyDown: handleKey(event); break;
		default: break;
	}
}

void NodeEditor::beginGesture(Gesture gesture) {
	mGesture = gesture;
	mDraggedPastThreshold = false;
	ui()->capturePointer(*this);
}

void NodeEditor::handlePointerDown(Event& event) {
	ui()->setKeyboardFocus(this);
	mPressGraphPoint = toGraph(event.position());
	mPointerGraphPoint = mPressGraphPoint;
	event.accept();

	if (event.button() == PointerButton::Middle) {
		beginGesture(Gesture::Panning);
		return;
	}

	std::optional<NodeId> node = nodeAt(mPressGraphPoint);
	for (auto candidate = mGraph.nodes().rbegin(); !node && candidate != mGraph.nodes().rend(); ++candidate) {
		if (isShown(candidate->id) && resizeEdgesAt(*candidate, mPressGraphPoint) != ResizeEdges::None)
			node = candidate->id;
	}

	if (event.button() == PointerButton::Secondary) {
		if (node && !isSelected(*node)) select({ *node });
		else if (!node) {
			if (const std::optional<Link> link = linkAt(mPressGraphPoint)) selectLink(*link);
		}
		if (mOnContextMenu) mOnContextMenu(event.windowPosition(), mPressGraphPoint, node);
		return;
	}

	if (const std::optional<PortRef> port = portAt(mPressGraphPoint)) {
		pressOnPort(*port);
		return;
	}
	if (node && pressOnResizeEdge(*node)) return;

	if (node && event.clickCount() >= 2 && event.clickCount() % 2 == 0) {
		const bool onHeader = contains(headerRect(*mGraph.findNode(*node)), mPressGraphPoint);
		if (onHeader) toggleCollapsed(*node);
		else if (!onHeader && mOnNodeOpened) mOnNodeOpened(*node);
		return;
	}

	if (node) {
		pressOnNode(*node, event.modifiers().shift || event.modifiers().control);
		return;
	}

	if (const std::optional<Link> link = linkAt(mPressGraphPoint)) {
		selectLink(*link);
		return;
	}

	if (event.clickCount() >= 2 && event.clickCount() % 2 == 0 && mOnEmptyDoubleClicked) {
		mOnEmptyDoubleClicked();
		return;
	}

	if (event.modifiers().alt) beginGesture(Gesture::Panning);
	else beginGesture(Gesture::Marquee);
}

bool NodeEditor::pressOnResizeEdge(NodeId id) {
	const Node& node = *mGraph.findNode(id);
	const ResizeEdges edges = resizeEdgesAt(node, mPressGraphPoint);
	if (edges == ResizeEdges::None) return false;

	if (!isSelected(id)) select({ id });
	mGraph.bringToFront(id);
	mResize = NodeResize{ id, edges, node.width, node.bodyHeight };
	beginGesture(Gesture::Resizing);
	return true;
}

void NodeEditor::resizeTo(Vec2F graphPoint) {
	const Vec2F delta = graphPoint - mPressGraphPoint;
	const unsigned edges = unsigned(mResize->edges);
	const Node* node = mGraph.findNode(mResize->node);
	if (!node) return;

	const halnf width =
		edges & unsigned(ResizeEdges::Width) ? std::max(mStyle.minimumWidth, mResize->startWidth + delta.x) : node->width;
	const halnf bodyHeight = edges & unsigned(ResizeEdges::Height)
														 ? std::max(mStyle.minimumBodyHeight, mResize->startBodyHeight + delta.y)
														 : node->bodyHeight;
	mGraph.resizeNode(mResize->node, width, bodyHeight);
}

void NodeEditor::refreshHoverCursor(Vec2F graphPoint) {
	CursorShape shape = CursorShape::Default;
	for (auto node = mGraph.nodes().rbegin(); node != mGraph.nodes().rend(); ++node) {
		if (!isShown(node->id)) continue;
		const ResizeEdges edges = resizeEdgesAt(*node, graphPoint);
		if (edges == ResizeEdges::Both) shape = CursorShape::ResizeNorthWestSouthEast;
		else if (edges == ResizeEdges::Width) shape = CursorShape::ResizeHorizontal;
		else if (edges == ResizeEdges::Height) shape = CursorShape::ResizeVertical;
		if (shape != CursorShape::Default || contains(nodeRect(*node), graphPoint)) break;
	}
	setCursor(shape);
}

void NodeEditor::pressOnNode(NodeId id, bool additive) {
	if (additive && isSelected(id)) {
		std::vector<NodeId> remaining = mSelection;
		std::erase(remaining, id);
		select(remaining);
		return;
	}

	if (additive) {
		std::vector<NodeId> extended = mSelection;
		extended.push_back(id);
		select(extended);
	} else if (!isSelected(id)) {
		select({ id });
	}

	mMoveStart.clear();
	for (NodeId selected : mSelection) {
		mGraph.bringToFront(selected);
		mMoveStart.emplace_back(selected, mGraph.findNode(selected)->position);
	}
	beginGesture(Gesture::MovingNodes);
}

void NodeEditor::pressOnPort(const PortRef& port) {
	const std::vector<Link> existing = mGraph.linksAt(port);
	const bool detachExisting = port.direction == PortDirection::Input && !existing.empty();

	if (detachExisting) {
		const Link link = existing.back();
		mGraph.disconnect(link);
		mLinkDrag = LinkDrag{ link.from, mPressGraphPoint, std::nullopt };
	} else {
		mLinkDrag = LinkDrag{ port, mPressGraphPoint, std::nullopt };
	}
	beginGesture(Gesture::Linking);
}

void NodeEditor::handlePointerMove(Event& event) {
	if (mGesture == Gesture::None) {
		refreshHoverCursor(toGraph(event.position()));
		return;
	}

	const Vec2F graphPoint = toGraph(event.position());
	event.accept();

	if (distance(toLocal(graphPoint), toLocal(mPressGraphPoint)) >= mStyle.dragThreshold) mDraggedPastThreshold = true;

	switch (mGesture) {
		case Gesture::Panning:
			if (mZoomAnchor) mZoomAnchor->local = mZoomAnchor->local + event.delta();
			else mPan.animateTo(targetPan() + event.delta());
			viewChanged();
			break;
		case Gesture::MovingNodes:
			for (const auto& [id, start] : mMoveStart)
				mGraph.moveNode(id, start + graphPoint - mPressGraphPoint);
			break;
		case Gesture::Linking:
			mLinkDrag->pointer = graphPoint;
			mLinkDrag->hovered = portAt(graphPoint);
			if (mLinkDrag->hovered && !mGraph.canConnect(mLinkDrag->anchor, *mLinkDrag->hovered)) mLinkDrag->hovered.reset();
			break;
		case Gesture::Resizing: resizeTo(graphPoint); break;
		case Gesture::Marquee:
		case Gesture::None: break;
	}

	mPointerGraphPoint = graphPoint;
	requestPaint();
}

void NodeEditor::handlePointerUp(Event& event) {
	if (mGesture == Gesture::None) return;

	if (mGesture == Gesture::Linking) finishLink();
	if (mGesture == Gesture::Marquee) {
		const bool additive = event.modifiers().shift || event.modifiers().control;
		if (isMarqueeVisible()) finishMarquee(additive);
		else if (!additive) clearSelection();
	}

	mGesture = Gesture::None;
	mMoveStart.clear();
	mResize.reset();
	ui()->releasePointer(*this);
	event.accept();
	requestPaint();
}

void NodeEditor::finishLink() {
	if (!mLinkDrag) return;

	const LinkDrag drag = *mLinkDrag;
	mLinkDrag.reset();
	if (drag.hovered) {
		mGraph.connect(drag.anchor, *drag.hovered);
		return;
	}

	const bool droppedOnEmpty = mDraggedPastThreshold && !nodeAt(drag.pointer);
	if (droppedOnEmpty && mOnLinkDroppedOnEmpty) mOnLinkDroppedOnEmpty(drag.anchor, drag.pointer);
}

void NodeEditor::finishMarquee(bool additive) {
	const RectF area = marqueeRect();
	std::vector<NodeId> picked = additive ? mSelection : std::vector<NodeId>{};

	for (const Node& node : mGraph.nodes()) {
		if (!isShown(node.id)) continue;
		if (overlaps(area, nodeRect(node)) && std::find(picked.begin(), picked.end(), node.id) == picked.end()) {
			picked.push_back(node.id);
		}
	}
	select(picked);
}

bool NodeEditor::isMarqueeVisible() const { return mGesture == Gesture::Marquee && mDraggedPastThreshold; }

RectF NodeEditor::marqueeRect() const { return normalized(mPressGraphPoint, mPointerGraphPoint); }

void NodeEditor::handleGesture(Event& event) {
	const TouchGesture& gesture = event.touchGesture();
	if (gesture.phase == GesturePhase::Update) applyPinch(event.position(), gesture.pan, gesture.scale);
	event.accept();
}

void NodeEditor::handleWheel(Event& event) {
	if (event.wheelAmount() == 0) return;
	zoomAround(event.position(), targetZoom() * std::pow(kWheelZoomStep, event.wheelAmount()));
	event.accept();
}

void NodeEditor::handleKey(Event& event) {
	const ShortcutMap<Command>::Binding* binding = mShortcuts.find(event);
	if (!binding) return;

	execute(binding->command);
	event.accept();
}

NodeEditor::NodePresentation NodeEditor::present(const Node& node) const {
	if (mPresenter) return mPresenter(node);
	return { mGraph.typeOf(node).name, {}, std::nullopt };
}

NodeEditor::Detail NodeEditor::detail() const {
	if (zoom() < mStyle.outlineZoom) return Detail::Outline;
	if (zoom() < mStyle.textZoom) return Detail::Simplified;
	return Detail::Full;
}

bool NodeEditor::isVisibleOnScreen(const Node& node) const {
	const RectF graphRect = nodeRect(node);
	const RectF local = makeRect(toLocal(graphRect.pos), graphRect.size * zoom());
	return overlaps(insetRect(localRect(), -kCullMargin), local);
}

bool NodeEditor::isVisibleOnScreen(const Link& link) const {
	const RectF span = normalized(toLocal(portPosition(link.from)), toLocal(portPosition(link.to)));
	const halnf bend = std::max(span.size.x * 0.5f, 40.0f * zoom());
	return overlaps(insetRect(localRect(), -kCullMargin), insetRect(span, -bend));
}

void NodeEditor::paint(Painter& painter) {
	if (mFramePending && size().x > 0 && size().y > 0) {
		mFramePending = false;
		if (const std::optional<std::pair<Vec2F, halnf>> view = framingView()) jumpToView(view->first, view->second);
	}
	painter.fillRect(localRect(), theme().palette.window);

	ScopedClip clip(painter, localRect());
	mPaintedNodes = 0;
	for (const Node& node : mGraph.nodes()) {
		if (!isShown(node.id) || !isVisibleOnScreen(node)) continue;
		paintNode(painter, node);
		mPaintedNodes++;
	}
	paintLinks(painter);
	if (detail() == Detail::Full) {
		for (const Node& node : mGraph.nodes())
			if (isShown(node.id) && isVisibleOnScreen(node)) paintPortHandles(painter, node);
	}
	if (isMarqueeVisible()) paintMarquee(painter);
}

void NodeEditor::paintLinks(Painter& painter) const {
	for (const Link& link : mGraph.links()) {
		if (!isShown(link) || !isVisibleOnScreen(link)) continue;
		const Vec2F from = toLocal(portPosition(link.from));
		const Vec2F to = toLocal(portPosition(link.to));
		if (isSelected(link)) {
			RGBA halo = theme().palette.accent;
			halo.a *= 0.35f;
			paintLink(painter, from, to, halo, mStyle.linkThickness * 3.5f);
			paintLink(painter, from, to, theme().palette.accent, mStyle.linkThickness * 1.8f);
			continue;
		}
		const PortDeclaration* port = mGraph.port(link.from);
		const RGBA color = port ? port->color : theme().palette.mutedText;
		paintLink(painter, from, to, color, mStyle.linkThickness);
	}

	if (!mLinkDrag) return;

	const Vec2F anchor = toLocal(portPosition(mLinkDrag->anchor));
	const Vec2F pointer = toLocal(mLinkDrag->hovered ? portPosition(*mLinkDrag->hovered) : mLinkDrag->pointer);
	const RGBA color = mLinkDrag->hovered ? theme().palette.accent : theme().palette.mutedText;

	if (mLinkDrag->anchor.direction == PortDirection::Output)
		paintLink(painter, anchor, pointer, color, mStyle.linkThickness);
	else paintLink(painter, pointer, anchor, color, mStyle.linkThickness);
}

std::vector<Vec2F> NodeEditor::linkCurvePoints(Vec2F from, Vec2F to) const {
	const halnf bend = std::max(std::abs(to.x - from.x) * 0.5f, 40.0f * zoom());
	const Vec2F controlFrom = from + Vec2F(bend, 0.0f);
	const Vec2F controlTo = to - Vec2F(bend, 0.0f);

	const int segments = detail() == Detail::Full ? kLinkSegments : kSimplifiedLinkSegments;
	std::vector<Vec2F> points;
	points.reserve(size_t(segments) + 1);
	for (int step = 0; step <= segments; step++) {
		const halnf t = halnf(step) / halnf(segments);
		const halnf u = 1 - t;
		points.push_back(
			from * (u * u * u) + controlFrom * (3 * u * u * t) + controlTo * (3 * u * t * t) + to * (t * t * t)
		);
	}
	return points;
}

void NodeEditor::paintLink(Painter& painter, Vec2F from, Vec2F to, const RGBA& color, halnf thickness) const {
	painter.polyline(linkCurvePoints(from, to), color, thickness * std::max(zoom(), 0.5f));
}

void NodeEditor::paintNode(Painter& painter, const Node& node) const {
	const Theme& style = theme();
	const NodePresentation presentation = present(node);
	if (detail() != Detail::Full) {
		paintSimplifiedNode(painter, node, presentation);
		return;
	}

	const RectF graphRect = nodeRect(node);
	const RectF rect = makeRect(toLocal(graphRect.pos), graphRect.size * zoom());
	const halnf radius = mStyle.cornerRadius * zoom();
	const halnf header = mStyle.headerHeight * zoom();
	const halnf padding = 10 * zoom();
	const RGBA accent = presentation.accent.value_or(mGraph.typeOf(node).headerColor);

	painter.fillRect(translated(rect, Vec2F(0.0f, 3 * zoom())), style.palette.shadow, radius);
	painter.fillRect(rect, accent, radius);
	painter.fillRect(
		makeRect(rect.pos + Vec2F(0.0f, header), rect.size - Vec2F(0.0f, header)), style.palette.raisedSurface, radius
	);

	const halnf badgeWidth =
		presentation.badge ? paintBadge(painter, makeRect(rect.pos, { rect.size.x, header }), *presentation.badge) : 0;
	const RectF titleBox = makeRect(rect.pos + Vec2F(padding, 0.0f), { rect.size.x - padding * 2 - badgeWidth, header });
	painter.text(presentation.title, titleBox, style.metrics.fontSize * zoom(), TextAlign::Start, style.palette.onAccent);

	const RectF subtitleBox =
		makeRect(rect.pos + Vec2F(padding, header), { rect.size.x - padding * 2, mStyle.subtitleHeight * zoom() });
	painter.text(
		presentation.subtitle, subtitleBox, style.metrics.smallFontSize * zoom(), TextAlign::Start, style.palette.mutedText
	);

	paintPortLabels(painter, node, PortDirection::Input);
	paintPortLabels(painter, node, PortDirection::Output);
	if (!node.collapsed) paintNodeBody(painter, node, presentation.body);

	const bool selected = isSelected(node.id);
	const RGBA border = selected ? style.palette.accent : style.palette.border;
	painter.strokeRect(rect, border, radius, selected ? 2 : 1);
}

void NodeEditor::paintSimplifiedNode(Painter& painter, const Node& node, const NodePresentation& presentation) const {
	const Theme& style = theme();
	const RectF graphRect = nodeRect(node);
	const RectF rect = makeRect(toLocal(graphRect.pos), graphRect.size * zoom());
	const halnf header = mStyle.headerHeight * zoom();
	const RGBA accent = presentation.accent.value_or(mGraph.typeOf(node).headerColor);
	const bool selected = isSelected(node.id);

	if (detail() == Detail::Outline) {
		const halnf radius = std::min(mStyle.cornerRadius, rect.size.y / 2);
		painter.fillRect(rect, accent, radius);
		if (selected) painter.strokeRect(rect, style.palette.accent, radius, 2);
		paintShortTitle(painter, rect, presentation.shortTitle, style.palette.onAccent);
		return;
	}

	const halnf radius = mStyle.cornerRadius * zoom();
	painter.fillRect(rect, style.palette.raisedSurface, radius);
	painter.fillRect(makeRect(rect.pos, { rect.size.x, header }), accent, radius);

	const halnf padding = 10 * zoom();
	const halnf lineWidth = rect.size.x - padding * 2;
	const halnf titleWidth = textWidth(presentation.title, style.metrics.fontSize) * zoom();
	paintTextLine(
		painter, makeRect(rect.pos + Vec2F(padding, 0.0f), { lineWidth, header }), titleWidth, style.palette.onAccent
	);
	const halnf subtitleWidth = textWidth(presentation.subtitle, style.metrics.smallFontSize) * zoom();
	paintTextLine(
		painter,
		makeRect(rect.pos + Vec2F(padding, header), { lineWidth, mStyle.subtitleHeight * zoom() }),
		subtitleWidth,
		style.palette.mutedText
	);

	const halnf bodyTop = header + mStyle.subtitleHeight * zoom();
	if (rect.size.y > bodyTop) {
		const RectF body = makeRect(rect.pos + Vec2F(0.0f, bodyTop), { rect.size.x, rect.size.y - bodyTop });
		paintShortTitle(painter, body, presentation.shortTitle, style.palette.mutedText);
	}

	if (presentation.badge) {
		const halnf dot = std::max(2.0f, 4 * zoom());
		painter.fillCircle(rect.pos + Vec2F(rect.size.x - padding - dot, header / 2), dot, presentation.badge->color);
	}

	const RGBA border = selected ? style.palette.accent : style.palette.border;
	painter.strokeRect(rect, border, radius, selected ? 2 : 1);
}

void NodeEditor::paintShortTitle(
	Painter& painter, const RectF& rect, const std::string& text, const RGBA& color
) const {
	if (text.empty()) return;

	const halnf fontSize = std::min(kShortTitleFontSize, rect.size.y * 0.6f);
	if (fontSize < kMinimumShortTitleSize || textWidth(text, fontSize) > rect.size.x - 4) return;
	painter.text(text, rect, fontSize, TextAlign::Center, color);
}

void NodeEditor::paintTextLine(Painter& painter, const RectF& box, halnf width, const RGBA& color) const {
	if (width <= 0 || box.size.x <= 0) return;
	const halnf thickness = std::max(1.0f, box.size.y * 0.22f);
	RGBA faded = color;
	faded.a *= 0.55f;
	painter.fillRect(
		makeRect(box.pos.x, box.pos.y + (box.size.y - thickness) / 2, std::min(width, box.size.x), thickness),
		faded,
		thickness / 2
	);
}

halnf NodeEditor::paintBadge(Painter& painter, const RectF& header, const NodeBadge& badge) const {
	const Theme& style = theme();
	const halnf fontSize = style.metrics.smallFontSize * zoom();
	const halnf padding = 8 * zoom();
	const halnf dot = 4 * zoom();
	const halnf textWidthPx = textWidth(badge.text, fontSize);
	const halnf height = header.size.y * 0.62f;
	const halnf width = textWidthPx + dot * 2 + padding * 1.5f;
	const RectF pill = makeRect(
		{ header.pos.x + header.size.x - width - padding * 0.6f, header.pos.y + (header.size.y - height) / 2 },
		{ width, height }
	);

	painter.fillRect(pill, RGBA(0, 0, 0, 0.28f), height / 2);
	const Vec2F center = pill.pos + Vec2F(padding * 0.6f + dot, height / 2);
	const halnf pulse = badge.pulsing ? 0.55f + 0.45f * std::sin(halnf(mBadgeClockMs) / 260.0f) : 1.0f;
	painter.fillCircle(center, dot, RGBA(badge.color.r, badge.color.g, badge.color.b, badge.color.a * pulse));
	const RectF textBox =
		makeRect(pill.pos + Vec2F(padding * 0.6f + dot * 2 + padding * 0.4f, 0.0f), { textWidthPx + 2, height });
	painter.text(badge.text, textBox, fontSize, TextAlign::Start, style.palette.onAccent);
	return width + padding;
}

void NodeEditor::paintNodeBody(Painter& painter, const Node& node, const std::string& text) const {
	const RectF graphBody = bodyRect(node);
	if (graphBody.size.y <= 1) return;

	const Theme& style = theme();
	const RectF divider =
		makeRect(toLocal(graphBody.pos + Vec2F(10.0f, 0.0f)), Vec2F(graphBody.size.x - 20, 1.0f) * zoom());
	painter.fillRect(divider, style.palette.separator);
	if (text.empty()) return;
	const halnf padding = 10;
	const halnf fontSize = style.metrics.smallFontSize;
	const halnf lineHeight = style.metrics.lineHeight * fontSize / style.metrics.fontSize;
	const halnf width = graphBody.size.x - padding * 2;
	const std::vector<std::string> lines =
		wrapText(text, width, [&](const std::string& line) { return textWidth(line, fontSize); });

	const RectF local = makeRect(toLocal(graphBody.pos), graphBody.size * zoom());
	ScopedClip clip(painter, local);
	const halnf maximumLines = std::floor((graphBody.size.y - padding / 2) / lineHeight);
	for (size_t index = 0; index < lines.size() && halnf(index) < maximumLines; index++) {
		const Vec2F origin = graphBody.pos + Vec2F(padding, padding / 2 + halnf(index) * lineHeight);
		const RectF box = makeRect(toLocal(origin), Vec2F(width, lineHeight) * zoom());
		painter.text(lines[index], box, fontSize * zoom(), TextAlign::Start, style.palette.mutedText);
	}
}

void NodeEditor::paintPortHandles(Painter& painter, const Node& node) const {
	const halnf radius = mStyle.portRadius * zoom();

	for (PortDirection direction : { PortDirection::Input, PortDirection::Output }) {
		const std::vector<PortDeclaration>& ports = mGraph.typeOf(node).ports(direction);
		for (ualni index = 0; index < ports.size(); index++) {
			const PortRef ref = { node.id, direction, index };
			const Vec2F center = toLocal(portPosition(ref));
			const std::vector<Link> links = mGraph.linksAt(ref);
			const bool linked = std::any_of(links.begin(), links.end(), [this](const Link& link) { return isShown(link); });

			painter.fillCircle(center, radius, linked ? ports[index].color : theme().palette.window);
			painter.strokeCircle(center, radius, ports[index].color, 1.5f);
		}
	}
}

void NodeEditor::paintPortLabels(Painter& painter, const Node& node, PortDirection direction) const {
	const Theme& style = theme();
	const std::vector<PortDeclaration>& ports = mGraph.typeOf(node).ports(direction);
	const halnf fontSize = style.metrics.smallFontSize * zoom();
	const halnf labelWidth = node.width * 0.5f * zoom();
	const halnf rowHeight = mStyle.portRowHeight * zoom();
	const halnf inset = 12 * zoom();
	const bool input = direction == PortDirection::Input;

	for (ualni index = 0; index < ports.size(); index++) {
		const Vec2F center = toLocal(portPosition({ node.id, direction, index }));
		const RectF label = makeRect(
			{ input ? center.x + inset : center.x - inset - labelWidth, center.y - rowHeight / 2 }, { labelWidth, rowHeight }
		);
		painter.text(ports[index].name, label, fontSize, input ? TextAlign::Start : TextAlign::End, style.palette.text);
	}
}

void NodeEditor::paintMarquee(Painter& painter) const {
	const RectF area = marqueeRect();
	const RectF local = makeRect(toLocal(area.pos), area.size * zoom());
	RGBA fill = theme().palette.accent;
	fill.a = 0.15f;

	painter.fillRect(local, fill, theme().metrics.indicatorCornerRadius);
	painter.strokeRect(local, theme().palette.accent, theme().metrics.indicatorCornerRadius, 1);
}
