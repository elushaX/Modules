#include "NodeGraph.hpp"

#include <algorithm>
#include <unordered_map>

using namespace tp;
using namespace tp::ui;

Vec2F NodeGraphFragment::origin() const {
	if (nodes.empty()) return { 0, 0 };

	Vec2F origin = nodes.front().position;
	for (const Node& node : nodes) {
		origin.x = std::min(origin.x, node.position.x);
		origin.y = std::min(origin.y, node.position.y);
	}
	return origin;
}

NodeTypeId NodeGraph::registerType(NodeType type) {
	mTypes.push_back(std::move(type));
	return mTypes.size() - 1;
}

NodeId NodeGraph::addNode(NodeTypeId type, Vec2F position, std::any payload) {
	const NodeId id = mNextId++;
	mNodes.push_back(
		{ id, type, position, std::move(payload), mTypes.at(type).width, mTypes.at(type).bodyHeight, false }
	);
	changed();
	return id;
}

bool NodeGraph::addNodeWithId(NodeId id, NodeTypeId type, Vec2F position, std::any payload) {
	if (id == 0 || findNode(id)) return false;

	mNodes.push_back(
		{ id, type, position, std::move(payload), mTypes.at(type).width, mTypes.at(type).bodyHeight, false }
	);
	mNextId = std::max(mNextId, id + 1);
	changed();
	return true;
}

bool NodeGraph::renameNode(NodeId from, NodeId to) {
	Node* node = findNode(from);
	if (!node || to == 0 || (from != to && findNode(to))) return false;
	if (from == to) return true;

	node->id = to;
	for (Link& link : mLinks) {
		if (link.from.node == from) link.from.node = to;
		if (link.to.node == from) link.to.node = to;
	}
	mNextId = std::max(mNextId, to + 1);
	changed();
	return true;
}

void NodeGraph::clear() {
	mNodes.clear();
	mLinks.clear();
	changed();
}

void NodeGraph::removeNode(NodeId id) { removeNodes({ id }); }

void NodeGraph::removeNodes(const std::vector<NodeId>& ids) {
	auto isRemoved = [&](NodeId id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); };

	std::erase_if(mLinks, [&](const Link& link) { return isRemoved(link.from.node) || isRemoved(link.to.node); });
	std::erase_if(mNodes, [&](const Node& node) { return isRemoved(node.id); });
	if (mObserver.nodesRemoved && !ids.empty()) mObserver.nodesRemoved(ids);
	changed();
}

void NodeGraph::removeLinksWhere(const std::function<bool(const Link&)>& matches) {
	std::vector<Link> removed;
	for (const Link& link : mLinks) {
		if (matches(link)) removed.push_back(link);
	}
	std::erase_if(mLinks, matches);
	for (const Link& link : removed) {
		if (mObserver.linkRemoved) mObserver.linkRemoved(link);
	}
}

void NodeGraph::moveNode(NodeId id, Vec2F position) {
	Node* node = findNode(id);
	if (!node) return;

	node->position = position;
	changed();
}

void NodeGraph::resizeNode(NodeId id, halnf width, halnf bodyHeight) {
	Node* node = findNode(id);
	if (!node) return;

	node->width = width;
	node->bodyHeight = bodyHeight;
	changed();
}

void NodeGraph::setCollapsed(NodeId id, bool collapsed) {
	Node* node = findNode(id);
	if (!node || node->collapsed == collapsed) return;

	node->collapsed = collapsed;
	changed();
}

void NodeGraph::bringToFront(NodeId id) {
	auto found = std::find_if(mNodes.begin(), mNodes.end(), [&](const Node& node) { return node.id == id; });
	if (found == mNodes.end()) return;

	std::rotate(found, found + 1, mNodes.end());
}

Node* NodeGraph::findNode(NodeId id) {
	auto found = std::find_if(mNodes.begin(), mNodes.end(), [&](const Node& node) { return node.id == id; });
	return found == mNodes.end() ? nullptr : &*found;
}

const Node* NodeGraph::findNode(NodeId id) const { return const_cast<NodeGraph*>(this)->findNode(id); }

const PortDeclaration* NodeGraph::port(const PortRef& ref) const {
	const Node* node = findNode(ref.node);
	if (!node) return nullptr;

	const std::vector<PortDeclaration>& ports = typeOf(*node).ports(ref.direction);
	return ref.index < ports.size() ? &ports[ref.index] : nullptr;
}

Link NodeGraph::orientedLink(const PortRef& first, const PortRef& second) {
	if (first.direction == PortDirection::Output) return { first, second };
	return { second, first };
}

bool NodeGraph::isStructurallyValid(const Link& link) const {
	if (link.from.direction != PortDirection::Output || link.to.direction != PortDirection::Input) return false;
	if (link.from.node == link.to.node) return false;

	const PortDeclaration* from = port(link.from);
	const PortDeclaration* to = port(link.to);
	return from && to && from->kind == to->kind;
}

bool NodeGraph::canConnect(const PortRef& first, const PortRef& second) const {
	const Link link = orientedLink(first, second);

	if (!isStructurallyValid(link) || hasLink(link)) return false;
	return !mLinkValidator || mLinkValidator(*this, link);
}

bool NodeGraph::connect(const PortRef& first, const PortRef& second) {
	if (!canConnect(first, second)) return false;

	const Link link = orientedLink(first, second);
	if (!port(link.to)->allowsMultipleLinks) disconnectPort(link.to);
	if (!port(link.from)->allowsMultipleLinks) disconnectPort(link.from);

	mLinks.push_back(link);
	if (mObserver.linkAdded) mObserver.linkAdded(link);
	changed();
	return true;
}

void NodeGraph::disconnect(const Link& link) {
	removeLinksWhere([&](const Link& candidate) { return candidate == link; });
	changed();
}

void NodeGraph::disconnectPort(const PortRef& port) {
	removeLinksWhere([&](const Link& link) { return link.from == port || link.to == port; });
	changed();
}

std::vector<Link> NodeGraph::linksAt(const PortRef& port) const {
	std::vector<Link> found;
	for (const Link& link : mLinks) {
		if (link.from == port || link.to == port) found.push_back(link);
	}
	return found;
}

bool NodeGraph::hasLink(const Link& link) const {
	return std::find(mLinks.begin(), mLinks.end(), link) != mLinks.end();
}

NodeGraphFragment NodeGraph::extract(const std::vector<NodeId>& ids) const {
	NodeGraphFragment fragment;
	auto isExtracted = [&](NodeId id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); };

	for (const Node& node : mNodes) {
		if (isExtracted(node.id)) fragment.nodes.push_back(node);
	}
	for (const Link& link : mLinks) {
		if (isExtracted(link.from.node) && isExtracted(link.to.node)) fragment.links.push_back(link);
	}
	return fragment;
}

std::vector<NodeId> NodeGraph::insert(const NodeGraphFragment& fragment, Vec2F offset) {
	std::unordered_map<NodeId, NodeId> remapped;
	std::vector<NodeId> inserted;

	for (const Node& node : fragment.nodes) {
		const NodeId id = mNextId++;
		Node copy = node;
		copy.id = id;
		copy.position = node.position + offset;
		mNodes.push_back(std::move(copy));
		remapped[node.id] = id;
		inserted.push_back(id);
	}

	for (Link link : fragment.links) {
		link.from.node = remapped.at(link.from.node);
		link.to.node = remapped.at(link.to.node);
		mLinks.push_back(link);
	}

	changed();
	return inserted;
}

void NodeGraph::changed() {
	if (mOnChanged) mOnChanged();
}
