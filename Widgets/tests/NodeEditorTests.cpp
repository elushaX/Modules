#include "NodeEditor.hpp"
#include "NodeGraph.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "RecordingPainter.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	NodeType flowType() {
		NodeType type;
		type.name = "Step";
		type.inputs = { { "after", "flow" } };
		type.outputs = { { "then", "flow" } };
		return type;
	}

	struct EditorScene {
		NodeGraph graph;
		NodeTypeId step = graph.registerType(flowType());
		Widget root;
		NodeEditor editor{ graph };

		EditorScene() {
			root.emplaceLayout<StackLayout>();
			root.addChild(editor);
		}

		Vec2F windowPointOf(Vec2F graphPoint) { return editor.windowPosition() + editor.toLocal(graphPoint); }
		Vec2F headerOf(NodeId id) { return windowPointOf(graph.findNode(id)->position + Vec2F(60.0f, 10.0f)); }
		Vec2F bottomRightOf(NodeId id) {
			const RectF rect = editor.nodeRect(*graph.findNode(id));
			return windowPointOf(rect.pos + rect.size - Vec2F(2.0f, 2.0f));
		}
		Vec2F rightEdgeOf(NodeId id) {
			const RectF rect = editor.nodeRect(*graph.findNode(id));
			return windowPointOf(rect.pos + Vec2F(rect.size.x - 2, editor.style().headerHeight / 2));
		}
		void doubleClick(UiDriver& driver, Vec2F point) {
			driver.click(point);
			driver.click(point);
		}
		Vec2F portOf(NodeId id, PortDirection direction) {
			return windowPointOf(editor.portPosition({ id, direction, 0 }));
		}
	};
}

SUITE(NodeGraphModel) {
	TEST(ConnectOrientsOutputToInput) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });

		CHECK(scene.graph.connect({ second, PortDirection::Input, 0 }, { first, PortDirection::Output, 0 }));
		REQUIRE CHECK_EQUAL(1u, scene.graph.links().size());
		CHECK_EQUAL(first, scene.graph.links()[0].from.node);
		CHECK_EQUAL(second, scene.graph.links()[0].to.node);
	}

	TEST(RejectsSelfDuplicateAndSameDirectionLinks) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });

		CHECK(!scene.graph.connect({ first, PortDirection::Output, 0 }, { first, PortDirection::Input, 0 }));
		CHECK(!scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Output, 0 }));
		CHECK(scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }));
		CHECK(!scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }));
	}

	TEST(ValidatorCanVetoLinks) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });
		scene.graph.setLinkValidator([](const NodeGraph&, const Link&) { return false; });

		CHECK(!scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }));
	}

	TEST(RemovingNodeRemovesItsLinks) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });
		scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 });

		scene.graph.removeNode(first);
		CHECK_EQUAL(1u, scene.graph.nodes().size());
		CHECK_EQUAL(0u, scene.graph.links().size());
	}
}

SUITE(NodeGraphExternalIds) {
	TEST(NodesCanUseExternalIdsAndGeneratedIdsSkipThem) {
		EditorScene scene;
		CHECK(scene.graph.addNodeWithId(42, scene.step, { 0, 0 }));
		CHECK(!scene.graph.addNodeWithId(42, scene.step, { 0, 0 }));
		CHECK(!scene.graph.addNodeWithId(0, scene.step, { 0, 0 }));
		CHECK_EQUAL(43u, scene.graph.addNode(scene.step, { 0, 0 }));
	}

	TEST(ClearRemovesNodesAndLinks) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });
		scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 });

		scene.graph.clear();
		CHECK_EQUAL(0u, scene.graph.nodes().size());
		CHECK_EQUAL(0u, scene.graph.links().size());
	}
}

