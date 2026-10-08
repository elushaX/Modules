#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <set>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class NullViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class NoIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct CommandScene {
		Document document;
		Brush brush;
		NullViewport viewport;
		NoIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		CommandScene() { root.canvas.settings().viewStiffness = 1; }
	};

	Modifiers withControl(bool shift = false) {
		Modifiers modifiers;
		modifiers.control = true;
		modifiers.shift = shift;
		return modifiers;
	}

	const MenuItem* itemWithText(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return item.get();
		}
		return nullptr;
	}
}

SUITE(Sketch2DDefaultShortcuts) {
	TEST(DefaultTableMatchesReference) {
		sketch2d::ShortcutMap shortcuts;
		bindDefaultShortcuts(shortcuts);

		CHECK_EQUAL(115u, shortcuts.bindings().size());

		std::set<std::string> texts;
		for (const auto& binding : shortcuts.bindings())
			texts.insert(binding.shortcut.text());
		CHECK_EQUAL(shortcuts.bindings().size(), texts.size());
	}

	TEST(UndoAndRedoUseReferenceFourBindingsAndRepeat) {
		sketch2d::ShortcutMap shortcuts;
		bindDefaultShortcuts(shortcuts);

		CHECK_EQUAL(2u, shortcuts.shortcutsFor(Command::Undo).size());
		CHECK_EQUAL(2u, shortcuts.shortcutsFor(Command::Redo).size());
		CHECK_EQUAL("Ctrl+Z", shortcuts.textFor(Command::Undo));
		CHECK_EQUAL("Ctrl+Shift+Z", shortcuts.textFor(Command::Redo));
		CHECK(shortcuts.find(Event::keyRepeat(InputID::Y).withModifiers(withControl())) != nullptr);
	}

	TEST(ToolKeysFollowReference) {
		sketch2d::ShortcutMap shortcuts;
		bindDefaultShortcuts(shortcuts);

		CHECK_EQUAL("B", shortcuts.textFor(Command::ToolPaint));
		CHECK_EQUAL("E", shortcuts.textFor(Command::ToolEraser));
		CHECK_EQUAL("S", shortcuts.textFor(Command::ToolSmudge));
		CHECK_EQUAL("U", shortcuts.textFor(Command::ToolBucket));
		CHECK_EQUAL("I", shortcuts.textFor(Command::ToolPicker));
		CHECK_EQUAL("V", shortcuts.textFor(Command::ToolMove));
		CHECK_EQUAL("W", shortcuts.textFor(Command::ToolMagicWand));
		CHECK_EQUAL("L", shortcuts.textFor(Command::ToolLasso));
		CHECK_EQUAL("K", shortcuts.textFor(Command::ToolRectangleSelect));
		CHECK_EQUAL("G", shortcuts.textFor(Command::ToolGradient));
		CHECK_EQUAL("O", shortcuts.textFor(Command::ToolRotate));
		CHECK_EQUAL("H", shortcuts.textFor(Command::ToolHand));
		CHECK_EQUAL("Space", shortcuts.textFor(Command::HoldHand));
	}

	TEST(BrushAndColorKeysFollowReference) {
		sketch2d::ShortcutMap shortcuts;
		bindDefaultShortcuts(shortcuts);

		CHECK_EQUAL("[", shortcuts.textFor(Command::SmallerBrush));
		CHECK_EQUAL("]", shortcuts.textFor(Command::BiggerBrush));
		CHECK_EQUAL("1", shortcuts.textFor(Command::BrushOpacity10));
		CHECK_EQUAL("0", shortcuts.textFor(Command::BrushOpacity100));
		CHECK_EQUAL("Alt+Z", shortcuts.textFor(Command::Darker));
		CHECK_EQUAL("Alt+X", shortcuts.textFor(Command::Brighter));
		CHECK_EQUAL("Alt+`", shortcuts.textFor(Command::Lightness0));
		CHECK_EQUAL("Ctrl+K", shortcuts.textFor(Command::ClearLayer));
		CHECK_EQUAL("Ctrl+J", shortcuts.textFor(Command::DuplicateLayer));
		CHECK_EQUAL("Ctrl+E", shortcuts.textFor(Command::MergeLayers));
	}

	TEST(EveryBoundCommandHasALabel) {
		sketch2d::ShortcutMap shortcuts;
		bindDefaultShortcuts(shortcuts);

		for (const auto& binding : shortcuts.bindings())
			CHECK(!commandInfo(binding.command).label.empty());
	}
}

