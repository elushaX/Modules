#include "BoxLayout.hpp"
#include "FileBrowser.hpp"
#include "Ui.hpp"
#include "MemoryFileSystem.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <filesystem>
#include <fstream>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	MemoryFileSystem sampleHome() {
		MemoryFileSystem files("/home/ilya");
		files.addDirectory("/home/ilya/Pictures");
		files.addDirectory("/home/ilya/.config");
		files.addDirectory("/home/ilya/sessions");
		files.addFile("/home/ilya/notes.txt", 1200);
		files.addFile("/home/ilya/sketch.png", 3 * 1024 * 1024);
		files.addFile("/home/ilya/Animal.PNG", 900);
		files.addFile("/home/ilya/.hidden", 1);
		return files;
	}

	std::vector<std::string> rowNames(const FileBrowser& browser) {
		std::vector<std::string> names;
		for (const auto& row : browser.rows())
			names.push_back(row->entry().name);
		return names;
	}

	struct BrowserScene {
		MemoryFileSystem files = sampleHome();
		Widget root;
		FileBrowser browser{ files };

		BrowserScene() {
			root.emplaceLayout<ColumnLayout>();
			root.addChild(browser);
			browser.setDirectory("/home/ilya");
		}
	};

	struct DialogScene {
		MemoryFileSystem files = sampleHome();
		Widget root;
		FileDialog dialog{ files };
		std::string chosen;
		int chosenCount = 0;

		void show(UiDriver& driver, FileDialog::Mode mode, const std::string& directory, const std::string& name = {}) {
			dialog.show(driver.ui(), mode, "Pick", directory, name, [this](const std::string& path) {
				chosen = path;
				chosenCount++;
			});
			driver.settle();
		}
	};

	void doubleClick(UiDriver& driver, const Widget& widget) {
		driver.click(widget);
		driver.click(widget);
	}
}

SUITE(FilePaths) {
	TEST(JoinParentAndNameHandleSlashes) {
		CHECK_EQUAL("/home/ilya/a.png", joinPath("/home/ilya", "a.png"));
		CHECK_EQUAL("/a", joinPath("/", "a"));
		CHECK_EQUAL("/home", parentPath("/home/ilya/"));
		CHECK_EQUAL("/", parentPath("/home"));
		CHECK_EQUAL("/", parentPath("/"));
		CHECK_EQUAL("ilya", fileName("/home/ilya/"));
	}

	TEST(SizesAreHumanReadable) {
		CHECK_EQUAL("512 B", describeSize(512));
		CHECK_EQUAL("1.5 KB", describeSize(1536));
		CHECK_EQUAL("3.0 MB", describeSize(3 * 1024 * 1024));
		CHECK_EQUAL("42 GB", describeSize(42ull * 1024 * 1024 * 1024));
	}
}