SUITE(NodeEditorPinch) {
	TouchGesture pinchUpdate(Vec2F center, Vec2F pan, halnf scale) {
		TouchGesture gesture;
		gesture.phase = GesturePhase::Update;
		gesture.center = center;
		gesture.pan = pan;
		gesture.scale = scale;
		return gesture;
	}

	TEST(PinchZoomKeepsGraphPointUnderFingers) {
		EditorScene scene;
		UiDriver driver(scene.root);
		const Vec2F fingers = scene.editor.windowPosition() + Vec2F(300.0f, 200.0f);
		const Vec2F graphBefore = scene.editor.toGraph(fingers - scene.editor.windowPosition());

		driver.gesture({ GesturePhase::Begin, fingers });
		driver.gesture(pinchUpdate(fingers, { 0, 0 }, 1.5f));
		driver.gesture(pinchUpdate(fingers, { 0, 0 }, 1.2f));
		driver.gesture({ GesturePhase::End, fingers });

		CHECK_CLOSE(1.8f, scene.editor.zoom(), 0.001f);
		const Vec2F graphAfter = scene.editor.toGraph(fingers - scene.editor.windowPosition());
		CHECK_CLOSE(graphBefore.x, graphAfter.x, 0.01f);
		CHECK_CLOSE(graphBefore.y, graphAfter.y, 0.01f);
	}

	TEST(TwoFingerPanMovesViewWithFingers) {
		EditorScene scene;
		UiDriver driver(scene.root);
		const Vec2F start = scene.editor.windowPosition() + Vec2F(300.0f, 200.0f);
		const Vec2F graphUnderFingers = scene.editor.toGraph(start - scene.editor.windowPosition());

		driver.gesture({ GesturePhase::Begin, start });
		driver.gesture(pinchUpdate(start + Vec2F(40.0f, 0.0f), { 40, 0 }, 1));
		driver.gesture(pinchUpdate(start + Vec2F(40.0f, 25.0f), { 0, 25 }, 1));
		driver.gesture({ GesturePhase::End, start + Vec2F(40.0f, 25.0f) });

		CHECK_CLOSE(1, scene.editor.zoom(), 0.001f);
		const Vec2F end = start + Vec2F(40.0f, 25.0f);
		const Vec2F graphAfter = scene.editor.toGraph(end - scene.editor.windowPosition());
		CHECK_CLOSE(graphUnderFingers.x, graphAfter.x, 0.01f);
		CHECK_CLOSE(graphUnderFingers.y, graphAfter.y, 0.01f);
	}

	TEST(PinchZoomIsClampedAndDoesNotDrift) {
		EditorScene scene;
		UiDriver driver(scene.root);
		const Vec2F fingers = scene.editor.windowPosition() + Vec2F(200.0f, 150.0f);
		const Vec2F graphBefore = scene.editor.toGraph(fingers - scene.editor.windowPosition());

		driver.gesture({ GesturePhase::Begin, fingers });
		for (int step = 0; step < 40; step++)
			driver.gesture(pinchUpdate(fingers, { 0, 0 }, 1.3f));
		CHECK_CLOSE(scene.editor.style().maximumZoom, scene.editor.zoom(), 0.001f);
		for (int step = 0; step < 80; step++)
			driver.gesture(pinchUpdate(fingers, { 0, 0 }, 0.7f));
		driver.gesture({ GesturePhase::End, fingers });

		CHECK_CLOSE(scene.editor.style().minimumZoom, scene.editor.zoom(), 0.001f);
		const Vec2F graphAfter = scene.editor.toGraph(fingers - scene.editor.windowPosition());
		CHECK_CLOSE(graphBefore.x, graphAfter.x, 0.05f);
		CHECK_CLOSE(graphBefore.y, graphAfter.y, 0.05f);
	}

	TEST(PinchDoesNotMoveOrSelectNodes) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 100, 100 });
		UiDriver driver(scene.root);
		const Vec2F onNode = scene.headerOf(id);

		driver.gesture({ GesturePhase::Begin, onNode });
		driver.gesture(pinchUpdate(onNode + Vec2F(30.0f, 0.0f), { 30, 0 }, 1.4f));
		driver.gesture({ GesturePhase::End, onNode });

		CHECK(scene.graph.findNode(id)->position == Vec2F(100.0f, 100.0f));
		CHECK(scene.editor.selection().empty());
	}
}

SUITE(NodeGraphRename) {
	TEST(RenameKeepsLinksAndRejectsTakenIds) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId second = scene.graph.addNode(scene.step, { 300, 0 });
		REQUIRE CHECK(scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }));

		CHECK(!scene.graph.renameNode(first, second));
		CHECK(!scene.graph.renameNode(999, 1000));
		REQUIRE CHECK(scene.graph.renameNode(first, 500));

		CHECK(scene.graph.findNode(first) == nullptr);
		CHECK(scene.graph.findNode(500) != nullptr);
		REQUIRE CHECK_EQUAL(1u, scene.graph.links().size());
		CHECK_EQUAL(500u, scene.graph.links().front().from.node);
		CHECK(scene.graph.addNode(scene.step, { 0, 0 }) > 500);
	}
}