SUITE(Sketch2DCommandRegistry) {
	TEST(UndefinedCommandsAreDisabledAndDoNothing) {
		CommandRegistry commands;
		CHECK(!commands.isEnabled(Command::Undo));
		CHECK(!commands.run(Command::Undo));
	}

	TEST(EnabledPredicateGuardsRun) {
		CommandRegistry commands;
		int runs = 0;
		bool enabled = false;
		commands.define(Command::Undo, [&runs](const Event*) { runs++; }, [&enabled]() { return enabled; });

		CHECK(!commands.run(Command::Undo));
		enabled = true;
		CHECK(commands.run(Command::Undo));
		CHECK_EQUAL(1, runs);
	}

	TEST(HandleKeyRunsBoundCommandAndAcceptsEvent) {
		CommandRegistry commands;
		int runs = 0;
		commands.define(Command::Undo, [&runs](const Event*) { runs++; });

		Event event = Event::key(Event::Type::KeyDown, InputID::Z).withModifiers(withControl());
		CHECK(commands.handleKey(event));
		CHECK(event.isAccepted());
		CHECK_EQUAL(1, runs);

		Event unbound = Event::key(Event::Type::KeyDown, InputID::Q);
		CHECK(!commands.handleKey(unbound));
		CHECK(!unbound.isAccepted());
	}

	TEST(RunNotifiesObserver) {
		CommandRegistry commands;
		Command observed = Command::Redo;
		commands.define(Command::Undo, [](const Event*) {});
		commands.setOnCommandRun([&observed](Command command) { observed = command; });

		commands.run(Command::Undo);
		CHECK(observed == Command::Undo);
	}
}

SUITE(Sketch2DCommandsInUi) {
	TEST(KeyboardShortcutReachesRootCommands) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.key(InputID::EQUAL, withControl());
		CHECK_CLOSE(1.25f, scene.document.zoom(), 0.001f);

		driver.key(InputID::N0, withControl(true));
		CHECK_CLOSE(1.0f, scene.document.zoom(), 0.001f);
	}

	TEST(ToolKeysSwitchBrushType) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.key(InputID::E);
		CHECK(scene.brush.type == BrushType::Eraser);
		CHECK(scene.root.toolStrip.button("eraser").isSelected());

		driver.key(InputID::B);
		CHECK(scene.brush.type != BrushType::Eraser);
	}

	TEST(NewLayerShortcutAddsLayer) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.key(InputID::N, withControl(true));
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK_EQUAL(2u, scene.root.toolBar.layersPopup.rows().size());
	}

	TEST(MenusShowReferenceLabelsAndShortcuts) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		const MenuItem* undo = itemWithText(scene.root.editMenu, "Undo");
		CHECK(undo != nullptr);
		CHECK_EQUAL("Ctrl+Z", undo->entry().shortcut);

		const MenuItem* zoomIn = itemWithText(scene.root.viewMenu, "Zoom in");
		CHECK(zoomIn != nullptr);
		CHECK_EQUAL("Ctrl+=", zoomIn->entry().shortcut);
	}

	TEST(UndefinedCommandsShowDisabledInMenus) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		const MenuItem* exportItem = itemWithText(scene.root.fileMenu, "Export");
		CHECK(exportItem != nullptr);
		CHECK(!exportItem->isEnabled());
	}
}

SUITE(Sketch2DMenuState) {
	TEST(CommandMenusFollowEnabledStateOnOpen) {
		CommandScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		MenuItem* deleteItem = nullptr;
		Menu layerMenu;
		deleteItem = &scene.root.addCommand(layerMenu, Command::DeleteLayer);

		layerMenu.refresh();
		CHECK(!deleteItem->isEnabled());

		scene.root.commands.run(Command::NewLayer);
		layerMenu.refresh();
		CHECK(deleteItem->isEnabled());
	}

	TEST(CheckedCommandsShowCheckState) {
		CommandScene scene;
		bool grid = false;
		scene.root.commands.define(
			Command::ShowGrid, [&grid](const Event*) { grid = !grid; }, {}, [&grid]() { return grid; }
		);
		Menu viewMenu;
		MenuItem& item = scene.root.addCommand(viewMenu, Command::ShowGrid);

		CHECK(!item.entry().isChecked());
		scene.root.commands.run(Command::ShowGrid);
		CHECK(item.entry().isChecked());
	}
}
