#include "TestSketchesRoot.hpp"
#include "FakeWindowHost.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <stdexcept>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class StudioViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 7 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class StudioIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override {
			ualni id = 1;
			for (const char letter : name)
				id = id * 131 + (ualni) (unsigned char) letter;
			return { id % 1000000 + 1 };
		}
	};

	struct StudioScene {
		Document document;
		Brush brush;
		StudioViewport viewport;
		StudioIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };

		StudioScene() {
			brush.apply(*findBrushPreset(BrushType::Round));
			classic.canvas.settings().viewStiffness = 1;
		}
	};

	struct Tablet {
		StudioScene scene;
		UiDriver driver;

		explicit Tablet(Vec2F size, InteractionMode mode = InteractionMode::Touch) :
			driver(scene.studio, size) {
			driver.ui().setInteractionMode(mode);
			driver.layout();
		}

		StudioRoot& studio() { return scene.studio; }

		void tool(const std::string& name) {
			if (StudioToolGroup* group = studio().groupOf(name)) {
				driver.click(*group->button);
				driver.settle();
				MenuItem* found = nullptr;
				for (const auto& item : group->menu.items())
					if (item->entry().icon.value == scene.icons.icon(name).value) found = item.get();
				REQUIRE CHECK(found != nullptr);
				driver.click(*found);
			} else {
				driver.click(studio().toolButton(name));
			}
			driver.layout();
		}

		void paintStroke(Vec2F from, Vec2F to) {
			driver.drag(from, to, 12);
			driver.advance(500);
			scene.document.settlePaint();
		}
	};

	bool inside(const RectF& outer, const RectF& inner) {
		return inner.pos.x >= outer.pos.x - 0.01f && inner.pos.y >= outer.pos.y - 0.01f &&
					 inner.pos.x + inner.size.x <= outer.pos.x + outer.size.x + 0.01f &&
					 inner.pos.y + inner.size.y <= outer.pos.y + outer.size.y + 0.01f;
	}

	bool intersects(const RectF& a, const RectF& b) {
		return a.pos.x < b.pos.x + b.size.x && b.pos.x < a.pos.x + a.size.x && a.pos.y < b.pos.y + b.size.y &&
					 b.pos.y < a.pos.y + a.size.y;
	}

	void checkLayout(Tablet& tablet, Vec2F screen) {
		StudioRoot& studio = tablet.studio();
		const RectF window = makeRect({ 0, 0 }, screen);
		const RectF left = studio.topLeft.windowRect();
		const RectF right = studio.topRight.windowRect();
		const RectF bar = studio.toolBar.windowRect();
		const RectF active = studio.activeBar.windowRect();

		CHECK_CLOSE(screen.x, tablet.scene.classic.canvas.windowRect().size.x, 0.01f);
		CHECK_CLOSE(screen.y, tablet.scene.classic.canvas.windowRect().size.y, 0.01f);
		for (const RectF& rect : { left, right, bar })
			CHECK(inside(window, rect));

		CHECK(left.center().x < screen.x * 0.5f);
		CHECK(left.center().y < screen.y * 0.25f);
		CHECK(right.center().x > screen.x * 0.5f);
		CHECK(right.center().y < screen.y * 0.25f);
		if (screen.x > screen.y) {
			CHECK(studio.isToolBarVertical());
			CHECK(bar.center().x > screen.x * 0.75f);
			CHECK(bar.size.y > bar.size.x * 3);
			CHECK(!intersects(bar, right));
		} else {
			CHECK(!studio.isToolBarVertical());
			CHECK_CLOSE(screen.x * 0.5f, bar.center().x, 0.5f);
			CHECK(bar.center().y > screen.y * 0.75f);
		}
		CHECK(!intersects(left, right));
		CHECK(left.pos.x + left.size.x < right.pos.x);
		for (Button* button : studio.toolButtons())
			CHECK(inside(bar, button->windowRect()));
		if (screen.x <= screen.y) {
			CHECK(!studio.activeBar.isVisible());
			return;
		}
		CHECK(inside(window, active));
		CHECK(active.center().x < screen.x * 0.25f);
		CHECK(active.size.y > active.size.x * 2);
		CHECK(!intersects(active, left));
		CHECK(inside(active, studio.size.windowRect()));
		CHECK(inside(active, studio.opacity.windowRect()));
	}
}