SUITE(NodeLinkDropOnEmpty) {
	TEST(DroppingLinkOnEmptySpaceReportsSourcePortAndPoint) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		std::optional<std::pair<PortRef, Vec2F>> dropped;
		scene.editor.setOnLinkDroppedOnEmpty([&](const PortRef& from, Vec2F point) {
			dropped = std::make_pair(from, point);
		});
		UiDriver driver(scene.root);

		const Vec2F target = scene.windowPointOf({ 500, 300 });
		driver.drag(scene.portOf(id, PortDirection::Output), target, 6);

		REQUIRE CHECK(dropped.has_value());
		CHECK((dropped->first == PortRef{ id, PortDirection::Output, 0 }));
		CHECK_CLOSE(500, dropped->second.x, 0.5f);
		CHECK_CLOSE(300, dropped->second.y, 0.5f);
		CHECK_EQUAL(0u, scene.graph.links().size());
	}

	TEST(DroppingOnPortConnectsAndDoesNotReportEmpty) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		bool reported = false;
		scene.editor.setOnLinkDroppedOnEmpty([&](const PortRef&, Vec2F) { reported = true; });
		UiDriver driver(scene.root);

		driver.drag(scene.portOf(first, PortDirection::Output), scene.portOf(second, PortDirection::Input), 6);
		CHECK_EQUAL(1u, scene.graph.links().size());
		CHECK(!reported);
	}

	TEST(DroppingOnNodeBodyOrTappingPortDoesNotReport) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		int reports = 0;
		scene.editor.setOnLinkDroppedOnEmpty([&](const PortRef&, Vec2F) { reports++; });
		UiDriver driver(scene.root);

		driver.drag(scene.portOf(first, PortDirection::Output), scene.headerOf(second), 6);
		driver.click(scene.portOf(first, PortDirection::Output));
		CHECK_EQUAL(0, reports);
	}
}

