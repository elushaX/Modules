#include "SketchLibrary.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unistd.h>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	struct TemporaryLibrary {
		std::filesystem::path root;
		SketchLibrary library;

		TemporaryLibrary() :
			root(
				std::filesystem::temp_directory_path() /
				("sketch2d-sketches-" + std::to_string(::getpid()) + "-" + std::to_string(counter()++))
			),
			library(root.string()) {
			std::filesystem::remove_all(root);
		}

		~TemporaryLibrary() { std::filesystem::remove_all(root); }

		static int& counter() {
			static int value = 0;
			return value;
		}
	};

	void paintSquare(Document& document, Vec2F at, halnf radius, const RGBA& color) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.color = color;
		brush.setSize(radius * 2);
		document.stampBrushDab(at, brush);
		document.stampBrushDab(at + Vec2F(40, 0), brush);
		document.finishStroke();
		document.settlePaint();
	}

	bool sameTiles(const Layer::State& left, const Layer::State& right) {
		if (left.tiles.size() != right.tiles.size()) return false;
		for (auto entry : left.tiles) {
			auto found = right.tiles.presents(entry->key);
			if (!found.isValid()) return false;
			const Tile* a = entry->val.tile.get();
			const Tile* b = right.tiles.getSlotVal(found).tile.get();
			if (std::memcmp(a->pixels(), b->pixels(), (size_t) kTileSize * kTileSize * 4) != 0) return false;
		}
		return true;
	}
}