SUITE(FileBrowser) {
	TEST(ListsFoldersFirstThenFilesCaseInsensitivelyAndHidesDotFiles) {
		BrowserScene scene;
		const std::vector<std::string> expected = { "Pictures", "sessions", "Animal.PNG", "notes.txt", "sketch.png" };
		CHECK(expected == rowNames(scene.browser));
		const std::vector<std::string> expectedPath = { "/", "/home", "/home/ilya" };
		CHECK(expectedPath == scene.browser.pathDirectories());
		REQUIRE CHECK_EQUAL(3u, scene.browser.path.partCount());
		CHECK_EQUAL("ilya", scene.browser.path.part(2).text());

		scene.browser.setShowsHidden(true);
		CHECK(scene.browser.row(".config") != nullptr);
		CHECK(scene.browser.row(".hidden") != nullptr);
	}

	TEST(PathStartsAtTheRootAndItsPartsOpenTheirFolders) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		scene.browser.setRootDirectory("/home");
		scene.browser.setDirectory("/home/ilya/sessions");
		driver.settle();

		REQUIRE CHECK_EQUAL(3u, scene.browser.path.partCount());
		CHECK_EQUAL("home", scene.browser.path.part(0).text());
		CHECK_EQUAL("sessions", scene.browser.path.part(2).text());
		CHECK(!scene.browser.path.part(2).isEnabled());
		CHECK(scene.browser.path.part(2).isCurrent());
		CHECK(scene.browser.path.part(2).fontFace() == FontFace::Bold);
		CHECK(scene.browser.path.part(1).fontFace() == FontFace::Regular);
		CHECK(scene.browser.up.text().empty());
		CHECK(scene.browser.up.glyph() == GlyphButton::Glyph::Up);

		driver.click(scene.browser.path.part(1));
		CHECK_EQUAL("/home/ilya", scene.browser.directory());
	}

	TEST(ExtensionFilterKeepsFoldersAndIgnoresCase) {
		BrowserScene scene;
		scene.browser.setExtensions({ ".png" });
		const std::vector<std::string> expected = { "Pictures", "sessions", "Animal.PNG", "sketch.png" };
		CHECK(expected == rowNames(scene.browser));
	}

	TEST(DirectoriesOnlyModeHidesFiles) {
		BrowserScene scene;
		scene.browser.setShowsFiles(false);
		const std::vector<std::string> expected = { "Pictures", "sessions" };
		CHECK(expected == rowNames(scene.browser));
	}

	TEST(ClickSelectsAndDoubleClickEntersAFolder) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		std::string changed;
		scene.browser.onDirectoryChanged = [&](const std::string& directory) { changed = directory; };

		driver.click(*scene.browser.row("notes.txt"));
		REQUIRE CHECK(scene.browser.selected() != nullptr);
		CHECK_EQUAL("notes.txt", scene.browser.selected()->name);
		CHECK(scene.browser.row("notes.txt")->isSelected());

		driver.advance(1000);
		doubleClick(driver, *scene.browser.row("Pictures"));
		CHECK_EQUAL("/home/ilya/Pictures", scene.browser.directory());
		CHECK_EQUAL("/home/ilya/Pictures", changed);
		CHECK(scene.browser.selected() == nullptr);
	}

	TEST(DoubleClickOnAFileReportsItsPath) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		std::string opened;
		scene.browser.onFileOpened = [&](const std::string& path) { opened = path; };
		doubleClick(driver, *scene.browser.row("sketch.png"));
		CHECK_EQUAL("/home/ilya/sketch.png", opened);
	}

	TEST(UpReturnsToTheParentAndSelectsTheFolderWeCameFrom) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		scene.browser.setDirectory("/home/ilya/sessions");

		driver.click(scene.browser.up);
		CHECK_EQUAL("/home/ilya", scene.browser.directory());
		REQUIRE CHECK(scene.browser.selected() != nullptr);
		CHECK_EQUAL("sessions", scene.browser.selected()->name);

		scene.browser.setDirectory("/");
		CHECK(!scene.browser.up.isEnabled());
		CHECK(!scene.browser.goUp());
	}

	TEST(UnreadableFolderKeepsTheCurrentListingAndReportsIt) {
		BrowserScene scene;
		CHECK(!scene.browser.setDirectory("/nowhere"));
		CHECK_EQUAL("/home/ilya", scene.browser.directory());
		CHECK_EQUAL("Can't open /nowhere", scene.browser.error());
	}

	TEST(EmptyFolderSaysSo) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		scene.browser.setDirectory("/home/ilya/Pictures");
		driver.settle();
		CHECK(scene.browser.rows().empty());
		CHECK(driver.paint().hasText("This folder is empty"));
	}

	TEST(RowsPaintNameAndSize) {
		BrowserScene scene;
		UiDriver driver(scene.root, { 600, 400 });
		driver.settle();
		CHECK(driver.paint().hasText("sketch.png"));
		CHECK(driver.paint().hasText("3.0 MB"));
	}

	TEST(LocalFileSystemListsARealFolder) {
		const std::filesystem::path folder = std::filesystem::temp_directory_path() / "tp-file-browser-test";
		std::filesystem::remove_all(folder);
		std::filesystem::create_directories(folder / "inner");
		std::ofstream(folder / "data.bin") << "12345";

		const std::optional<std::vector<FileEntry>> entries = FileSystem::local().list(folder.string());
		REQUIRE CHECK(entries.has_value());
		CHECK_EQUAL(2u, entries->size());
		for (const FileEntry& entry : *entries) {
			if (entry.name == "data.bin") CHECK_EQUAL(5u, entry.size);
			else CHECK(entry.isDirectory);
		}
		CHECK(!FileSystem::local().list((folder / "missing").string()).has_value());
		std::filesystem::remove_all(folder);
	}
}