SUITE(NodeCollapseAndResize) {
	TEST(DoubleClickOnHeaderTogglesOnlyTheBody) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);
		const Vec2F portBefore = scene.editor.portPosition({ id, PortDirection::Output, 0 });
		const halnf expandedHeight = scene.editor.nodeRect(node).size.y;

		scene.doubleClick(driver, scene.headerOf(id));
		REQUIRE CHECK(scene.graph.findNode(id)->collapsed);
		CHECK_CLOSE(expandedHeight - node.bodyHeight, scene.editor.nodeRect(node).size.y, 0.01f);
		CHECK_CLOSE(0, scene.editor.bodyRect(node).size.y, 0.01f);
		CHECK(scene.editor.portPosition({ id, PortDirection::Output, 0 }) == portBefore);

		scene.doubleClick(driver, scene.headerOf(id));
		CHECK(!scene.graph.findNode(id)->collapsed);
		CHECK_CLOSE(expandedHeight, scene.editor.nodeRect(node).size.y, 0.01f);
	}

	TEST(DoubleClickOutsideHeaderOpensNodeInsteadOfToggling) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		NodeId opened = 0;
		scene.editor.setOnNodeOpened([&](NodeId node) { opened = node; });
		UiDriver driver(scene.root);

		const RectF body = scene.editor.bodyRect(*scene.graph.findNode(id));
		scene.doubleClick(driver, scene.windowPointOf(body.pos + body.size / 2.0f));
		CHECK_EQUAL(id, opened);
		CHECK(!scene.graph.findNode(id)->collapsed);
	}

	TEST(PortsStayConnectableWhileCollapsed) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		scene.graph.setCollapsed(first, true);
		scene.graph.setCollapsed(second, true);
		UiDriver driver(scene.root);

		driver.drag(scene.portOf(first, PortDirection::Output), scene.portOf(second, PortDirection::Input), 6);
		CHECK_EQUAL(1u, scene.graph.links().size());
	}

	TEST(BodyTextIsPaintedOnlyWhenExpanded) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.editor.setPresenter([](const Node&) {
			return NodeEditor::NodePresentation{ "Title", "status", std::nullopt, "Describe the work" };
		});
		UiDriver driver(scene.root);

		CHECK(driver.paint().findText("Describe the work") != nullptr);
		CHECK(driver.paint().findText("status") != nullptr);
		scene.graph.setCollapsed(id, true);
		driver.settle();
		CHECK(driver.paint().findText("Describe the work") == nullptr);
		CHECK(driver.paint().findText("status") != nullptr);
	}

	TEST(LongBodyWrapsAndIsClippedToBodyHeight) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.editor.setPresenter([](const Node&) {
			return NodeEditor::NodePresentation{ "T", "", std::nullopt, std::string(60, 'w') + " " + std::string(400, 'z') };
		});
		UiDriver driver(scene.root);

		const RectF body = scene.editor.bodyRect(*scene.graph.findNode(id));
		const RectF localBody = makeRect(scene.windowPointOf(body.pos), body.size);
		int lines = 0;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (command.kind != PaintCommand::Kind::Text || command.text.empty() || command.text[0] == 'T') continue;
			if (command.text[0] != 'w' && command.text[0] != 'z') continue;
			lines++;
			CHECK(command.rect.pos.y >= localBody.pos.y - 0.5f);
			CHECK(command.rect.pos.y + command.rect.size.y <= localBody.pos.y + localBody.size.y + 0.5f);
		}
		CHECK(lines > 1);
	}

	TEST(DraggingCornerResizesWidthAndBodyHeight) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);
		const halnf width = node.width;
		const halnf body = node.bodyHeight;

		driver.drag(scene.bottomRightOf(id), scene.bottomRightOf(id) + Vec2F(60.0f, 40.0f), 6);
		CHECK_CLOSE(width + 60, node.width, 0.5f);
		CHECK_CLOSE(body + 40, node.bodyHeight, 0.5f);
		CHECK(node.position == Vec2F(40.0f, 40.0f));
	}

	TEST(CollapsedNodeResizesOnlyWidth) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.graph.setCollapsed(id, true);
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);
		const halnf width = node.width;
		const halnf body = node.bodyHeight;
		const halnf height = scene.editor.nodeRect(node).size.y;

		driver.drag(scene.rightEdgeOf(id), scene.rightEdgeOf(id) + Vec2F(50.0f, 80.0f), 6);
		CHECK_CLOSE(width + 50, node.width, 0.5f);
		CHECK_CLOSE(body, node.bodyHeight, 0.01f);
		CHECK_CLOSE(height, scene.editor.nodeRect(node).size.y, 0.01f);

		scene.graph.setCollapsed(id, false);
		CHECK_CLOSE(body, node.bodyHeight, 0.01f);
	}

	TEST(ResizeRespectsMinimums) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);

		driver.drag(scene.bottomRightOf(id), scene.bottomRightOf(id) - Vec2F(500.0f, 500.0f), 6);
		CHECK_CLOSE(scene.editor.style().minimumWidth, node.width, 0.5f);
		CHECK_CLOSE(scene.editor.style().minimumBodyHeight, node.bodyHeight, 0.5f);
	}

	TEST(RightEdgeDragResizesWidthWithoutMovingNode) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);
		const halnf body = node.bodyHeight;

		driver.drag(scene.rightEdgeOf(id), scene.rightEdgeOf(id) + Vec2F(30.0f, 25.0f), 6);
		CHECK_CLOSE(scene.graph.type(scene.step).width + 30, node.width, 0.5f);
		CHECK_CLOSE(body, node.bodyHeight, 0.01f);
		CHECK(node.position == Vec2F(40.0f, 40.0f));
	}

	TEST(CollapsedBottomCornerDoesNotResizeHeight) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.graph.setCollapsed(id, true);
		UiDriver driver(scene.root);
		const Node& node = *scene.graph.findNode(id);

		const Vec2F corner = scene.bottomRightOf(id);
		CHECK(
			scene.editor.resizeEdgesAt(node, scene.editor.toGraph(corner - scene.editor.windowPosition())) ==
			NodeEditor::ResizeEdges::Width
		);
	}

	TEST(ResizedPortsFollowNewWidth) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.graph.resizeNode(id, 300, 50);
		UiDriver driver(scene.root);

		CHECK_CLOSE(340, scene.editor.portPosition({ id, PortDirection::Output, 0 }).x, 0.01f);
	}

	TEST(SizeAndCollapseSurviveCopyPaste) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		scene.graph.resizeNode(id, 310, 77);
		scene.graph.setCollapsed(id, true);
		UiDriver driver(scene.root);

		scene.editor.select({ id });
		scene.editor.execute(NodeEditor::Command::Duplicate);
		REQUIRE CHECK_EQUAL(1u, scene.editor.selection().size());
		const Node& copy = *scene.graph.findNode(scene.editor.selection().front());
		CHECK_CLOSE(310, copy.width, 0.01f);
		CHECK_CLOSE(77, copy.bodyHeight, 0.01f);
		CHECK(copy.collapsed);
	}
}