SUITE(StudioLayout) {
	TEST(OldTabletControlsAreHidden) {
		Tablet tablet({ 1280, 800 });

		CHECK(!tablet.scene.classic.menuBar.isVisible());
		CHECK(!tablet.scene.classic.toolBar.isVisible());
		CHECK(!tablet.scene.classic.toolStrip.isVisible());
		CHECK(!tablet.scene.classic.transformBar.isVisible());
	}

	TEST(PortraitPlacesTheFourBarsLikeTheDesign) {
		Tablet tablet({ 800, 1280 });
		checkLayout(tablet, { 800, 1280 });
	}

	TEST(LandscapePlacesTheFourBarsLikeTheDesign) {
		Tablet tablet({ 1280, 800 });
		checkLayout(tablet, { 1280, 800 });
	}

	TEST(RotatingKeepsTheLayout) {
		Tablet tablet({ 1280, 800 });
		tablet.driver.resize({ 800, 1280 });
		tablet.driver.layout();
		checkLayout(tablet, { 800, 1280 });
		tablet.driver.resize({ 1280, 800 });
		tablet.driver.layout();
		checkLayout(tablet, { 1280, 800 });
	}

	TEST(ResizingSwitchesTheToolBarSideByAspect) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		CHECK(studio.isToolBarVertical());
		CHECK(studio.toolBar.windowRect().center().x > 1000);

		tablet.driver.resize({ 700, 900 });
		tablet.driver.layout();
		CHECK(!studio.isToolBarVertical());
		CHECK(studio.toolBar.windowRect().center().y > 700);

		tablet.driver.resize({ 1000, 900 });
		tablet.driver.layout();
		CHECK(studio.isToolBarVertical());
		for (Button* button : studio.toolButtons())
			CHECK(inside(studio.toolBar.windowRect(), button->windowRect()));
	}

	TEST(LandscapeGroupPopupsOpenToTheLeft) {
		Tablet tablet({ 1280, 800 });
		StudioToolGroup& group = *tablet.studio().group("select");
		tablet.driver.click(*group.button);
		tablet.driver.settle();
		REQUIRE CHECK(group.button->isPopupOpen());
		CHECK(group.menu.windowRect().pos.x + group.menu.windowRect().size.x <= group.button->windowRect().pos.x + 0.5f);
	}

	TEST(TopLeftHoldsHideLibraryUndoRedoAndSettings) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		const std::vector<Widget*> expected = {
			&studio.hide, &studio.library, &studio.undo, &studio.redo, &studio.settingsButton
		};
		CHECK(studio.topLeft.children() == expected);
	}

	TEST(TopRightHoldsPresetsColorAndLayers) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		for (Widget* widget : std::initializer_list<Widget*>{ &studio.presets, &studio.color, &studio.layers })
			CHECK(widget->parent() == &studio.topRight);
	}

	TEST(SmallPhoneShrinksTheToolBarToFit) {
		Tablet phone({ 420, 820 });
		const RectF bar = phone.studio().toolBar.windowRect();

		CHECK(inside(makeRect({ 0, 0 }, { 420, 820 }), bar));
		for (Button* button : phone.studio().toolButtons())
			CHECK(inside(bar, button->windowRect()));
	}

	TEST(NarrowPhonesShrinkTheTopBarsSoTheyNeverOverlap) {
		for (const Vec2F screen : { Vec2F(360, 760), Vec2F(300, 640), Vec2F(412, 915) }) {
			Tablet phone(screen);
			StudioRoot& studio = phone.studio();
			const RectF left = studio.topLeft.windowRect();
			const RectF right = studio.topRight.windowRect();
			CHECK(left.pos.x + left.size.x < right.pos.x);
			CHECK(inside(makeRect({ 0, 0 }, screen), right));
			for (Widget* button : studio.topLeft.children())
				CHECK(inside(left, button->windowRect()));
			for (Widget* button : studio.topRight.children())
				CHECK(inside(right, button->windowRect()));
		}
	}

	TEST(ActiveBarStaysInsideTheScreenInLandscape) {
		for (const Vec2F screen : { Vec2F(1057, 600), Vec2F(820, 420), Vec2F(700, 330), Vec2F(1280, 800) }) {
			Tablet tablet(screen);
			StudioRoot& studio = tablet.studio();
			tablet.tool("brush");
			const RectF active = studio.activeBar.windowRect();
			const halnf margin = StudioRoot::kEdgeMargin * studio.scale();
			CHECK(active.pos.y + active.size.y <= screen.y - margin + 0.5f);
			CHECK(!intersects(active, studio.topLeft.windowRect()));
			CHECK(inside(active, studio.opacityMark.windowRect()));
			CHECK(inside(active, studio.size.windowRect()));
		}
	}

	TEST(ActiveBarIsHiddenInPortraitAndShownInLandscape) {
		Tablet tablet({ 800, 1280 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("brush");
		CHECK(!studio.activeBar.isVisible());
		tablet.paintStroke({ 300, 500 }, { 500, 520 });
		tablet.tool("transform");
		CHECK(!studio.activeBar.isVisible());

		tablet.driver.resize({ 1280, 800 });
		tablet.driver.layout();
		tablet.driver.layout();
		CHECK(studio.activeBar.isVisible());
		CHECK(studio.applyTransform.isVisible());
	}

	TEST(ControlsAvoidSystemBars) {
		Tablet tablet({ 800, 1280 });
		FakeWindowHost host;
		host.platform.formFactor = FormFactor::Tablet;
		host.platform.safeInsets = { 0, 24, 0, 48 };
		tablet.driver.ui().setWindowHost(&host);
		tablet.studio().requestLayout();
		tablet.driver.layout();

		const halnf margin = StudioRoot::kEdgeMargin * tablet.studio().scale();
		CHECK_CLOSE(24 + margin, tablet.studio().topLeft.windowRect().pos.y, 0.5f);
		CHECK_CLOSE(24 + margin, tablet.studio().topRight.windowRect().pos.y, 0.5f);
		const RectF bar = tablet.studio().toolBar.windowRect();
		CHECK(bar.pos.y + bar.size.y <= 1280 - 48);
		CHECK_CLOSE(1280.0f, tablet.scene.classic.canvas.windowRect().size.y, 0.01f);
		tablet.driver.ui().setWindowHost(nullptr);
	}

	TEST(HidingControlsLeavesOnlyTheRevealButton) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();

		const RectF hideRect = studio.hide.windowRect();
		tablet.driver.click(studio.hide);
		tablet.driver.layout();
		CHECK(!studio.areControlsShown());
		for (Widget* widget :
				 std::initializer_list<Widget*>{ &studio.topLeft, &studio.topRight, &studio.toolBar, &studio.activeBar })
			CHECK(!widget->isVisible());
		REQUIRE CHECK(studio.reveal.isVisible());
		const RectF revealRect = studio.reveal.windowRect();
		CHECK_CLOSE(hideRect.pos.x, revealRect.pos.x, 0.5f);
		CHECK_CLOSE(hideRect.pos.y, revealRect.pos.y, 0.5f);
		CHECK_CLOSE(hideRect.size.x, revealRect.size.x, 0.5f);

		tablet.driver.click(hideRect.center());
		tablet.driver.layout();
		CHECK(studio.areControlsShown());
		tablet.driver.click(studio.hide);
		tablet.driver.layout();
		tablet.driver.click(studio.reveal);
		tablet.driver.layout();
		CHECK(studio.areControlsShown());
		CHECK(studio.activeBar.isVisible());
		CHECK(!studio.reveal.isVisible());
	}
}

