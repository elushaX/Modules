#pragma once

#include "Color.hpp"
#include "Geometry.hpp"

#include <any>
#include <functional>
#include <string>
#include <vector>

namespace tp::ui {

	using NodeId = ualni;
	using NodeTypeId = ualni;

	enum class PortDirection { Input, Output };

	struct PortDeclaration {
		std::string name;
		std::string kind;
		RGBA color = { 0.62f, 0.63f, 0.66f, 1.0f };
		bool allowsMultipleLinks = true;
	};

	struct NodeType {
		std::string name;
		RGBA headerColor = { 0.25f, 0.27f, 0.31f, 1.0f };
		halnf width = 200;
		std::vector<PortDeclaration> inputs;
		std::vector<PortDeclaration> outputs;
		halnf bodyHeight = 40;

		[[nodiscard]] const std::vector<PortDeclaration>& ports(PortDirection direction) const {
			return direction == PortDirection::Input ? inputs : outputs;
		}
	};

	struct PortRef {
		NodeId node = 0;
		PortDirection direction = PortDirection::Output;
		ualni index = 0;

		bool operator==(const PortRef& other) const = default;
	};

	struct Link {
		PortRef from;
		PortRef to;

		bool operator==(const Link& other) const = default;
	};

	struct Node {
		NodeId id = 0;
		NodeTypeId type = 0;
		Vec2F position = { 0, 0 };
		std::any payload;
		halnf width = 0;
		halnf bodyHeight = 0;
		bool collapsed = false;
	};

	struct NodeGraphFragment {
		std::vector<Node> nodes;
		std::vector<Link> links;

		[[nodiscard]] bool isEmpty() const { return nodes.empty(); }
		[[nodiscard]] Vec2F origin() const;
	};

	class NodeGraph {
	public:
		using LinkValidator = std::function<bool(const NodeGraph&, const Link&)>;

		NodeTypeId registerType(NodeType type);
		[[nodiscard]] const NodeType& type(NodeTypeId id) const { return mTypes.at(id); }
		[[nodiscard]] const NodeType& typeOf(const Node& node) const { return type(node.type); }

		NodeId addNode(NodeTypeId type, Vec2F position, std::any payload = {});
		bool addNodeWithId(NodeId id, NodeTypeId type, Vec2F position, std::any payload = {});
		// Re-keys a node and every link touching it; fails if `from` is missing or `to` is taken.
		bool renameNode(NodeId from, NodeId to);
		void clear();
		void removeNode(NodeId id);
		void removeNodes(const std::vector<NodeId>& ids);
		void moveNode(NodeId id, Vec2F position);
		void resizeNode(NodeId id, halnf width, halnf bodyHeight);
		void setCollapsed(NodeId id, bool collapsed);
		void bringToFront(NodeId id);

		[[nodiscard]] Node* findNode(NodeId id);
		[[nodiscard]] const Node* findNode(NodeId id) const;
		[[nodiscard]] const std::vector<Node>& nodes() const { return mNodes; }

		void setLinkValidator(LinkValidator validator) { mLinkValidator = std::move(validator); }
		[[nodiscard]] const PortDeclaration* port(const PortRef& ref) const;
		[[nodiscard]] bool canConnect(const PortRef& first, const PortRef& second) const;
		bool connect(const PortRef& first, const PortRef& second);
		void disconnect(const Link& link);
		void disconnectPort(const PortRef& port);

		[[nodiscard]] const std::vector<Link>& links() const { return mLinks; }
		[[nodiscard]] std::vector<Link> linksAt(const PortRef& port) const;
		[[nodiscard]] bool hasLink(const Link& link) const;

		[[nodiscard]] NodeGraphFragment extract(const std::vector<NodeId>& ids) const;
		std::vector<NodeId> insert(const NodeGraphFragment& fragment, Vec2F offset);

		struct Observer {
			std::function<void(NodeId)> nodeAdded;
			std::function<void(const std::vector<NodeId>&)> nodesRemoved;
			std::function<void(const Link&)> linkAdded;
			std::function<void(const Link&)> linkRemoved;
		};

		void setOnChanged(std::function<void()> onChanged) { mOnChanged = std::move(onChanged); }
		void setObserver(Observer observer) { mObserver = std::move(observer); }

	private:
		[[nodiscard]] static Link orientedLink(const PortRef& first, const PortRef& second);
		[[nodiscard]] bool isStructurallyValid(const Link& link) const;
		void changed();
		void removeLinksWhere(const std::function<bool(const Link&)>& matches);

		std::vector<NodeType> mTypes;
		std::vector<Node> mNodes;
		std::vector<Link> mLinks;
		NodeId mNextId = 1;
		LinkValidator mLinkValidator;
		std::function<void()> mOnChanged;
		Observer mObserver;
	};
}