SUITE(NodeEditorInteraction) {
	TEST(LinksAndPortsPaintAboveNodeBodies) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 });
		UiDriver driver(scene.root);

		const std::vector<PaintCommand>& commands = driver.paint().commands();
		size_t lastBorder = 0;
		size_t firstLink = commands.size();
		size_t firstPort = commands.size();
		for (size_t index = 0; index < commands.size(); index++) {
			const PaintCommand::Kind kind = commands[index].kind;
			if (kind == PaintCommand::Kind::StrokeRect) lastBorder = index;
			if (kind == PaintCommand::Kind::Polyline) firstLink = std::min(firstLink, index);
			if (kind == PaintCommand::Kind::Circle) firstPort = std::min(firstPort, index);
		}

		CHECK(lastBorder < firstLink);
		CHECK(firstLink < firstPort);
	}

	TEST(DraggingHeaderMovesNode) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);

		driver.drag(scene.headerOf(id), scene.headerOf(id) + Vec2F(100.0f, 50.0f), 6);
		CHECK_CLOSE(140, scene.graph.findNode(id)->position.x, 0.5f);
		CHECK_CLOSE(90, scene.graph.findNode(id)->position.y, 0.5f);
		CHECK(scene.editor.isSelected(id));
	}

	TEST(DraggingBetweenPortsCreatesLink) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		UiDriver driver(scene.root);

		driver.drag(scene.portOf(first, PortDirection::Output), scene.portOf(second, PortDirection::Input), 6);
		REQUIRE CHECK_EQUAL(1u, scene.graph.links().size());
		CHECK_EQUAL(first, scene.graph.links()[0].from.node);
	}

	TEST(DraggingLinkedInputAwayDisconnects) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 });
		UiDriver driver(scene.root);

		driver.drag(
			scene.portOf(second, PortDirection::Input), scene.portOf(second, PortDirection::Input) + Vec2F(0.0f, 300.0f), 6
		);
		CHECK_EQUAL(0u, scene.graph.links().size());
	}

	TEST(MarqueeSelectsEnclosedNodes) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		scene.graph.addNode(scene.step, { 500, 400 });
		UiDriver driver(scene.root);

		driver.drag(scene.windowPointOf({ 10, 10 }), scene.windowPointOf({ 300, 200 }), 6);
		REQUIRE CHECK_EQUAL(1u, scene.editor.selection().size());
		CHECK_EQUAL(first, scene.editor.selection()[0]);
	}

	TEST(TapOnEmptySpaceClearsSelectionWithoutMarquee) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);
		scene.editor.select({ id });

		const Vec2F empty = scene.windowPointOf({ 500, 400 });
		driver.moveTo(empty);
		const size_t idleCommands = driver.paint().commands().size();
		driver.press();
		CHECK_EQUAL(idleCommands, driver.paint().commands().size());
		driver.release();

		CHECK_EQUAL(0u, scene.editor.selection().size());
	}

	TEST(DuplicateCopiesSelectedNodesWithInternalLinks) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 40, 40 });
		const NodeId second = scene.graph.addNode(scene.step, { 400, 40 });
		scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 });
		UiDriver driver(scene.root);

		scene.editor.select({ first, second });
		driver.click(scene.windowPointOf({ 300, 500 }));
		scene.editor.select({ first, second });
		driver.key(InputID::D, { .control = true });

		CHECK_EQUAL(4u, scene.graph.nodes().size());
		CHECK_EQUAL(2u, scene.graph.links().size());
		CHECK_EQUAL(2u, scene.editor.selection().size());
		CHECK(!scene.editor.isSelected(first));
	}

	TEST(CutThenPasteRestoresNodes) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);

		driver.click(scene.headerOf(id));
		driver.key(InputID::X, { .control = true });
		CHECK_EQUAL(0u, scene.graph.nodes().size());

		driver.key(InputID::V, { .control = true });
		CHECK_EQUAL(1u, scene.graph.nodes().size());
		CHECK(scene.editor.hasClipboard());
	}

	TEST(DeleteKeyRemovesSelection) {
		EditorScene scene;
		const NodeId id = scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);

		driver.click(scene.headerOf(id));
		driver.key(InputID::DELETE_KEY);
		CHECK_EQUAL(0u, scene.graph.nodes().size());
	}

	TEST(ViewChangesAnimateWithSpring) {
		EditorScene scene;
		UiDriver driver(scene.root);
		driver.setAutoSettle(false);

		scene.editor.setView({ 200, 0 }, 2);
		driver.advance(16);
		CHECK(scene.editor.pan().x > 0.0f && scene.editor.pan().x < 200.0f);
		CHECK(scene.editor.zoom() > 1.0f && scene.editor.zoom() < 2.0f);

		driver.settle();
		CHECK_CLOSE(200, scene.editor.pan().x, 0.5f);
		CHECK_CLOSE(2, scene.editor.zoom(), 0.01f);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(WheelZoomKeepsPointerAnchoredWhileAnimating) {
		EditorScene scene;
		UiDriver driver(scene.root);
		driver.setAutoSettle(false);

		const Vec2F anchor = scene.windowPointOf({ 100, 100 });
		driver.moveTo(anchor);
		driver.wheel(3);
		driver.advance(16);

		const Vec2F graphPoint = scene.editor.toGraph(anchor - scene.editor.windowPosition());
		CHECK_CLOSE(100, graphPoint.x, 0.5f);
		CHECK_CLOSE(100, graphPoint.y, 0.5f);
	}

	TEST(WheelZoomKeepsPointerAnchored) {
		EditorScene scene;
		scene.graph.addNode(scene.step, { 40, 40 });
		UiDriver driver(scene.root);

		const Vec2F anchor = scene.windowPointOf({ 100, 100 });
		driver.moveTo(anchor);
		driver.wheel(3);
		driver.settle();

		CHECK(scene.editor.zoom() > 1.0f);
		const Vec2F graphPoint = scene.editor.toGraph(anchor - scene.editor.windowPosition());
		CHECK_CLOSE(100, graphPoint.x, 0.5f);
		CHECK_CLOSE(100, graphPoint.y, 0.5f);
	}
}