SUITE(FileDialog) {
	TEST(SaveTypesANameAndReturnsThePathInTheCurrentFolder) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya/sessions", "take-1");
		CHECK(scene.dialog.isOpen());
		CHECK_EQUAL("take-1", scene.dialog.name.text());
		CHECK(driver.ui().keyboardFocus() == &scene.dialog.name);

		driver.type("night-");
		driver.key(InputID::END);
		driver.type("x");
		driver.key(InputID::ENTER);
		CHECK_EQUAL("/home/ilya/sessions/night-x", scene.chosen);
		CHECK(!scene.dialog.isOpen());
	}

	TEST(SaveIntoAFolderPickedByDoubleClick) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "take-2");
		doubleClick(driver, *scene.dialog.browser.row("sessions"));
		CHECK_EQUAL("/home/ilya/sessions", scene.dialog.browser.directory());
		CHECK_EQUAL("take-2", scene.dialog.name.text());

		driver.click(*scene.dialog.buttons().front());
		CHECK_EQUAL("/home/ilya/sessions/take-2", scene.chosen);
	}

	TEST(SavingOverAnExistingFileAsksFirst) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "notes.txt");
		scene.dialog.confirm();
		driver.settle();
		CHECK_EQUAL(0, scene.chosenCount);
		REQUIRE CHECK(scene.dialog.overwrite.isOpen());
		CHECK_EQUAL("Replace notes.txt?", scene.dialog.overwrite.title());

		driver.click(*scene.dialog.overwrite.buttons().front());
		CHECK_EQUAL("/home/ilya/notes.txt", scene.chosen);
	}

	TEST(TypingAFolderNameEntersItInsteadOfSaving) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "Pictures");
		scene.dialog.confirm();
		CHECK_EQUAL(0, scene.chosenCount);
		CHECK_EQUAL("/home/ilya/Pictures", scene.dialog.browser.directory());
		CHECK_EQUAL("", scene.dialog.name.text());
	}

	TEST(EmptyNameIsRejectedWithAMessage) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "");
		scene.dialog.confirm();
		CHECK_EQUAL(0, scene.chosenCount);
		CHECK(scene.dialog.problem.isVisible());
		CHECK_EQUAL("Enter a name", scene.dialog.problem.text());
	}

	TEST(OpenRequiresAnExistingFileAndFollowsSelection) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Open, "/home/ilya");
		driver.click(*scene.dialog.browser.row("sketch.png"));
		CHECK_EQUAL("sketch.png", scene.dialog.name.text());

		scene.dialog.name.setText("missing.png");
		scene.dialog.confirm();
		CHECK_EQUAL(0, scene.chosenCount);
		CHECK_EQUAL("missing.png does not exist", scene.dialog.problem.text());

		driver.advance(1000);
		doubleClick(driver, *scene.dialog.browser.row("notes.txt"));
		CHECK_EQUAL("/home/ilya/notes.txt", scene.chosen);
	}

	TEST(ChooseDirectoryReturnsTheSelectedOrCurrentFolder) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::ChooseDirectory, "/home/ilya");
		CHECK(!scene.dialog.name.isVisible());
		CHECK(scene.dialog.browser.row("notes.txt") == nullptr);
		CHECK_EQUAL("Choose", scene.dialog.buttons().front()->text());

		driver.click(*scene.dialog.browser.row("sessions"));
		scene.dialog.confirm();
		CHECK_EQUAL("/home/ilya/sessions", scene.chosen);
	}

	TEST(NewFolderCreatesAUniqueFolderAndSelectsIt) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "take");
		driver.click(scene.dialog.newFolder);
		CHECK(scene.files.isDirectory("/home/ilya/New folder"));
		driver.click(scene.dialog.newFolder);
		CHECK(scene.files.isDirectory("/home/ilya/New folder 2"));
		REQUIRE CHECK(scene.dialog.browser.selected() != nullptr);
		CHECK_EQUAL("New folder 2", scene.dialog.browser.selected()->name);
	}

	TEST(MissingStartFolderFallsBackToTheNearestExistingParent) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya/sessions/not/yet", "take");
		CHECK_EQUAL("/home/ilya/sessions", scene.dialog.browser.directory());
		scene.dialog.close();
		scene.show(driver, FileDialog::Mode::Save, "", "take");
		CHECK_EQUAL("/home/ilya", scene.dialog.browser.directory());
	}

	TEST(CancelClosesWithoutChoosing) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "take");
		driver.click(*scene.dialog.buttons().back());
		CHECK(!scene.dialog.isOpen());
		CHECK_EQUAL(0, scene.chosenCount);
	}

	TEST(BackspaceGoesUpUnlessTyping) {
		DialogScene scene;
		UiDriver driver(scene.root, { 800, 700 });
		scene.show(driver, FileDialog::Mode::ChooseDirectory, "/home/ilya/sessions");
		driver.click(scene.dialog.browser.up);
		CHECK_EQUAL("/home/ilya", scene.dialog.browser.directory());
		driver.click(*scene.dialog.browser.row("sessions"));
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("/home", scene.dialog.browser.directory());

		scene.dialog.close();
		scene.show(driver, FileDialog::Mode::Save, "/home/ilya", "ab");
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("/home/ilya", scene.dialog.browser.directory());
		CHECK_EQUAL("", scene.dialog.name.text());
	}
}
