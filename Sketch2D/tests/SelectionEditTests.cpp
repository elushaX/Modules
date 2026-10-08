#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class EditViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class EditIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct EditScene {
		Document document;
		Brush brush;
		EditViewport viewport;
		EditIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		EditScene() { root.canvas.settings().viewStiffness = 1; }

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		RGBA at(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false); }
	};
}

SUITE(SelectionEdits) {
	TEST(SpreadGrowsAndShrinks) {
		Selection selection;
		selection.addRectangle(10, 10, 20, 20, SelectionMode::Replace);
		selection.spread(3);
		CHECK_EQUAL(255, selection.coverage(7, 15));
		CHECK_EQUAL(0, selection.coverage(6, 15));
		CHECK_EQUAL(255, selection.coverage(22, 15));
		CHECK_EQUAL(0, selection.coverage(23, 15));
		selection.spread(-5);
		CHECK_EQUAL(0, selection.coverage(11, 15));
		CHECK_EQUAL(255, selection.coverage(12, 15));
		CHECK_EQUAL(255, selection.coverage(17, 15));
		CHECK_EQUAL(0, selection.coverage(18, 15));
		selection.spread(-10);
		CHECK(!selection.isActive());
	}

	TEST(SpreadOfAnInvertedSelectionShrinksTheHole) {
		Selection selection;
		selection.addRectangle(10, 10, 20, 20, SelectionMode::Replace);
		selection.invert();
		selection.spread(2);
		CHECK_EQUAL(255, selection.coverage(11, 15));
		CHECK_EQUAL(0, selection.coverage(12, 15));
	}

	TEST(BlurSoftensTheEdge) {
		Selection selection;
		selection.addRectangle(10, 10, 30, 30, SelectionMode::Replace);
		selection.blur(3);
		CHECK_EQUAL(255, selection.coverage(20, 20));
		const uint1 edge = selection.coverage(10, 20);
		CHECK(edge > 100);
		CHECK(edge < 160);
		CHECK(selection.coverage(8, 20) > 0);
		CHECK(selection.coverage(8, 20) < edge);
		CHECK_EQUAL(0, selection.coverage(6, 20));
		const uint1 top = selection.coverage(20, 10);
		CHECK(top > 100);
		CHECK(top < 160);
		CHECK(selection.coverage(20, 8) > 0);
		CHECK(selection.coverage(10, 10) < edge);
	}

	TEST(FillColorPaintsTheSelection) {
		EditScene scene;
		scene.root.colors.setPrimary(hueColorOf(RGBA(0, 0, 1, 1)));
		scene.root.commands.run(Command::FillSelection);
		CHECK_EQUAL(std::string("Please select an area before you fill"), scene.root.lastNotice());
		scene.document.editSelection().addRectangle(0, 0, 40, 30, SelectionMode::Replace);
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::FillSelection);
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_CLOSE(1.0f, scene.at(20, 15).b, 0.01f);
		CHECK_CLOSE(1.0f, scene.at(20, 15).a, 0.01f);
		CHECK_CLOSE(0.0f, scene.at(45, 15).a, 1e-6f);
	}

	TEST(SoftSelectionFillsPartially) {
		EditScene scene;
		scene.document.editSelection().addRectangle(0, 0, 40, 30, SelectionMode::Replace);
		scene.document.editSelection().blur(4);
		scene.root.commands.run(Command::FillSelection);
		const halnf edge = scene.at(0, 15).a;
		CHECK(edge > 0.3f);
		CHECK(edge < 0.7f);
	}

	TEST(InvertedSelectionCannotBeFilled) {
		EditScene scene;
		scene.document.editSelection().addRectangle(0, 0, 40, 30, SelectionMode::Replace);
		scene.document.editSelection().invert();
		scene.root.commands.run(Command::FillSelection);
		CHECK_EQUAL(std::string("You can't fill the invert of a selection"), scene.root.lastNotice());
	}

	TEST(FillUnderneathAddsALayerBelow) {
		EditScene scene;
		Layer* lines = scene.document.activeLayer();
		scene.document.editSelection().addRectangle(0, 0, 40, 30, SelectionMode::Replace);
		const size_t layers = scene.document.layers().size();
		scene.root.commands.run(Command::FillUnderneath);
		REQUIRE CHECK_EQUAL(layers + 1, scene.document.layers().size());
		CHECK(scene.document.activeLayer() == lines);
		size_t linesIndex = 0;
		for (size_t index = 0; index < scene.document.layers().size(); index++)
			if (scene.document.layers()[index].get() == lines) linesIndex = index;
		REQUIRE CHECK(linesIndex > 0);
		const Layer& fill = *scene.document.layers()[linesIndex - 1];
		CHECK(fill.tileIfExists({ 0, 0 }) != nullptr);
		CHECK_CLOSE(0.0f, scene.at(20, 15).a, 1e-6f);
		CHECK_CLOSE(1.0f, scene.document.sampleColor({ 20.5f, 15.5f }, true).a, 0.01f);
	}

	TEST(ClearRemovesOnlySelectedPixels) {
		EditScene scene;
		scene.document.editSelection().addRectangle(0, 0, 100, 100, SelectionMode::Replace);
		scene.root.commands.run(Command::FillSelection);
		scene.document.editSelection().addRectangle(0, 0, 50, 100, SelectionMode::Replace);
		scene.driver.key(InputID::DELETE_KEY);
		CHECK_CLOSE(0.0f, scene.at(20, 20).a, 1e-6f);
		CHECK_CLOSE(1.0f, scene.at(70, 20).a, 0.01f);
		scene.root.commands.run(Command::Undo);
		CHECK_CLOSE(1.0f, scene.at(20, 20).a, 0.01f);
	}

	TEST(ClearWithoutSelectionClearsTheLayerAndRespectsAlphaLock) {
		EditScene scene;
		scene.document.editSelection().addRectangle(0, 0, 100, 100, SelectionMode::Replace);
		scene.root.commands.run(Command::FillSelection);
		scene.document.deselect();
		scene.document.toggleAlphaLock();
		scene.root.commands.run(Command::Clear);
		CHECK_EQUAL(std::string("You can't clear a alpha locked layer"), scene.root.lastNotice());
		CHECK_CLOSE(1.0f, scene.at(20, 20).a, 0.01f);
		scene.document.toggleAlphaLock();
		scene.root.commands.run(Command::Clear);
		CHECK_CLOSE(0.0f, scene.at(20, 20).a, 1e-6f);
	}

	TEST(SpreadAndBlurCommandsUseTheChosenAmount) {
		EditScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 20, SelectionMode::Replace);
		MenuItem* eight = nullptr;
		for (const auto& item : scene.root.selectAdjustMenu.items())
			if (item->entry().text == "+8 px") eight = item.get();
		REQUIRE CHECK(eight != nullptr);
		eight->entry().action();
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::SpreadSelection);
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_EQUAL(255, scene.document.selection().coverage(2, 15));
		scene.root.commands.run(Command::BlurSelection);
		CHECK(scene.document.selection().coverage(1, 15) < 255);
	}

	TEST(AutoFillFillsNewSelections) {
		EditScene scene;
		scene.root.colors.setPrimary(hueColorOf(RGBA(1, 0, 0, 1)));
		scene.brush.color = { 1, 0, 0, 1 };
		scene.root.canvas.settings().selection.autoFill = true;
		scene.root.commands.run(Command::ToolRectangleSelect);
		scene.driver.drag(scene.screenOf({ 10, 10 }), scene.screenOf({ 60, 40 }));
		CHECK_CLOSE(1.0f, scene.at(30, 20).r, 0.01f);
		CHECK_CLOSE(1.0f, scene.at(30, 20).a, 0.01f);
		scene.root.canvas.settings().selection.autoFill = false;
		scene.driver.drag(scene.screenOf({ 100, 100 }), scene.screenOf({ 150, 140 }));
		CHECK_CLOSE(0.0f, scene.at(120, 120).a, 1e-6f);
	}

	TEST(SelectMenuListsTheEdits) {
		EditScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.selectMenu.items())
			texts.push_back(item->entry().text);
		for (const char* text : { "Spread selection",
															"Blur selection",
															"Fill color",
															"Fill to new layer underneath",
															"Clear",
															"Auto fill selection" }) {
			bool found = false;
			for (const std::string& existing : texts)
				found = found || existing == text;
			CHECK(found);
		}
	}
}