SUITE(NodeLinkSelection) {
	struct LinkedScene : EditorScene {
		NodeId first = graph.addNode(step, { 40, 40 });
		NodeId second = graph.addNode(step, { 400, 40 });
		LinkedScene() { graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }); }
		Vec2F midLink() {
			const Vec2F from = editor.portPosition({ first, PortDirection::Output, 0 });
			const Vec2F to = editor.portPosition({ second, PortDirection::Input, 0 });
			return windowPointOf((from + to) / 2.0f);
		}
	};

	TEST(ClickingAConnectionSelectsItAndClearsNodeSelection) {
		LinkedScene scene;
		UiDriver driver(scene.root);
		scene.editor.select({ scene.first });

		driver.click(scene.midLink());

		REQUIRE CHECK(scene.editor.selectedLink().has_value());
		CHECK_EQUAL(scene.first, scene.editor.selectedLink()->from.node);
		CHECK_EQUAL(scene.second, scene.editor.selectedLink()->to.node);
		CHECK(scene.editor.selection().empty());
	}

	TEST(DeleteRemovesSelectedConnectionButKeepsNodes) {
		LinkedScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.midLink());
		REQUIRE CHECK(scene.editor.selectedLink().has_value());
		driver.key(InputID::DELETE_KEY);

		CHECK_EQUAL(0u, scene.graph.links().size());
		CHECK_EQUAL(2u, scene.graph.nodes().size());
		CHECK(!scene.editor.selectedLink().has_value());
	}

	TEST(SelectingANodeClearsLinkSelection) {
		LinkedScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.midLink());
		REQUIRE CHECK(scene.editor.selectedLink().has_value());
		driver.click(scene.headerOf(scene.first));

		CHECK(!scene.editor.selectedLink().has_value());
		CHECK(scene.editor.isSelected(scene.first));
	}

	TEST(ClickingEmptySpaceDeselectsTheConnection) {
		LinkedScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.midLink());
		REQUIRE CHECK(scene.editor.selectedLink().has_value());
		driver.click(scene.windowPointOf({ 700, 500 }));

		CHECK(!scene.editor.selectedLink().has_value());
	}

	TEST(SelectedConnectionIsHighlighted) {
		LinkedScene scene;
		UiDriver driver(scene.root);
		const RGBA accent = scene.editor.theme().palette.accent;

		const auto accentPolylines = [&] {
			int count = 0;
			for (const PaintCommand& command : driver.paint().commands()) {
				if (command.kind == PaintCommand::Kind::Polyline && command.color == accent) count++;
			}
			return count;
		};
		const int before = accentPolylines();
		driver.click(scene.midLink());
		CHECK(accentPolylines() > before);
	}
}