SUITE(StudioToolGroups) {
	TEST(BottomBarKeepsMainToolsAndGroupsTheRest) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();

		CHECK_EQUAL(6u, studio.toolButtons().size());
		CHECK(studio.group("paint") != nullptr);
		CHECK(studio.group("select") != nullptr);
		CHECK(studio.group("transform") != nullptr);
		const std::vector<std::string> paint = { "smudge", "fill", "gradient" };
		const std::vector<std::string> select = { "lasso", "rectselect", "wand" };
		const std::vector<std::string> transform = { "transform", "move", "crop" };
		CHECK(studio.group("paint")->tools == paint);
		CHECK(studio.group("select")->tools == select);
		CHECK(studio.group("transform")->tools == transform);
	}

	TEST(GroupPopupOpensAboveTheBarAndPicksATool) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		StudioToolGroup& group = *studio.group("paint");

		tablet.driver.click(*group.button);
		tablet.driver.settle();
		REQUIRE CHECK(group.button->isPopupOpen());
		CHECK(group.menu.windowRect().pos.x + group.menu.windowRect().size.x <= group.button->windowRect().pos.x + 0.5f);
		tablet.driver.click(*group.menu.items()[1]);
		tablet.driver.layout();

		CHECK(!group.button->isPopupOpen());
		CHECK(tablet.scene.classic.canvas.tool() == CanvasTool::Bucket);
		CHECK_EQUAL("fill", group.current);
		CHECK(group.button->icon().value == tablet.scene.icons.icon("fill").value);
		CHECK(group.button->isSelected());
		CHECK(!studio.toolButton("brush").isSelected());
	}

	TEST(EveryBottomButtonHasATooltipShownOnLongPress) {
		Tablet tablet({ 1280, 800 });
		for (Button* button : tablet.studio().toolButtons()) {
			CHECK(!button->tooltip().empty());
			tablet.driver.moveTo(*button);
			tablet.driver.press();
			tablet.driver.advance(Ui::kLongPressMs + 20);
			tablet.driver.release();
			CHECK(tablet.driver.ui().openTooltip() != nullptr);
			tablet.driver.click(tablet.scene.classic.canvas.windowRect().center());
		}
	}
}