SUITE(SketchFiles) {
	TEST(SavedSketchReopensWithLayersPixelsAndProperties) {
		TemporaryLibrary temporary;
		Document original;
		paintSquare(original, { 10, 10 }, 8, { 1, 0, 0, 1 });
		original.addLayer("Ink");
		paintSquare(original, { 300, 40 }, 6, { 0, 0, 1, 1 });
		original.activeLayer()->setOpacity(0.5f);
		CanvasBackground background;
		background.color = { 120, 0.5f, 0.8f };
		original.setBackground(background);

		REQUIRE CHECK(temporary.library.save("First", original));
		Document reopened;
		REQUIRE CHECK(temporary.library.open("First", reopened));

		const DocumentState expected = original.state();
		const DocumentState actual = reopened.state();
		REQUIRE CHECK_EQUAL(expected.layers.size(), actual.layers.size());
		for (size_t index = 0; index < expected.layers.size(); index++) {
			CHECK_EQUAL(expected.layers[index].properties.name, actual.layers[index].properties.name);
			CHECK_CLOSE(expected.layers[index].properties.opacity, actual.layers[index].properties.opacity, 0.0001f);
			CHECK(sameTiles(expected.layers[index], actual.layers[index]));
		}
		CHECK(expected.background == actual.background);
		CHECK_EQUAL(1u, reopened.history().entries().size());
	}

	TEST(SketchesKeepTheirView) {
		TemporaryLibrary temporary;
		Document document;
		paintSquare(document, { 20, 20 }, 6, { 1, 0, 0, 1 });
		document.setZoom(2.5f);
		document.setRotation(0.75f);
		document.setPanWorld({ -40, 12 });
		REQUIRE CHECK(temporary.library.save("Viewed", document));

		Document reopened;
		REQUIRE CHECK(temporary.library.open("Viewed", reopened));

		CHECK_CLOSE(2.5f, reopened.zoom(), 0.0001f);
		CHECK_CLOSE(0.75f, reopened.rotation(), 0.0001f);
		CHECK_CLOSE(-40.0f, reopened.panWorld().x, 0.001f);
		CHECK_CLOSE(12.0f, reopened.panWorld().y, 0.001f);
	}

	TEST(SketchesWithoutAViewOpenWithTheDefaultView) {
		TemporaryLibrary temporary;
		Document document;
		paintSquare(document, { 20, 20 }, 6, { 1, 0, 0, 1 });
		REQUIRE CHECK(temporary.library.save("Old", document));
		const std::filesystem::path file = std::filesystem::path(temporary.library.directoryOf("Old")) / "sketch.txt";
		std::string text = readFileBytes(file.string());
		text.erase(text.find("view.transform"));
		writeFileBytes(file.string(), text);

		Document reopened;
		reopened.setZoom(3);
		REQUIRE CHECK(temporary.library.open("Old", reopened));
		CHECK_CLOSE(1.0f, reopened.zoom(), 0.0001f);
	}

	TEST(LibraryListsNewestFirstWithPreviews) {
		TemporaryLibrary temporary;
		Document document;
		paintSquare(document, { 0, 0 }, 10, { 0, 1, 0, 1 });

		CHECK(temporary.library.save("Older", document));
		std::filesystem::last_write_time(
			std::filesystem::path(temporary.library.directoryOf("Older")) / "sketch.txt",
			std::filesystem::file_time_type::clock::now() - std::chrono::hours(1)
		);
		CHECK(temporary.library.save("Newer", document));

		const std::vector<SketchEntry> entries = temporary.library.list();
		REQUIRE CHECK_EQUAL(2u, entries.size());
		CHECK_EQUAL("Newer", entries[0].name);
		CHECK_EQUAL("Older", entries[1].name);

		const std::optional<SketchPreview> preview = SketchFile::loadPreview(entries[0].directory);
		REQUIRE CHECK(preview.has_value());
		CHECK_EQUAL((size_t) SketchPreview::kSize * SketchPreview::kSize * 4, preview->rgba.size());
		CHECK_EQUAL(SketchPreview::kSize, preview->size);
	}

	TEST(PreviewShowsPaintOverTheBackground) {
		Document document;
		paintSquare(document, { 0, 0 }, 60, { 1, 0, 0, 1 });

		const SketchPreview preview = SketchPreview::render(document);
		bool red = false;
		for (size_t offset = 0; offset < preview.rgba.size(); offset += 4)
			red = red || (preview.rgba[offset] > 200 && preview.rgba[offset + 1] < 60 && preview.rgba[offset + 3] == 255);
		CHECK(red);
		CHECK_EQUAL(255, (int) preview.rgba[3]);
	}

	TEST(SavingAgainReplacesTheSketch) {
		TemporaryLibrary temporary;
		Document document;
		paintSquare(document, { 0, 0 }, 10, { 1, 1, 1, 1 });
		CHECK(temporary.library.save("Same", document));
		document.addLayer("Second");

		CHECK(temporary.library.save("Same", document));

		Document reopened;
		CHECK(temporary.library.open("Same", reopened));
		CHECK_EQUAL(2u, reopened.layers().size());
		CHECK_EQUAL(1u, temporary.library.list().size());
	}

	TEST(MissingOrDamagedSketchesDoNotOpen) {
		TemporaryLibrary temporary;
		Document document;
		const ualni layers = document.layers().size();

		CHECK(!temporary.library.open("Nothing", document));

		CHECK(temporary.library.save("Broken", document));
		std::ofstream(std::filesystem::path(temporary.library.directoryOf("Broken")) / "sketch.txt") << "garbage\n";
		CHECK(!temporary.library.open("Broken", document));
		CHECK_EQUAL(layers, document.layers().size());
	}

	TEST(NamesAreSanitizedAndUnusedNamesCount) {
		TemporaryLibrary temporary;
		Document document;

		CHECK_EQUAL("a_b_c", SketchLibrary::sanitizedName("a/b\\c"));
		CHECK_EQUAL("Sketch", SketchLibrary::sanitizedName("   "));
		CHECK_EQUAL("Scratch/Sketch 1", temporary.library.unusedName());
		CHECK(temporary.library.save("Scratch/Sketch 1", document));
		CHECK_EQUAL("Scratch/Sketch 2", temporary.library.unusedName());
		CHECK_EQUAL("a/b_c", SketchLibrary::sanitizedPath("/a//b\\c/"));
	}

	TEST(RemovingDeletesTheSketch) {
		TemporaryLibrary temporary;
		Document document;
		CHECK(temporary.library.save("Gone", document));

		CHECK(temporary.library.remove("Gone"));

		CHECK(!temporary.library.contains("Gone"));
		CHECK_EQUAL(0u, temporary.library.list().size());
	}

	TEST(FoldersHoldSketchesAndOtherFolders) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.createFolder("Animals"));
		REQUIRE CHECK(temporary.library.createFolder("Animals/Cats"));
		REQUIRE CHECK(temporary.library.save("Animals/Dog", document));
		REQUIRE CHECK(temporary.library.save("Animals/Cats/Tom", document));

		const std::vector<LibraryItem> animals = temporary.library.children("Animals");

		REQUIRE CHECK_EQUAL(2u, animals.size());
		CHECK(animals[0].kind == LibraryItem::Kind::Folder);
		CHECK_EQUAL("Animals/Cats", animals[0].path);
		CHECK(animals[1].kind == LibraryItem::Kind::Sketch);
		CHECK_EQUAL("Animals/Dog", animals[1].path);
		CHECK(temporary.library.isFolder("Animals/Cats"));
		CHECK(!temporary.library.isFolder("Animals/Dog"));
		CHECK_EQUAL(2u, temporary.library.list().size());
	}

	TEST(OldTopLevelSketchesMoveIntoScratch) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("Old", document));

		temporary.library.ensureScratch();

		CHECK(temporary.library.contains("Scratch/Old"));
		CHECK(!temporary.library.contains("Old"));
		CHECK(temporary.library.isFolder("Scratch"));
	}

	TEST(MigrationKeepsSketchesWhoseNameIsTakenInScratch) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("Scratch/Twin", document));
		REQUIRE CHECK(temporary.library.save("Twin", document));

		temporary.library.ensureScratch();

		CHECK(temporary.library.contains("Scratch/Twin"));
		CHECK(temporary.library.contains("Scratch/Twin 1"));
		CHECK(!temporary.library.contains("Twin"));
	}

	TEST(MovingPutsItemsIntoAnotherFolder) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.createFolder("A"));
		REQUIRE CHECK(temporary.library.createFolder("B"));
		REQUIRE CHECK(temporary.library.save("A/Drawing", document));

		CHECK(temporary.library.move("A/Drawing", "B"));
		CHECK(temporary.library.contains("B/Drawing"));
		CHECK(!temporary.library.contains("A/Drawing"));

		CHECK(temporary.library.move("A", "B"));
		CHECK(temporary.library.isFolder("B/A"));
		CHECK(!temporary.library.move("B", "B/A"));
	}

	TEST(ScratchCannotBeRemovedOrMoved) {
		TemporaryLibrary temporary;
		temporary.library.ensureScratch();
		REQUIRE CHECK(temporary.library.createFolder("Other"));

		CHECK(!temporary.library.remove("Scratch"));
		CHECK(!temporary.library.move("Scratch", "Other"));
		CHECK(temporary.library.isFolder("Scratch"));
	}

	TEST(CustomOrderIsRemembered) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("F/One", document));
		REQUIRE CHECK(temporary.library.save("F/Two", document));
		REQUIRE CHECK(temporary.library.save("F/Three", document));

		REQUIRE CHECK(temporary.library.reorder("F", { "Two", "Three", "One" }));
		const std::vector<LibraryItem> items = temporary.library.children("F");

		REQUIRE CHECK_EQUAL(3u, items.size());
		CHECK_EQUAL("Two", items[0].name);
		CHECK_EQUAL("Three", items[1].name);
		CHECK_EQUAL("One", items[2].name);
	}

	TEST(RemovingAFolderRemovesItsContents) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("Trash/Inside", document));

		CHECK(temporary.library.remove("Trash"));

		CHECK(!temporary.library.isFolder("Trash"));
		CHECK(temporary.library.list().empty());
	}

	TEST(OlderSmallerPreviewsStillLoad) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("Small", document));
		const std::string file = (std::filesystem::path(temporary.library.directoryOf("Small")) / "preview.rgba").string();
		writeFileBytes(file, "sketch2d-preview 1\n" + std::string((size_t) 128 * 128 * 4, '\x7f'));

		const std::optional<SketchPreview> preview = SketchFile::loadPreview(temporary.library.directoryOf("Small"));

		REQUIRE CHECK(preview.has_value());
		CHECK_EQUAL(128, preview->size);
	}

	TEST(SketchesSavedUnderTheOldNameStillOpen) {
		TemporaryLibrary temporary;
		Document document;
		REQUIRE CHECK(temporary.library.save("Scratch/Old", document));
		const std::string directory = temporary.library.directoryOf("Scratch/Old");
		const std::string sketchFile = (std::filesystem::path(directory) / "sketch.txt").string();
		std::string text = readFileBytes(sketchFile);
		REQUIRE CHECK(text.starts_with(SketchFile::kHeader));
		writeFileBytes(sketchFile, "leo-sketch 1" + text.substr(std::string(SketchFile::kHeader).size()));
		writeFileBytes(
			(std::filesystem::path(directory) / "preview.rgba").string(),
			"leo-preview 1\n" + std::string((size_t) 64 * 64 * 4, '\x10')
		);

		Document loaded;
		CHECK(temporary.library.open("Scratch/Old", loaded));
		CHECK(SketchFile::loadPreview(directory).has_value());
	}
}