SUITE(NodeEditorFilter) {
	TEST(HiddenNodesAreNotPaintedHitSelectedOrFramed) {
		EditorScene scene;
		const NodeId shown = scene.graph.addNode(scene.step, { 0, 0 });
		const NodeId hidden = scene.graph.addNode(scene.step, { 2000, 2000 });
		REQUIRE CHECK(scene.graph.connect({ shown, PortDirection::Output, 0 }, { hidden, PortDirection::Input, 0 }));
		UiDriver driver(scene.root, { 800, 600 });

		scene.editor.select({ shown, hidden });
		scene.editor.setNodeFilter([&](NodeId id) { return id != hidden; });
		CHECK(scene.editor.selection() == std::vector<NodeId>{ shown });
		CHECK(!scene.editor.nodeAt(scene.graph.findNode(hidden)->position + Vec2F(10.0f, 10.0f)));
		CHECK(!scene.editor.isShown(scene.graph.links().front()));

		scene.editor.execute(NodeEditor::Command::SelectAll);
		CHECK(scene.editor.selection() == std::vector<NodeId>{ shown });

		scene.editor.execute(NodeEditor::Command::FrameAll);
		driver.settle();
		CHECK_CLOSE(1.0f, scene.editor.zoom(), 0.01f);

		scene.editor.execute(NodeEditor::Command::Delete);
		CHECK(scene.graph.findNode(hidden) != nullptr);
		CHECK(scene.graph.findNode(shown) == nullptr);

		scene.editor.setNodeFilter({});
		CHECK(scene.editor.isShown(hidden));
	}
}

SUITE(NodeEditorEmptyDoubleClick) {
	TEST(DoubleClickOnEmptySpaceIsReportedSingleIsNot) {
		EditorScene scene;
		const NodeId node = scene.graph.addNode(scene.step, { 20, 20 });
		UiDriver driver(scene.root, { 800, 600 });
		scene.editor.jumpToView({ 0, 0 }, 1);
		int reported = 0;
		scene.editor.setOnEmptyDoubleClicked([&] { reported++; });

		const Vec2F empty = scene.windowPointOf({ 600, 450 });
		driver.click(empty);
		CHECK_EQUAL(0, reported);
		driver.click(empty);
		CHECK_EQUAL(1, reported);

		scene.doubleClick(driver, scene.headerOf(node));
		CHECK_EQUAL(1, reported);
	}
}