SUITE(StudioActiveToolBar) {
	TEST(EachSliderHasAGlyphBelowIt) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("brush");

		const RectF size = studio.size.windowRect();
		const RectF sizeMark = studio.sizeMark.windowRect();
		const RectF opacity = studio.opacity.windowRect();
		const RectF opacityMark = studio.opacityMark.windowRect();
		CHECK(studio.sizeMark.isVisible());
		CHECK(studio.opacityMark.isVisible());
		CHECK(sizeMark.pos.y >= size.pos.y + size.size.y);
		CHECK(sizeMark.pos.y + sizeMark.size.y <= opacity.pos.y);
		CHECK(opacityMark.pos.y >= opacity.pos.y + opacity.size.y);
		CHECK(inside(studio.activeBar.windowRect(), opacityMark));
		CHECK(studio.sizeMark.glyph() == SliderGlyph::Size);
		CHECK(studio.opacityMark.glyph() == SliderGlyph::Opacity);

		tablet.tool("eyedropper");
		CHECK(!studio.sizeMark.isVisible());
	}

	TEST(DraggingASliderShowsTheRealBrushCursorAtTheCanvasCenter) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		CanvasWidget& canvas = tablet.scene.classic.canvas;
		tablet.tool("brush");
		tablet.scene.document.setZoom(0.5f);
		CHECK(!canvas.brushPreview());

		tablet.driver.moveTo(studio.size.windowPointForValue(20));
		tablet.driver.press();
		tablet.driver.moveTo(studio.size.windowPointForValue(150));
		REQUIRE CHECK(canvas.brushPreview() == BrushParameter::Size);
		const BrushCursorShape shape = canvas.brushPreviewShape();
		CHECK_CLOSE(640.0f, shape.center.x, 0.5f);
		CHECK_CLOSE(400.0f, shape.center.y, 0.5f);
		CHECK_CLOSE(tablet.scene.brush.dabAt(1).radius() * 0.5f, shape.radius, 0.01f);
		const RectF backdrop = canvas.brushPreviewBackdrop();
		CHECK(backdrop.size.x > shape.radius * 2);
		CHECK_EQUAL(std::to_string((int) std::round(tablet.scene.brush.size())) + " px", canvas.brushPreviewText());
		CHECK(tablet.driver.paint().hasText(canvas.brushPreviewText()));
		tablet.driver.release();
		CHECK(!canvas.brushPreview());

		tablet.driver.moveTo(studio.opacity.windowPointForValue(0.9f));
		tablet.driver.press();
		tablet.driver.moveTo(studio.opacity.windowPointForValue(0.4f));
		REQUIRE CHECK(canvas.brushPreview() == BrushParameter::Opacity);
		CHECK_EQUAL(std::to_string((int) std::round(tablet.scene.brush.opacity() * 100)) + "%", canvas.brushPreviewText());
		tablet.driver.release();
		CHECK(!canvas.brushPreview());
		CHECK_EQUAL(0u, tablet.scene.document.tileCount());
	}

	TEST(BrushOptionsSlidersAlsoShowTheCanvasPreview) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		CanvasWidget& canvas = tablet.scene.classic.canvas;
		BrushPopup& popup = tablet.scene.classic.toolBar.brushPopup;
		tablet.driver.click(studio.brushOptions);
		tablet.driver.settle();
		REQUIRE CHECK(studio.brushOptions.isPopupOpen());

		tablet.driver.moveTo(popup.size.slider().windowPointForValue(30));
		tablet.driver.press();
		tablet.driver.moveTo(popup.size.slider().windowPointForValue(120));
		CHECK(canvas.brushPreview() == BrushParameter::Size);
		tablet.driver.release();
		CHECK(!canvas.brushPreview());

		tablet.driver.moveTo(popup.opacity.slider().windowPointForValue(0.8f));
		tablet.driver.press();
		tablet.driver.moveTo(popup.opacity.slider().windowPointForValue(0.3f));
		CHECK(canvas.brushPreview() == BrushParameter::Opacity);
		CHECK_EQUAL(std::to_string((int) std::round(tablet.scene.brush.opacity() * 100)) + "%", canvas.brushPreviewText());
		tablet.driver.release();
		CHECK(!canvas.brushPreview());
	}

	TEST(BrushShowsTheSizeAndOpacitySliders) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("brush");

		CHECK(studio.activePanel() == ActivePanel::Brush);
		CHECK(studio.toolButton("brush").isSelected());
		CHECK(studio.size.isVisible());
		CHECK(studio.opacity.isVisible());
		CHECK(!studio.applyTransform.isVisible());
		CHECK(!studio.deselect.isVisible());
		CHECK(studio.size.orientation() == Axis::Vertical);
	}

	TEST(EraserAlsoShowsTheSizeAndOpacitySliders) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("eraser");

		CHECK(tablet.scene.brush.type == BrushType::Eraser);
		CHECK(studio.toolButton("eraser").isSelected());
		CHECK(!studio.toolButton("brush").isSelected());
		CHECK(studio.size.isVisible());
		CHECK(studio.opacity.isVisible());
	}

	TEST(SlidersFollowTheBrushOfEachTool) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("brush");
		tablet.driver.setSlider(studio.size, 200);
		const halnf brushSize = tablet.scene.brush.size();
		CHECK_CLOSE(200.0f, brushSize, 200 * 0.08f);

		tablet.tool("eraser");
		tablet.driver.setSlider(studio.size, 40);
		CHECK_CLOSE(40.0f, tablet.scene.brush.size(), 40 * 0.08f);
		CHECK_CLOSE(tablet.scene.brush.size(), studio.size.value(), 0.01f);

		tablet.tool("brush");
		CHECK_CLOSE(brushSize, tablet.scene.brush.size(), 0.01f);
		CHECK_CLOSE(brushSize, studio.size.value(), 0.01f);
	}

	TEST(OpacitySliderChangesTheBrushAndThePaint) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("brush");
		tablet.driver.setSlider(studio.opacity, 0.5f);

		CHECK_CLOSE(0.5f, tablet.scene.brush.opacity(), 0.03f);
		CHECK_CLOSE(tablet.scene.brush.opacity(), tablet.scene.classic.toolBar.brushPopup.opacity.value(), 0.001f);
	}

	TEST(SliderChangesFromThePopupShowOnTheBar) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.scene.brush.setSize(77);
		tablet.scene.brush.setOpacity(0.25f);
		tablet.scene.classic.refreshFromState();

		CHECK_CLOSE(77.0f, studio.size.value(), 0.01f);
		CHECK_CLOSE(0.25f, studio.opacity.value(), 0.01f);
	}

	TEST(TransformShowsItsActionsInsteadOfSliders) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.paintStroke({ 500, 400 }, { 700, 420 });
		tablet.tool("transform");

		REQUIRE CHECK(studio.activePanel() == ActivePanel::Transform);
		CHECK(studio.applyTransform.isVisible());
		CHECK(studio.cancelTransform.isVisible());
		CHECK(studio.flip.isVisible());
		CHECK(!studio.size.isVisible());
		CHECK(!tablet.scene.classic.transformBar.isVisible());
		CHECK(inside(studio.activeBar.windowRect(), studio.applyTransform.windowRect()));

		tablet.driver.click(studio.applyTransform);
		tablet.driver.layout();
		CHECK(!tablet.scene.document.isTransforming());
	}

	TEST(LassoShowsSelectionActions) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.paintStroke({ 400, 400 }, { 700, 400 });
		tablet.tool("lasso");

		CHECK(studio.activePanel() == ActivePanel::Selection);
		CHECK(studio.toolButton("lasso").isSelected());
		CHECK(studio.deselect.isVisible());
		CHECK(studio.invertSelection.isVisible());
		CHECK(!studio.size.isVisible());

		tablet.scene.classic.commands.run(Command::SelectAllAsRectangle);
		CHECK(tablet.scene.document.hasSelection());
		tablet.driver.click(studio.invertSelection);
		CHECK(tablet.scene.classic.commands.isEnabled(Command::Deselect));
		tablet.driver.click(studio.deselect);
		CHECK(!tablet.scene.document.hasSelection());
	}

	TEST(PickerHasNoActiveBar) {
		Tablet tablet({ 1280, 800 });
		tablet.tool("eyedropper");

		CHECK(tablet.studio().activePanel() == ActivePanel::None);
		CHECK(!tablet.studio().activeBar.isVisible());
	}

	TEST(PaintingWorksAndUndoRedoFollowHistory) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		CHECK(!studio.undo.isEffectivelyEnabled());

		tablet.paintStroke({ 400, 400 }, { 700, 400 });
		CHECK(tablet.scene.document.tileCount() > 0);
		tablet.driver.layout();
		CHECK(studio.undo.isEffectivelyEnabled());

		tablet.driver.click(studio.undo);
		CHECK_EQUAL(0u, tablet.scene.document.tileCount());
		CHECK(studio.redo.isEffectivelyEnabled());
		tablet.driver.click(studio.redo);
		CHECK(tablet.scene.document.tileCount() > 0);
	}

	TEST(TouchingTheBarsDoesNotPaint) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		const RectF active = studio.activeBar.windowRect();
		tablet.driver.drag(active.center(), active.center() + Vec2F(0, 40));
		tablet.driver.click(studio.toolBar.windowRect().pos + Vec2F(2, 2));
		tablet.driver.advance(500);
		tablet.scene.document.settlePaint();

		CHECK_EQUAL(0u, tablet.scene.document.tileCount());
	}
}

