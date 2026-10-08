#include "TestSketchesRoot.hpp"
#include "FakeWindowHost.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <filesystem>
#include <unistd.h>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class SilentViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 7 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class TouchIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct TouchScene {
		Document document;
		Brush brush;
		SilentViewport viewport;
		TouchIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };

		TouchScene() {
			brush.apply(*findBrushPreset(BrushType::Round));
			classic.canvas.settings().viewStiffness = 1;
		}
	};

	struct Device {
		TouchScene scene;
		UiDriver driver;

		explicit Device(Vec2F size) :
			driver(scene.studio, size) {
			driver.ui().setInteractionMode(InteractionMode::Touch);
			driver.layout();
		}

		[[nodiscard]] RectF canvas() const { return scene.classic.canvas.windowRect(); }

		void openLibrary() { driver.click(scene.studio.library); }
	};

	bool inside(const RectF& outer, const RectF& inner) {
		return inner.pos.x >= outer.pos.x - 0.01f && inner.pos.y >= outer.pos.y - 0.01f &&
					 inner.pos.x + inner.size.x <= outer.pos.x + outer.size.x + 0.01f &&
					 inner.pos.y + inner.size.y <= outer.pos.y + outer.size.y + 0.01f;
	}
}

SUITE(TouchLibrary) {
	struct LibraryDevice : Device {
		std::filesystem::path root;

		LibraryDevice() :
			Device({ 1280, 800 }),
			root(std::filesystem::temp_directory_path() / ("sketch2d-touch-library-" + std::to_string(::getpid()))) {
			std::filesystem::remove_all(root);
			scene.classic.sketches = SketchLibrary(root.string());
		}

		~LibraryDevice() { std::filesystem::remove_all(root); }

		void paint() {
			driver.drag({ 300, 400 }, { 500, 400 });
			scene.document.settlePaint();
		}

		[[nodiscard]] MenuItem* action(const std::string& text) {
			for (const auto& item : scene.classic.library.actionsMenu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		}

		void hold(Widget& row) {
			driver.moveTo(row.windowRect().center());
			driver.press();
			driver.advance(Ui::kLongPressMs + 20);
			driver.release();
		}

		void openFolderPage(const std::string& name) {
			LibraryView& library = scene.classic.library;
			if (!library.isOpen()) openLibrary();
			if (library.tab() != LibraryView::Tab::Collections) driver.click(library.collectionsTab);
			SketchRow* row = library.currentList().row(name);
			REQUIRE CHECK(row != nullptr);
			driver.click(*row);
		}

		void chooseAction(const std::string& text) {
			driver.click(scene.classic.library.actionsButton);
			MenuItem* item = action(text);
			REQUIRE CHECK(item != nullptr);
			driver.click(*item);
		}
	};

	TEST(LibraryButtonOpensAFullscreenLibrary) {
		LibraryDevice device;

		device.openLibrary();

		CHECK(device.scene.classic.library.isOpen());
		const RectF view = device.scene.classic.library.windowRect();
		CHECK_CLOSE(1280.0f, view.size.x, 0.5f);
		CHECK_CLOSE(800.0f, view.size.y, 0.5f);
	}

	TEST(LibrarySlidesUpWhenOpened) {
		LibraryDevice device;
		device.driver.setAutoSettle(false);

		device.scene.classic.library.open(device.driver.ui());
		device.driver.layout();
		device.driver.advance(16);

		CHECK(device.scene.classic.library.windowRect().pos.y > 100);
		device.driver.settle();
		CHECK_CLOSE(0.0f, device.scene.classic.library.windowRect().pos.y, 0.5f);
	}

	TEST(BackKeyClosesTheLibraryAndItsSaveDialogOneAtATime) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();
		device.chooseAction("Save current");
		REQUIRE CHECK(library.saveDialog.isOpen());

		device.driver.key(InputID::ESCAPE);
		CHECK(!library.saveDialog.isOpen());
		CHECK(library.isOpen());

		device.driver.key(InputID::ESCAPE);
		CHECK(!library.isOpen());
	}

	TEST(ChangesAreAutosavedAfterAPause) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.driver.setAutoSettle(false);
		device.paint();

		CHECK(device.driver.ui().needsFrame());
		device.driver.settle(LibraryView::kAutosaveDelayMs - 500);
		CHECK(device.scene.classic.sketches.list().empty());
		device.driver.settle(1000);

		REQUIRE CHECK_EQUAL(1u, device.scene.classic.sketches.list().size());
		CHECK_EQUAL(library.currentName(), device.scene.classic.sketches.list().front().path);
		CHECK_EQUAL("Scratch/Sketch 1", library.currentName());
		device.driver.settle(LibraryView::kAutosaveDelayMs * 2);
		CHECK_EQUAL(1u, device.scene.classic.sketches.list().size());
		CHECK(!device.driver.ui().needsFrame());
	}

	TEST(AutosaveWritesInTheBackgroundAndKeepsLaterEdits) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveInBackground();
		CHECK(library.isSavingInBackground());
		device.paint();
		library.finishBackgroundSave();
		CHECK(!library.isSavingInBackground());
		CHECK_EQUAL("Scratch/Sketch 1", library.currentName());
		CHECK(library.hasUnsavedChanges());
		REQUIRE CHECK_EQUAL(1u, device.scene.classic.sketches.list().size());
		library.saveIfChanged();
		CHECK(!library.hasUnsavedChanges());
	}

	TEST(AutosaveWaitsForTheEngineToBeIdle) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.document.setEngineThreads(1);
		device.paint();
		device.scene.document.whilePaintPaused([&]() {
			for (int4 tile = 0; tile < 4; tile++)
				device.scene.document.activeLayer()->tileAt({ tile, 0 })->clear({ 1, 0, 0, 1 });
			(void) device.scene.document.applyFilter(Document::PixelFilter::Invert);
			library.advanceAutosave(LibraryView::kAutosaveDelayMs * 2, false);
			CHECK(!library.isSavingInBackground());
		});
		device.scene.document.settlePaint();
	}

	TEST(StartingANewSketchSavesTheCurrentOneFirst) {
		LibraryDevice device;
		device.paint();

		device.scene.classic.commands.run(Command::NewInfiniteCanvas);

		REQUIRE CHECK_EQUAL(1u, device.scene.classic.sketches.list().size());
		CHECK_EQUAL(0u, device.scene.document.tileCount());
	}

	TEST(OpeningAnotherSketchSavesTheCurrentOneFirst) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("First");
		library.startNew();
		device.paint();
		REQUIRE CHECK(library.hasUnsavedChanges());

		library.openSketch("Scratch/First");

		CHECK_EQUAL(2u, device.scene.classic.sketches.list().size());
		CHECK_EQUAL("Scratch/First", library.currentName());
	}

	TEST(AnEmptyNewCanvasIsNotAutosaved) {
		LibraryDevice device;
		device.paint();
		device.scene.classic.commands.run(Command::Undo);
		REQUIRE CHECK(device.scene.classic.library.hasUnsavedChanges());

		device.scene.classic.library.saveIfChanged();

		CHECK(device.scene.classic.sketches.list().empty());
	}

	TEST(AutosaveCanBeTurnedOff) {
		LibraryDevice device;
		device.driver.setAutoSettle(false);
		device.scene.classic.library.setAutosave(false);
		device.scene.document.stampBrushDab({ 30, 30 }, device.scene.brush, 1);
		device.scene.document.finishStroke();

		device.driver.advance(LibraryView::kAutosaveDelayMs * 2);

		CHECK(device.scene.classic.sketches.list().empty());
	}

	TEST(SavingTheCurrentSketchAddsARowAndOpeningItRestoresIt) {
		LibraryDevice device;
		device.paint();
		const ualni tiles = device.scene.document.tileCount();
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();

		device.chooseAction("Save current");
		REQUIRE CHECK(library.saveDialog.isOpen());
		device.driver.click(*library.saveDialog.buttons().front());

		REQUIRE CHECK_EQUAL(1u, library.latestList().rows.size());
		CHECK_EQUAL("Sketch 1", library.latestList().rows.front()->item().name);
		CHECK_EQUAL("Scratch/Sketch 1", library.currentName());

		REQUIRE CHECK(library.isOpen());
		device.chooseAction("New sketch");
		CHECK(!library.isOpen());
		CHECK_EQUAL(0u, device.scene.document.tileCount());

		device.openLibrary();
		device.driver.click(*library.latestList().rows.front());

		CHECK(!library.isOpen());
		CHECK_EQUAL(tiles, device.scene.document.tileCount());
		CHECK_EQUAL("Scratch/Sketch 1", library.currentName());
	}

	TEST(SaveDialogReleasesTheKeyboardAfterSaving) {
		LibraryDevice device;
		FakeWindowHost host;
		host.platform.formFactor = FormFactor::Tablet;
		device.driver.ui().setWindowHost(&host);
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();
		device.driver.setPointerTool(PointerTool::Finger);

		device.chooseAction("Save current");
		REQUIRE CHECK(host.textInputEnabled);
		device.driver.click(*library.saveDialog.buttons().front());

		CHECK(!library.saveDialog.isOpen());
		CHECK(!host.textInputEnabled);
		device.driver.ui().setWindowHost(nullptr);
	}

	TEST(SavingAnOpenedSketchOverwritesItWithoutAsking) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		library.saveCurrentAs("Mine");
		device.paint();

		device.openLibrary();
		device.chooseAction("Save current");

		CHECK(!library.saveDialog.isOpen());
		CHECK_EQUAL(1u, library.latestList().rows.size());
		Document reopened;
		CHECK(device.scene.classic.sketches.open("Scratch/Mine", reopened));
		CHECK(reopened.tileCount() > 0);
	}

	TEST(LibraryHasLatestAndCollectionsTabs) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("One");

		device.openLibrary();
		CHECK(library.tab() == LibraryView::Tab::Latest);
		CHECK_EQUAL(1u, library.latestList().rows.size());

		device.driver.click(library.collectionsTab);
		CHECK(library.tab() == LibraryView::Tab::Collections);
		device.driver.click(library.actionsButton);
		CHECK(device.action("New folder") != nullptr);
		device.driver.key(InputID::ESCAPE);
		LibraryList& collections = library.currentList();
		REQUIRE CHECK(collections.row("Scratch") != nullptr);
		CHECK(collections.row("Scratch")->isFolder());
	}

	TEST(FoldersOpenAsPagesWithTheirOwnNewSketchButton) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.classic.sketches.createFolder("Animals");
		device.openLibrary();
		device.driver.click(library.collectionsTab);

		device.driver.click(*library.currentList().row("Animals"));
		CHECK_EQUAL(2u, library.depth());
		CHECK_EQUAL("Animals", library.currentPage().title());
		CHECK(inside(library.currentPage().windowRect(), library.actionsButton.windowRect()));
		CHECK(library.actionsButton.windowRect().pos.y > library.currentPage().backButton().windowRect().pos.y + 10);

		device.chooseAction("New sketch");
		device.paint();
		library.saveIfChanged();

		CHECK_EQUAL("Animals/Sketch 1", library.currentName());
		CHECK(device.scene.classic.sketches.contains("Animals/Sketch 1"));
	}

	TEST(NewFolderIsCreatedInTheOpenFolder) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.classic.sketches.createFolder("Work");
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("Work"));

		device.chooseAction("New folder");
		REQUIRE CHECK(library.folderDialog.isOpen());
		library.folderDialog.name.setText("Ideas");
		device.driver.click(*library.folderDialog.buttons().front());

		CHECK(device.scene.classic.sketches.isFolder("Work/Ideas"));
		CHECK(library.isOpen());
		CHECK(library.currentList().row("Ideas") != nullptr);
	}

	TEST(DeletingAFolderAsksFirst) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.classic.sketches.createFolder("Old");
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("Old"));

		device.chooseAction("Delete folder");
		REQUIRE CHECK(library.confirmDialog.isOpen());
		CHECK(device.scene.classic.sketches.isFolder("Old"));
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!device.scene.classic.sketches.isFolder("Old"));
		CHECK_EQUAL(1u, library.depth());
	}

	TEST(ScratchFolderCannotBeDeleted) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("Scratch"));

		device.driver.click(library.actionsButton);
		CHECK(device.action("Delete folder") == nullptr);
		CHECK(device.action("New sketch") != nullptr);
	}

	TEST(ClearScratchDeletesOnlyScratchSketchesAfterConfirming) {
		LibraryDevice device;
		SketchLibrary& sketches = device.scene.classic.sketches;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		sketches.createFolder("Keep");
		REQUIRE CHECK(sketches.save("Scratch/One", device.scene.document));
		REQUIRE CHECK(sketches.save("Scratch/Two", device.scene.document));
		REQUIRE CHECK(sketches.save("Keep/Three", device.scene.document));
		library.setCurrentName("Scratch/One");
		device.openLibrary();
		REQUIRE CHECK_EQUAL(3u, library.latestList().rows.size());
		CHECK(library.clearScratch.isVisible());

		device.driver.click(library.clearScratch);
		REQUIRE CHECK(library.confirmDialog.isOpen());
		CHECK(sketches.contains("Scratch/One"));
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!sketches.contains("Scratch/One"));
		CHECK(!sketches.contains("Scratch/Two"));
		CHECK(sketches.contains("Keep/Three"));
		CHECK(sketches.isFolder("Scratch"));
		CHECK_EQUAL(1u, library.latestList().rows.size());
		CHECK(library.currentName().empty());
	}

	TEST(ClearScratchCanBeCancelledAndIsOfferedInsideScratch) {
		LibraryDevice device;
		SketchLibrary& sketches = device.scene.classic.sketches;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		REQUIRE CHECK(sketches.save("Scratch/One", device.scene.document));
		device.openLibrary();

		device.driver.click(library.clearScratch);
		REQUIRE CHECK(library.confirmDialog.isOpen());
		device.driver.key(InputID::ESCAPE);
		CHECK(sketches.contains("Scratch/One"));

		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("Scratch"));
		device.chooseAction("Clear scratch");
		REQUIRE CHECK(library.confirmDialog.isOpen());
		device.driver.click(*library.confirmDialog.buttons().front());
		CHECK(!sketches.contains("Scratch/One"));
	}

	TEST(ClearingAnEmptyScratchOnlyNotifies) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();

		device.driver.click(library.clearScratch);
		CHECK(!library.confirmDialog.isOpen());
		CHECK(library.isOpen());
	}

	TEST(LongPressDraggingASketchOntoAFolderMovesIt) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("Mine");
		device.scene.classic.sketches.createFolder("Scratch/Keep");
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("Scratch"));
		LibraryList& scratch = library.currentList();
		REQUIRE CHECK(scratch.row("Mine") != nullptr && scratch.row("Keep") != nullptr);

		const Vec2F from = scratch.row("Mine")->windowRect().center();
		const Vec2F to = scratch.row("Keep")->windowRect().center();
		device.driver.moveTo(from);
		device.driver.press();
		device.driver.advance(Ui::kLongPressMs + 20);
		device.driver.moveTo(to);
		device.driver.release();

		CHECK(device.scene.classic.sketches.contains("Scratch/Keep/Mine"));
		CHECK_EQUAL("Scratch/Keep/Mine", library.currentName());
	}

	TEST(FolderPagesShowAClickablePath) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.classic.sketches.createFolder("A");
		device.scene.classic.sketches.createFolder("A/B");
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.driver.click(*library.currentList().row("A"));
		device.driver.click(*library.currentList().row("B"));
		REQUIRE CHECK_EQUAL(3u, library.depth());

		std::vector<std::string> labels;
		for (const auto& crumb : library.currentList().crumbs)
			labels.push_back(crumb->text());
		CHECK(labels == std::vector<std::string>({ "Collections", "A", "B" }));

		Button* collections = library.currentList().crumbs.front().get();
		device.driver.click(*collections);

		CHECK_EQUAL(1u, library.depth());
		CHECK(library.tab() == LibraryView::Tab::Collections);
	}

	TEST(HoldingARowSelectsItAndTapsThenToggle) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		SketchLibrary& sketches = device.scene.classic.sketches;
		Document empty;
		sketches.save("Scratch/One", empty);
		sketches.save("Scratch/Two", empty);
		device.openFolderPage("Scratch");
		LibraryList& list = library.currentList();

		device.hold(*list.row("One"));
		CHECK(library.isSelecting());
		CHECK(list.row("One")->isSelected());
		REQUIRE CHECK(list.banner != nullptr);
		CHECK(list.banner->message.text().find("1 item selected") != std::string::npos);
		CHECK(library.isOpen());
		CHECK(library.currentName().empty());

		device.driver.click(*list.row("Two"));
		CHECK_EQUAL(2u, library.selection().size());
		device.driver.click(*list.row("One"));
		CHECK_EQUAL(1u, library.selection().size());
		CHECK(!list.row("One")->isSelected());

		device.driver.key(InputID::ESCAPE);
		CHECK(!library.isSelecting());
		CHECK(library.isOpen());
		CHECK(list.banner == nullptr);
	}

	TEST(SelectionBarDeletesAndMoves) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.scene.classic.sketches.save("Scratch/Two", empty);
		device.openFolderPage("Scratch");
		LibraryList& list = library.currentList();
		device.hold(*list.row("One"));

		Button* deleteButton = nullptr;
		for (const auto& button : list.bannerButtons)
			if (button->text() == "Delete") deleteButton = button.get();
		REQUIRE CHECK(deleteButton != nullptr);
		device.driver.click(*deleteButton);
		REQUIRE CHECK(library.confirmDialog.isOpen());
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!device.scene.classic.sketches.contains("Scratch/One"));
		CHECK(device.scene.classic.sketches.contains("Scratch/Two"));
		CHECK(library.currentList().banner == nullptr);
	}

	TEST(SelectionMovesIntoAPickedFolder) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		SketchLibrary& sketches = device.scene.classic.sketches;
		Document empty;
		sketches.save("Scratch/One", empty);
		sketches.save("Scratch/Two", empty);
		sketches.createFolder("Art");
		sketches.createFolder("Art/Old");
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.driver.click(*library.currentList().row("Two"));

		device.chooseAction("Move to\u2026");
		REQUIRE CHECK(library.isPickingDestination());
		REQUIRE CHECK(library.currentList().banner != nullptr);
		CHECK(library.currentList().banner->message.text().find("2 items") != std::string::npos);
		CHECK(!library.currentList().moveHere->isEnabled());
		CHECK(inside(library.currentPage().windowRect(), library.currentList().moveHere->windowRect()));
		REQUIRE CHECK(library.currentList().row("Art") != nullptr);
		CHECK(library.currentList().row("Scratch") != nullptr);
		device.driver.click(*library.currentList().row("Art"));
		CHECK(library.currentList().row("Old") != nullptr);
		CHECK(library.currentList().banner->message.text().find("\"Art\"") != std::string::npos);
		CHECK(library.currentList().moveHere->isEnabled());
		std::vector<std::string> labels;
		for (const auto& crumb : library.currentList().crumbs)
			labels.push_back(crumb->text());
		CHECK(labels == std::vector<std::string>({ "Move to", "Art" }));
		device.chooseAction("Move here");

		CHECK(sketches.contains("Art/One"));
		CHECK(sketches.contains("Art/Two"));
		CHECK(!sketches.contains("Scratch/One"));
		CHECK(!library.isSelecting());
		CHECK(!library.isPickingDestination());
		CHECK_EQUAL("Scratch", library.currentPage().title());
	}

	TEST(SelectionIsDeletedAfterConfirming) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		SketchLibrary& sketches = device.scene.classic.sketches;
		Document empty;
		sketches.save("Scratch/One", empty);
		sketches.createFolder("Scratch/Box");
		sketches.save("Scratch/Box/Inside", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.driver.click(*library.currentList().row("Box"));

		device.chooseAction("Delete");
		REQUIRE CHECK(library.confirmDialog.isOpen());
		CHECK(sketches.contains("Scratch/One"));
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!sketches.contains("Scratch/One"));
		CHECK(!sketches.isFolder("Scratch/Box"));
		CHECK(!library.isSelecting());
		CHECK(library.isOpen());
	}

	TEST(SketchesCanBeSelectedInLatest) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.openLibrary();

		device.hold(*library.latestList().rows.front());

		CHECK(library.isSelecting());
		device.driver.click(library.actionsButton);
		CHECK(device.action("Move to\u2026") != nullptr);
	}

	TEST(DeletingTheOpenSketchDoesNotRecreateIt) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("Mine");
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("Mine"));
		device.chooseAction("Delete");
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!device.scene.classic.sketches.contains("Scratch/Mine"));
		CHECK(library.currentName().empty());
		library.saveIfChanged();
		CHECK(!device.scene.classic.sketches.contains("Scratch/Mine"));
	}

	TEST(MovingTheOpenSketchKeepsSavingIntoItsNewPlace) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("Mine");
		device.scene.classic.sketches.createFolder("Art");
		library.moveInto("Scratch/Mine", "Art");
		CHECK_EQUAL("Art/Mine", library.currentName());
		library.close();

		device.paint();
		library.saveIfChanged();

		CHECK(device.scene.classic.sketches.contains("Art/Mine"));
		CHECK(!device.scene.classic.sketches.contains("Scratch/Mine"));
	}

	TEST(MovingOntoATakenNameKeepsBoth) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/Twin", empty);
		device.scene.classic.sketches.save("Scratch/Box/Twin", empty);

		library.moveInto("Scratch/Twin", "Scratch/Box");

		CHECK(device.scene.classic.sketches.contains("Scratch/Twin"));
		CHECK(device.scene.classic.sketches.contains("Scratch/Box/Twin"));
	}

	TEST(BackFromAFolderReturnsToCollections) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.openFolderPage("Scratch");
		device.driver.key(InputID::ESCAPE);
		CHECK_EQUAL(1u, library.depth());
		CHECK(library.tab() == LibraryView::Tab::Collections);
		device.driver.key(InputID::ESCAPE);
		CHECK(!library.isOpen());
	}

	TEST(LatestRefreshesAfterAMove) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.scene.classic.sketches.createFolder("Art");
		device.openLibrary();
		device.hold(*library.latestList().rows.front());
		device.chooseAction("Move to\u2026");
		device.driver.click(*library.currentList().row("Art"));
		device.driver.click(*library.currentList().moveHere);

		CHECK_EQUAL(1u, library.depth());
		REQUIRE CHECK_EQUAL(1u, library.latestList().rows.size());
		CHECK_EQUAL("Art/One", library.latestList().rows.front()->item().path);
	}

	TEST(CancellingThePickerReturnsToTheFolder) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.chooseAction("Move to\u2026");
		device.driver.click(library.currentList().banner->cancel);

		CHECK(!library.isPickingDestination());
		CHECK_EQUAL("Scratch", library.currentPage().title());
		CHECK(library.isOpen());
	}

	TEST(DeletingTheFolderOfTheOpenSketchForgetsIt) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		device.scene.classic.sketches.createFolder("Box");
		library.saveCurrentAs("Box/Mine");
		device.openFolderPage("Box");
		device.chooseAction("Delete folder");
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK(!device.scene.classic.sketches.isFolder("Box"));
		CHECK(library.currentName().empty());
		CHECK_EQUAL(1u, library.depth());
	}

	TEST(SaveCurrentUsesTheFolderOfANewSketch) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.scene.classic.sketches.createFolder("Art");
		device.openFolderPage("Art");
		device.chooseAction("New sketch");
		device.paint();

		device.openLibrary();
		device.chooseAction("Save current");
		REQUIRE CHECK(library.saveDialog.isOpen());
		device.driver.click(*library.saveDialog.buttons().front());

		CHECK(device.scene.classic.sketches.contains("Art/Sketch 1"));
	}

	TEST(BlankFolderNameGetsANewFolderName) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		library.createFolder("", "   ");

		CHECK(!device.scene.classic.sketches.isFolder("Sketch"));
		CHECK(device.scene.classic.sketches.isFolder("New folder 1"));
	}

	TEST(MovingAFolderIntoItselfIsRefusedWithANotice) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		std::vector<std::string> notices;
		library.onNotice = [&notices](const std::string& text) { notices.push_back(text); };
		device.scene.classic.sketches.createFolder("A");
		device.scene.classic.sketches.createFolder("A/B");

		library.moveInto("A", "A/B");

		CHECK(device.scene.classic.sketches.isFolder("A/B"));
		CHECK_EQUAL(1u, notices.size());
	}

	TEST(TappingOpensAgainAfterTheSelectionIsEmpty) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.driver.click(*library.currentList().row("One"));
		CHECK(!library.isSelecting());

		device.driver.click(*library.currentList().row("One"));

		CHECK(!library.isOpen());
		CHECK_EQUAL("Scratch/One", library.currentName());
	}

	TEST(RenamingTheOpenSketchKeepsItOpen) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("Old");
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("Old"));

		device.chooseAction("Rename\u2026");
		REQUIRE CHECK(library.renameDialog.isOpen());
		library.renameDialog.name.setText("New");
		device.driver.click(*library.renameDialog.buttons().front());

		CHECK(device.scene.classic.sketches.contains("Scratch/New"));
		CHECK(!device.scene.classic.sketches.contains("Scratch/Old"));
		CHECK_EQUAL("Scratch/New", library.currentName());
		CHECK(library.currentList().row("New") != nullptr);
		CHECK(!library.isSelecting());
	}

	TEST(RenamingAFolderUpdatesTheOpenSketchInside) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		device.scene.classic.sketches.createFolder("Box");
		library.saveCurrentAs("Box/Mine");

		library.renameItem("Box", "Crate");

		CHECK(device.scene.classic.sketches.contains("Crate/Mine"));
		CHECK_EQUAL("Crate/Mine", library.currentName());
	}

	TEST(RenamingToATakenNameIsRefused) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		std::vector<std::string> notices;
		library.onNotice = [&notices](const std::string& text) { notices.push_back(text); };
		Document empty;
		device.scene.classic.sketches.save("Scratch/A", empty);
		device.scene.classic.sketches.save("Scratch/B", empty);

		library.renameItem("Scratch/A", "B");

		CHECK(device.scene.classic.sketches.contains("Scratch/A"));
		CHECK_EQUAL(1u, notices.size());
	}

	TEST(RenameIsOfferedOnlyForASingleSelection) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/A", empty);
		device.scene.classic.sketches.save("Scratch/B", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("A"));
		device.driver.click(*library.currentList().row("B"));

		device.driver.click(library.actionsButton);

		CHECK(device.action("Rename\u2026") == nullptr);
		CHECK(device.action("Move to\u2026") != nullptr);
	}

	TEST(OpeningAFolderLeavesSelection) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.scene.classic.sketches.createFolder("Scratch/Box");
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.driver.click(*library.currentList().row("Box"));
		CHECK_EQUAL(2u, library.selection().size());
		CHECK_EQUAL(2u, library.depth());

		device.driver.click(library.currentList().banner->cancel);
		device.driver.click(*library.currentList().row("Box"));

		CHECK_EQUAL(3u, library.depth());
		CHECK(!library.isSelecting());
	}

	TEST(SelectionSurvivesBackingOutOfThePicker) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.chooseAction("Move to\u2026");
		device.driver.key(InputID::ESCAPE);

		CHECK_EQUAL("Scratch", library.currentPage().title());
		CHECK(library.isSelecting());
		REQUIRE CHECK(library.currentList().row("One") != nullptr);
		CHECK(library.currentList().row("One")->isSelected());
		CHECK(library.currentList().banner != nullptr);
	}

	TEST(ReorderingIsKeptAfterReopening) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/A", empty);
		device.scene.classic.sketches.save("Scratch/B", empty);
		device.scene.classic.sketches.save("Scratch/C", empty);
		device.openFolderPage("Scratch");
		LibraryList& list = library.currentList();
		const std::string first = list.rows[0]->item().name;
		const Vec2F start = list.rows[0]->windowRect().center();
		const halnf step = list.rows[1]->windowRect().pos.y - list.rows[0]->windowRect().pos.y;
		device.driver.moveTo(start);
		device.driver.press();
		device.driver.advance(Ui::kLongPressMs + 20);
		for (int index = 1; index <= 6; index++)
			device.driver.moveTo(start + Vec2F(0.0f, step * 2.0f * (halnf) index / 6.0f));
		device.driver.release();
		library.close();

		device.openFolderPage("Scratch");
		REQUIRE CHECK_EQUAL(3u, library.currentList().rows.size());
		CHECK_EQUAL(first, library.currentList().rows[2]->item().name);
		CHECK(!library.isSelecting());
	}

	TEST(TheOpenSketchIsHighlighted) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.paint();
		library.saveCurrentAs("Mine");
		Document empty;
		device.scene.classic.sketches.save("Scratch/Other", empty);
		device.openFolderPage("Scratch");

		CHECK(library.currentList().row("Mine")->isCurrent());
		CHECK(!library.currentList().row("Other")->isCurrent());
	}

	TEST(CancellingDeleteKeepsEverything) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.openFolderPage("Scratch");
		device.hold(*library.currentList().row("One"));
		device.chooseAction("Delete");
		REQUIRE CHECK(library.confirmDialog.isOpen());

		device.driver.key(InputID::ESCAPE);

		CHECK(!library.confirmDialog.isOpen());
		CHECK(device.scene.classic.sketches.contains("Scratch/One"));
		CHECK(library.isOpen());
		CHECK(library.isSelecting());
	}

	TEST(DeletingFromLatestRefreshesTheList) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		Document empty;
		device.scene.classic.sketches.save("Scratch/One", empty);
		device.scene.classic.sketches.save("Scratch/Two", empty);
		device.openLibrary();
		device.hold(*library.latestList().rows.front());
		device.chooseAction("Delete");
		device.driver.click(*library.confirmDialog.buttons().front());

		CHECK_EQUAL(1u, library.latestList().rows.size());
		CHECK(library.latestList().banner == nullptr);
	}

	TEST(ScratchCannotBeSelected) {
		LibraryDevice device;
		LibraryView& library = device.scene.classic.library;
		device.openLibrary();
		device.driver.click(library.collectionsTab);
		device.hold(*library.currentList().row("Scratch"));

		CHECK(!library.isSelecting());
		CHECK(device.scene.classic.sketches.isFolder("Scratch"));
	}

	TEST(FileMenuOpenAndSaveUseTheLibrary) {
		LibraryDevice device;

		device.scene.classic.commands.run(Command::Open);
		CHECK(device.scene.classic.library.isOpen());
		device.scene.classic.library.close();

		device.scene.classic.commands.run(Command::SaveAs);
		CHECK(device.scene.classic.library.saveDialog.isOpen());
	}
}