SUITE(NodeEditorLevelOfDetail) {
	size_t textCommands(const RecordingPainter& painter) {
		size_t count = 0;
		for (const PaintCommand& command : painter.commands())
			if (command.kind == PaintCommand::Kind::Text) count++;
		return count;
	}

	TEST(NodesOutsideTheViewportAreNotPainted) {
		EditorScene scene;
		const NodeId visible = scene.graph.addNode(scene.step, { 20, 20 });
		const NodeId farAway = scene.graph.addNode(scene.step, { 5000, 5000 });
		const NodeId alsoFar = scene.graph.addNode(scene.step, { -4000, 30 });
		UiDriver driver(scene.root, { 800, 600 });
		scene.editor.jumpToView({ 0, 0 }, 1);

		driver.paint();
		CHECK_EQUAL(1u, scene.editor.paintedNodeCount());
		CHECK(scene.editor.isVisibleOnScreen(*scene.graph.findNode(visible)));
		CHECK(!scene.editor.isVisibleOnScreen(*scene.graph.findNode(farAway)));
		CHECK(!scene.editor.isVisibleOnScreen(*scene.graph.findNode(alsoFar)));

		scene.editor.jumpToView({ -4980, -4980 }, 1);
		driver.paint();
		CHECK_EQUAL(1u, scene.editor.paintedNodeCount());
		CHECK(scene.editor.isVisibleOnScreen(*scene.graph.findNode(farAway)));
	}

	TEST(PartlyVisibleNodeIsStillPainted) {
		EditorScene scene;
		const NodeId node = scene.graph.addNode(scene.step, { -100, 40 });
		UiDriver driver(scene.root, { 800, 600 });
		scene.editor.jumpToView({ 0, 0 }, 1);

		driver.paint();
		CHECK(scene.editor.isVisibleOnScreen(*scene.graph.findNode(node)));
		CHECK_EQUAL(1u, scene.editor.paintedNodeCount());
	}

	TEST(ZoomingOutReplacesTextWithLinesThenBlocks) {
		EditorScene scene;
		scene.graph.addNode(scene.step, { 20, 20 });
		UiDriver driver(scene.root, { 800, 600 });

		scene.editor.jumpToView({ 0, 0 }, 1);
		CHECK(scene.editor.detail() == NodeEditor::Detail::Full);
		CHECK(textCommands(driver.paint()) > 0);

		scene.graph.findNode(1)->bodyHeight = 120;
		scene.editor.jumpToView({ 0, 0 }, 0.5f);
		CHECK(scene.editor.detail() == NodeEditor::Detail::Simplified);
		const RecordingPainter& simplified = driver.paint();
		CHECK_EQUAL(0u, textCommands(simplified));
		size_t fills = 0;
		for (const PaintCommand& command : simplified.commands())
			if (command.kind == PaintCommand::Kind::FillRect) fills++;
		CHECK(fills >= 4);

		scene.editor.jumpToView({ 0, 0 }, 0.3f);
		CHECK(scene.editor.detail() == NodeEditor::Detail::Outline);
		const RecordingPainter& outline = driver.paint();
		CHECK_EQUAL(0u, textCommands(outline));

		scene.editor.setPresenter([](const Node&) {
			NodeEditor::NodePresentation presentation;
			presentation.title = "Long title";
			presentation.shortTitle = "#7";
			return presentation;
		});
		const RecordingPainter& labelled = driver.paint();
		CHECK_EQUAL(1u, textCommands(labelled));
		CHECK(labelled.hasText("#7"));
		CHECK(!labelled.hasText("Long title"));

		scene.editor.jumpToView({ 0, 0 }, 0.5f);
		const RecordingPainter& simplifiedLabel = driver.paint();
		CHECK_EQUAL(1u, textCommands(simplifiedLabel));
		CHECK(simplifiedLabel.hasText("#7"));
		size_t circles = 0;
		for (const PaintCommand& command : outline.commands())
			if (command.kind == PaintCommand::Kind::Circle || command.kind == PaintCommand::Kind::StrokeCircle) circles++;
		CHECK_EQUAL(0u, circles);
	}

	TEST(FrameAllBeforeLayoutAppliesOnFirstPaint) {
		EditorScene scene;
		const NodeId node = scene.graph.addNode(scene.step, { 4000, 3000 });
		scene.editor.frameAll();
		CHECK(scene.editor.isFramePending());

		UiDriver driver(scene.root, { 800, 600 });
		driver.paint();
		CHECK(!scene.editor.isFramePending());
		CHECK(scene.editor.isVisibleOnScreen(*scene.graph.findNode(node)));
		CHECK_EQUAL(1u, scene.editor.paintedNodeCount());
	}

	TEST(LinksBetweenOffscreenNodesAreSkipped) {
		EditorScene scene;
		const NodeId first = scene.graph.addNode(scene.step, { 3000, 3000 });
		const NodeId second = scene.graph.addNode(scene.step, { 3400, 3000 });
		REQUIRE CHECK(scene.graph.connect({ first, PortDirection::Output, 0 }, { second, PortDirection::Input, 0 }));
		UiDriver driver(scene.root, { 800, 600 });
		scene.editor.jumpToView({ 0, 0 }, 1);

		size_t polylines = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Polyline) polylines++;
		CHECK_EQUAL(0u, polylines);
	}
}