SUITE(StudioTopBars) {
	TEST(LibraryButtonOpensTheLibrary) {
		Tablet tablet({ 1280, 800 });
		tablet.driver.click(tablet.studio().library);
		CHECK(tablet.scene.classic.library.isOpen());
	}

	TEST(SettingsButtonOpensTheMenuPages) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.driver.click(studio.settingsButton);

		REQUIRE CHECK(studio.settings.isOpen());
		for (const char* expected : { "File", "Edit", "View", "Layer", "Select", "Brush", "Debug" })
			CHECK(studio.settings.currentPage().row(expected) != nullptr);
	}

	TEST(PopupsOpenBelowTheirButtonsAndStayOnScreen) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		for (PopupButton* button :
				 std::initializer_list<PopupButton*>{ &studio.presets, &studio.brushOptions, &studio.color, &studio.layers }) {
			tablet.driver.click(*button);
			tablet.driver.settle();
			REQUIRE CHECK(button->isPopupOpen());
			tablet.driver.click(*button);
			tablet.driver.settle();
		}
		tablet.driver.click(studio.layers);
		tablet.driver.settle();
		const RectF popup = tablet.scene.classic.toolBar.layersPopup.windowRect();
		CHECK(inside(makeRect({ 0, 0 }, { 1280, 800 }), popup));
		CHECK(popup.pos.y >= studio.topRight.windowRect().pos.y + studio.topRight.windowRect().size.y);
	}

	TEST(ChoosingAPresetKeepsTheChosenBrush) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		tablet.tool("eraser");
		tablet.driver.click(studio.presets);
		tablet.driver.settle();

		CHECK(ToolStrip::isPaintingBrush(tablet.scene.brush));
		CHECK(studio.toolButton("brush").isSelected());
		CHECK(studio.size.isVisible());
	}

	TEST(BackOnTheCanvasOpensTheLibrary) {
		Tablet tablet({ 1280, 800 });
		tablet.driver.key(InputID::ESCAPE);
		CHECK(tablet.scene.classic.library.isOpen());
	}
}

namespace {
	struct Desktop : Tablet {
		explicit Desktop(Vec2F size) :
			Tablet(size, InteractionMode::Pointer) {}

		MenuBar& menuBar() { return scene.classic.menuBar; }

		PopupButton& menuTitle(const std::string& title) {
			for (const auto& button : menuBar().titles())
				if (button->text() == title) return *button;
			throw std::runtime_error("menu not found: " + title);
		}

		MenuItem& menuItem(Menu& menu, const std::string& text) {
			for (const auto& item : menu.items())
				if (item->entry().text == text) return *item;
			throw std::runtime_error("menu item not found: " + text);
		}
	};

	halnf bottomOf(const RectF& rect) { return rect.pos.y + rect.size.y; }
}

SUITE(StudioDesktopMenuBar) {
	TEST(MenuBarSitsAboveTheStudioControls) {
		Desktop desktop({ 1280, 800 });
		StudioRoot& studio = desktop.studio();
		const RectF bar = desktop.menuBar().windowRect();

		REQUIRE CHECK(studio.isMenuBarShown());
		CHECK(desktop.menuBar().isVisible());
		CHECK_CLOSE(0.0f, bar.pos.y, 0.01f);
		CHECK_CLOSE(1280.0f, bar.size.x, 0.01f);
		CHECK(bar.size.y > 0);
		CHECK(desktop.scene.classic.canvas.windowRect().pos.y >= bottomOf(bar) - 0.01f);
		for (Panel* panel : { &studio.topLeft, &studio.topRight, &studio.toolBar, &studio.activeBar }) {
			if (!panel->isVisible()) continue;
			CHECK(!intersects(bar, panel->windowRect()));
			CHECK(panel->windowRect().pos.y >= bottomOf(bar));
		}
		CHECK(!desktop.scene.classic.toolBar.isVisible());
		CHECK(!desktop.scene.classic.toolStrip.isVisible());
	}

	TEST(TouchFirstKeepsTheMenuBarHidden) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();

		CHECK(!studio.isMenuBarShown());
		CHECK(!tablet.scene.classic.menuBar.isVisible());
		CHECK_CLOSE(0.0f, tablet.scene.classic.canvas.windowRect().pos.y, 0.01f);
		CHECK_CLOSE(StudioRoot::kEdgeMargin * studio.scale(), studio.topLeft.windowRect().pos.y, 0.01f);
	}

	TEST(HidingTheMenuBarAgainGivesTheSpaceBack) {
		Desktop desktop({ 1280, 800 });
		desktop.driver.ui().setInteractionMode(InteractionMode::Touch);
		desktop.driver.layout();

		CHECK(!desktop.menuBar().isVisible());
		CHECK_CLOSE(0.0f, desktop.scene.classic.canvas.windowRect().pos.y, 0.01f);
	}

	TEST(ViewMenuZoomInZoomsTheDocument) {
		Desktop desktop({ 1280, 800 });
		const halnf before = desktop.scene.document.zoom();

		desktop.driver.click(desktop.menuTitle("View"));
		desktop.driver.settle();
		REQUIRE CHECK(desktop.scene.classic.viewMenu.isVisible());
		desktop.driver.click(desktop.menuItem(desktop.scene.classic.viewMenu, "Zoom in"));
		desktop.driver.advance(500);

		CHECK(desktop.scene.document.zoom() > before * 1.2f);
	}

	TEST(PaintingBelowTheMenuBarStillWorks) {
		Desktop desktop({ 1280, 800 });

		desktop.paintStroke({ 400, 400 }, { 700, 400 });

		CHECK(desktop.scene.document.tileCount() > 0);
		desktop.driver.layout();
		CHECK(desktop.studio().undo.isEffectivelyEnabled());
	}
}

SUITE(StudioOneMenu) {
	TEST(MenuBarAndMenuPagesShowTheSameMenus) {
		Desktop desktop({ 1280, 800 });
		const Menu& menu = desktop.scene.classic.mainMenu;
		const auto& titles = desktop.menuBar().titles();
		REQUIRE CHECK_EQUAL(menu.items().size(), titles.size());
		for (size_t index = 0; index < titles.size(); index++) {
			CHECK_EQUAL(menu.items()[index]->entry().text, titles[index]->text());
			desktop.driver.click(*titles[index]);
			desktop.driver.settle();
			CHECK(menu.items()[index]->entry().submenu->isOpen());
			desktop.driver.click(*titles[index]);
			desktop.driver.settle();
		}
	}

	TEST(TabletShowsTheGearAndNoMenuBar) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		CHECK(studio.settingsButton.isVisible());
		CHECK(!tablet.scene.classic.menuBar.isVisible());

		tablet.driver.click(studio.settingsButton);
		REQUIRE CHECK(studio.settings.isOpen());
		for (const auto& item : tablet.scene.classic.mainMenu.items())
			CHECK(studio.settings.currentPage().row(item->entry().text) != nullptr);
	}

	TEST(DesktopShowsTheMenuBarAndNoGear) {
		Desktop desktop({ 1280, 800 });
		StudioRoot& studio = desktop.studio();
		CHECK(!studio.settingsButton.isVisible());
		CHECK(desktop.menuBar().isVisible());
		const RectF undo = studio.undo.windowRect();
		const RectF bar = studio.topLeft.windowRect();
		CHECK(bar.pos.x + bar.size.x <= undo.pos.x + undo.size.x + StudioRoot::kButton);
	}
}

SUITE(StudioDebugMenu) {
	TEST(DebugMenuSwitchesBetweenTabletAndDesktopLayout) {
		Tablet tablet({ 1280, 800 });
		StudioRoot& studio = tablet.studio();
		Menu& debug = tablet.scene.classic.debugMenu;
		const MenuItem* toggle = nullptr;
		for (const auto& item : debug.items())
			if (item->entry().text == "Desktop layout") toggle = item.get();
		REQUIRE CHECK(toggle != nullptr);
		CHECK(!toggle->entry().isChecked());

		toggle->entry().action();
		tablet.driver.layout();
		CHECK(!tablet.driver.ui().isTouchMode());
		CHECK(studio.isMenuBarShown());
		CHECK(toggle->entry().isChecked());
		CHECK(!studio.settingsButton.isVisible());

		toggle->entry().action();
		tablet.driver.layout();
		CHECK(tablet.driver.ui().isTouchMode());
		CHECK(!studio.isMenuBarShown());
		CHECK(studio.settingsButton.isVisible());
	}

	TEST(TileStatisticsLiveInTheDebugMenu) {
		Tablet tablet({ 1280, 800 });
		bool inDebug = false;
		for (const auto& item : tablet.scene.classic.debugMenu.items())
			inDebug = inDebug || item->entry().text == "Show Tile Statistics";
		CHECK(inDebug);
		for (const auto& item : tablet.scene.classic.viewMenu.items())
			CHECK(item->entry().text != "Show Tile Statistics");
	}
}
