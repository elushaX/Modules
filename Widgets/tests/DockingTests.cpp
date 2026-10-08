#include "Docking.hpp"
#include "Probes.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <stdexcept>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	MenuItem& menuItem(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return *item;
		}
		throw std::runtime_error("no menu item " + text);
	}

	void chooseViewMode(UiDriver& driver, DockPanel& panel, const std::string& mode) {
		driver.click(panel.optionsButton());
		driver.hoverFor(menuItem(panel.optionsMenu(), "View Mode"), 50);
		Menu& modes = *menuItem(panel.optionsMenu(), "View Mode").entry().submenu;
		driver.click(menuItem(modes, mode));
	}

	struct DockScene {
		DockArea area;
		EventProbe editor{ { 100, 100 }, "editor" };
		DockPanel project{ "Project", DockSide::Left };
		DockPanel structure{ "Structure", DockSide::Left };
		DockPanel terminal{ "Terminal", DockSide::Bottom };
		DockPanel inspector{ "Inspector", DockSide::Right };
		Box projectContent;
		Box terminalContent;

		DockScene() {
			editor.setSizing(AxisSizing::fill(), AxisSizing::fill());
			area.setCenter(&editor);

			project.setContent(&projectContent);
			terminal.setContent(&terminalContent);

			for (DockPanel* panel : { &project, &structure, &terminal, &inspector })
				area.addPanel(*panel);
		}

		[[nodiscard]] Vec2F dropPoint(DockSide side) const {
			return area.windowPosition() + area.dropTarget(side).center();
		}

		[[nodiscard]] Vec2F headerPoint(const DockPanel& panel) const {
			return panel.windowPosition() + Vec2F(20.0f, panel.headerRect().pos.y + panel.headerRect().size.y / 2);
		}
	};

	const Vec2F kWindow = { 1200, 800 };
	const halnf kGap = Theme::standard().metrics.surfaceGap;
}

SUITE(DockStripes) {
	TEST(StripesListPanelsAndPanelsStartHidden) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);

		CHECK(scene.area.stripeRect(DockSide::Left).size.x > 0);
		CHECK(scene.area.stripeRect(DockSide::Right).size.x > 0);
		CHECK(!scene.project.isVisible());
		CHECK(scene.area.stripeButton(scene.project) != nullptr);
		CHECK_CLOSE(scene.area.stripeRect(DockSide::Left).size.x, scene.editor.rect().pos.x, 0.01f);
	}

	TEST(ClickingStripeButtonTogglesPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		Button& button = *scene.area.stripeButton(scene.project);

		driver.click(button);
		CHECK(scene.project.isOpen());
		CHECK(scene.project.isVisible());
		CHECK(button.isSelected());

		driver.click(button);
		CHECK(!scene.project.isOpen());
		CHECK(!button.isSelected());
	}

	TEST(BottomPanelsStackAtTheBottomOfTheLeftStripe) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);

		const RectF projectButton = scene.area.stripeButton(scene.project)->rect();
		const RectF terminalButton = scene.area.stripeButton(scene.terminal)->rect();

		CHECK(projectButton.pos.y < 100);
		CHECK(terminalButton.pos.y > kWindow.y - 100);
	}

	TEST(StripeButtonShowsFullTitleVertically) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		StripeButton& button = *scene.area.stripeButton(scene.project);

		CHECK_EQUAL("Project", button.text());
		CHECK_EQUAL("Project", button.tooltip());
		CHECK(button.size().y > button.size().x);
		CHECK(button.size().y >= button.textWidth("Project", Theme::standard().metrics.smallFontSize));
		CHECK(driver.paint().hasText("Project"));
	}

	TEST(StripeButtonsOnOneRailDoNotOverlap) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);

		const RectF project = scene.area.stripeButton(scene.project)->rect();
		const RectF structure = scene.area.stripeButton(scene.structure)->rect();

		CHECK(structure.pos.y >= project.pos.y + project.size.y);
	}

	TEST(StripeButtonsHaveFillAndBorder) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		const RectF button = scene.area.stripeButton(scene.project)->windowRect();

		bool filled = false;
		bool bordered = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (!(command.rect == button)) continue;
			filled |= command.kind == PaintCommand::Kind::FillRect && command.color.a > 0;
			bordered |= command.kind == PaintCommand::Kind::StrokeRect;
		}

		CHECK(filled);
		CHECK(bordered);
	}
}

SUITE(DockedPanels) {
	TEST(PinnedPanelPushesMainViewAside) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		const halnf stripe = scene.area.stripeRect(DockSide::Left).size.x;

		scene.area.openPanel(scene.project);
		driver.layout();

		CHECK_CLOSE(stripe, scene.project.rect().pos.x, 0.01f);
		CHECK_CLOSE(260, scene.project.size().x, 0.01f);
		CHECK_CLOSE(stripe + 260 + kGap, scene.editor.rect().pos.x, 0.01f);
	}

	TEST(OnlyOnePanelPerSideIsOpen) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);

		scene.area.openPanel(scene.project);
		scene.area.openPanel(scene.structure);

		CHECK(!scene.project.isOpen());
		CHECK(scene.structure.isOpen());
	}

	TEST(PanelsOnDifferentSidesCoexist) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);

		scene.area.openPanel(scene.project);
		scene.area.openPanel(scene.terminal);
		scene.area.openPanel(scene.inspector);
		driver.layout();

		const RectF editor = scene.editor.windowRect();
		CHECK(scene.project.isOpen() && scene.terminal.isOpen() && scene.inspector.isOpen());
		CHECK_CLOSE(scene.project.windowRect().pos.x + scene.project.size().x + kGap, editor.pos.x, 0.01f);
		CHECK_CLOSE(scene.inspector.windowRect().pos.x, editor.pos.x + editor.size.x + kGap, 0.01f);
		CHECK_CLOSE(scene.terminal.windowRect().pos.y, editor.pos.y + editor.size.y + kGap, 0.01f);
		CHECK_CLOSE(editor.pos.x, scene.terminal.windowRect().pos.x, 0.01f);
	}

	TEST(DraggingEdgeResizesDockedPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.layout();

		const RectF panel = scene.project.windowRect();
		const Vec2F edge = { panel.pos.x + panel.size.x + kGap / 2, panel.center().y };
		driver.drag(edge, edge + Vec2F(140.0f, 0.0f));

		CHECK_CLOSE(400, scene.project.size().x, 1);
		CHECK_CLOSE(scene.project.windowRect().pos.x + 400 + kGap, scene.editor.windowRect().pos.x, 1);
	}

	TEST(ResizeIsLimitedSoMainViewStaysVisible) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.terminal);
		driver.layout();

		const RectF panel = scene.terminal.windowRect();
		const Vec2F edge = { panel.center().x, panel.pos.y - kGap / 2 };
		driver.drag(edge, Vec2F(edge.x, -500.0f));

		CHECK(scene.editor.size().y > 0);
		CHECK(scene.terminal.size().y <= kWindow.y * 0.86f);
	}

	TEST(HideButtonClosesPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);

		driver.click(scene.project.hideButton());

		CHECK(!scene.project.isOpen());
	}
}

SUITE(ResizeHandles) {
	TEST(EdgeHandleSitsCenteredInTheGapAndIsHiddenUntilHovered) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();

		ResizeHandle& handle = scene.area.edgeHandle(DockSide::Left);
		const RectF panel = scene.project.windowRect();
		const RectF line = translated(handle.lineRect(), handle.windowPosition());
		CHECK_CLOSE(panel.pos.x + panel.size.x + kGap / 2, line.center().x, 0.01f);
		CHECK_CLOSE(0, handle.revealAmount(), 0.01f);

		driver.moveTo(handle.windowRect().center());
		driver.settle();
		CHECK_CLOSE(1, handle.revealAmount(), 0.01f);

		driver.moveTo(scene.editor.windowRect().center());
		driver.settle();
		CHECK(!handle.isHovered());
		CHECK_CLOSE(0, handle.revealAmount(), 0.01f);
	}

	TEST(EdgeHandleStaysLitWhileDraggingOutsideItAndFadesAfter) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();
		ResizeHandle& handle = scene.area.edgeHandle(DockSide::Left);

		driver.moveTo(handle.windowRect().center());
		driver.press();
		driver.moveTo(handle.windowRect().center() + Vec2F(120.0f, 0.0f));
		driver.settle();
		CHECK_CLOSE(1, handle.revealAmount(), 0.01f);

		driver.release();
		driver.moveTo(scene.editor.windowRect().center());
		driver.settle();
		CHECK_CLOSE(0, handle.revealAmount(), 0.01f);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(OnlyPinnedDockedSidesHaveHandles) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();
		CHECK(scene.area.edgeHandle(DockSide::Left).isVisible());
		CHECK(!scene.area.edgeHandle(DockSide::Right).isVisible());

		chooseViewMode(driver, scene.project, "Float");
		CHECK(!scene.area.edgeHandle(DockSide::Left).isVisible());
		CHECK(scene.project.cornerGrip().isVisible());
	}
}

SUITE(UnpinnedPanels) {
	TEST(UnpinnedPanelOverlaysMainViewInsteadOfPushingIt) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		const halnf editorLeft = scene.editor.rect().pos.x;
		scene.area.openPanel(scene.project);

		chooseViewMode(driver, scene.project, "Dock Unpinned");
		driver.layout();

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Undocked);
		CHECK(scene.project.isOpen());
		CHECK_CLOSE(editorLeft, scene.editor.rect().pos.x, 0.01f);
		CHECK(driver.ui().widgetAt(scene.project.windowRect().center()) != &scene.editor);
	}

	TEST(UnpinnedPanelHidesWhenFocusMovesToMainView) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");

		driver.click(scene.editor.windowRect().pos + scene.editor.size() - Vec2F(20.0f, 20.0f));

		CHECK(!scene.project.isOpen());
		CHECK_EQUAL(1, scene.editor.count(Event::Type::Click));
	}

	TEST(UnpinnedPanelStaysOpenWhileUsingIt) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");

		driver.click(scene.projectContent);

		CHECK(scene.project.isOpen());
	}

	TEST(StripeButtonTogglesUnpinnedPanelWithoutFlicker) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");
		driver.click(scene.editor);
		REQUIRE CHECK(!scene.project.isOpen());

		driver.click(*scene.area.stripeButton(scene.project));
		CHECK(scene.project.isOpen());

		driver.click(*scene.area.stripeButton(scene.project));
		CHECK(!scene.project.isOpen());
	}

	TEST(PinningAgainRestoresDockedLayout) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");

		chooseViewMode(driver, scene.project, "Dock Pinned");
		driver.layout();

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Docked);
		CHECK_CLOSE(
			scene.project.windowRect().pos.x + scene.project.size().x + kGap, scene.editor.windowRect().pos.x, 0.01f
		);
	}
}

SUITE(PanelOptionsMenu) {
	TEST(HeaderHasOnlyTitleOptionsAndHide) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.layout();

		CHECK(scene.project.optionsButton().size().x <= 24);
		CHECK(scene.project.hideButton().size().x <= 24);
		CHECK(driver.paint().hasText("Project"));
		CHECK(!driver.paint().hasText("Pin"));
		CHECK(!driver.paint().hasText("Float"));
	}

	TEST(OptionsMenuMarksCurrentViewModeAndSide) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);

		driver.click(scene.project.optionsButton());
		CHECK(driver.ui().isPopupOpen(scene.project.optionsMenu()));

		Menu& modes = *menuItem(scene.project.optionsMenu(), "View Mode").entry().submenu;
		Menu& sides = *menuItem(scene.project.optionsMenu(), "Move To").entry().submenu;
		CHECK(menuItem(modes, "Dock Pinned").entry().isChecked());
		CHECK(!menuItem(modes, "Float").entry().isChecked());
		CHECK(menuItem(sides, "Left").entry().isChecked());
	}

	TEST(MoveToRightDocksPanelOnRight) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);

		driver.click(scene.project.optionsButton());
		driver.hoverFor(menuItem(scene.project.optionsMenu(), "Move To"), 50);
		Menu& sides = *menuItem(scene.project.optionsMenu(), "Move To").entry().submenu;
		driver.click(menuItem(sides, "Right"));
		driver.settle();

		CHECK(scene.project.side() == DockSide::Right);
		CHECK(!driver.ui().isPopupOpen(scene.project.optionsMenu()));
		CHECK_CLOSE(
			scene.project.windowRect().pos.x, scene.editor.windowRect().pos.x + scene.editor.size().x + kGap, 0.01f
		);
	}

	TEST(UsingOptionsMenuDoesNotHideUnpinnedPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");
		REQUIRE CHECK(scene.project.isOpen());

		driver.click(scene.project.optionsButton());
		driver.hoverFor(menuItem(scene.project.optionsMenu(), "Move To"), 50);

		CHECK(scene.project.isOpen());
	}
}

SUITE(DraggingPanels) {
	TEST(DraggingHeaderUndocksAndShowsDropPreviewOverTargetSide) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.layout();

		driver.moveTo(scene.headerPoint(scene.project));
		driver.press();
		driver.moveTo(Vec2F(600.0f, 300.0f));

		CHECK(scene.project.isDragging());
		CHECK(scene.area.isDraggingPanel());
		CHECK(!scene.area.isPreviewingDrop());
		CHECK(scene.editor.rect().pos.x < 100);

		driver.moveTo(scene.dropPoint(DockSide::Right));
		REQUIRE CHECK(scene.area.isPreviewingDrop());
		driver.settle();
		CHECK(scene.area.dropPreview().pos.x > kWindow.x / 2);
		CHECK(scene.area.dropPreview().size.y > kWindow.y / 2);

		driver.release();
		CHECK(scene.project.side() == DockSide::Right);
		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Docked);
		CHECK(!scene.area.isPreviewingDrop());
		driver.layout();
		CHECK_CLOSE(
			scene.project.windowRect().pos.x, scene.editor.windowRect().pos.x + scene.editor.size().x + kGap, 0.01f
		);
	}

	TEST(DroppingOnSideMovesStripeButtonAndReplacesOpenPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.terminal);
		scene.area.openPanel(scene.project);
		driver.layout();

		driver.drag(scene.headerPoint(scene.project), scene.dropPoint(DockSide::Bottom), 8);

		CHECK(scene.project.side() == DockSide::Bottom);
		CHECK(scene.project.isOpen());
		CHECK(!scene.terminal.isOpen());
		CHECK(scene.area.stripeButton(scene.project)->rect().pos.y > kWindow.y / 2);
	}

	TEST(DroppingInTheMiddleFloatsThePanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.layout();

		driver.drag(scene.headerPoint(scene.project), Vec2F(600.0f, 300.0f), 8);
		driver.layout();

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Float);
		CHECK(scene.project.isOpen());
		CHECK(contains(scene.project.windowRect(), Vec2F(600.0f, 300.0f)));
		CHECK_CLOSE(scene.area.stripeRect(DockSide::Left).size.x, scene.editor.rect().pos.x, 0.01f);
	}

	TEST(ShortPressOnHeaderDoesNotUndock) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.layout();

		const Vec2F point = scene.headerPoint(scene.project);
		driver.drag(point, point + Vec2F(2.0f, 1.0f), 1);

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Docked);
		CHECK(scene.project.side() == DockSide::Left);
	}
}

SUITE(FloatingDockPanels) {
	TEST(FloatButtonFloatsAndMovingKeepsItFloating) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Float");
		driver.layout();

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Float);
		const Vec2F start = scene.project.windowPosition();

		const Vec2F grab = scene.headerPoint(scene.project);
		driver.drag(grab, grab + Vec2F(120.0f, 80.0f), 6);

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Float);
		CHECK_CLOSE(start.x + 120, scene.project.windowPosition().x, 1);
		CHECK_CLOSE(start.y + 80, scene.project.windowPosition().y, 1);
	}

	TEST(CornerGripResizesFloatingPanelWithMinimum) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Float");
		driver.layout();
		const Vec2F before = scene.project.size();

		const Vec2F corner = scene.project.windowPosition() + before - Vec2F(3.0f, 3.0f);
		driver.drag(corner, corner + Vec2F(60.0f, 40.0f));
		CHECK_CLOSE(before.x + 60, scene.project.size().x, 1);
		CHECK_CLOSE(before.y + 40, scene.project.size().y, 1);

		const Vec2F newCorner = scene.project.windowPosition() + scene.project.size() - Vec2F(3.0f, 3.0f);
		driver.drag(newCorner, newCorner - Vec2F(2000.0f, 2000.0f));
		CHECK_CLOSE(160, scene.project.size().x, 1);
		CHECK_CLOSE(120, scene.project.size().y, 1);
	}

	TEST(FloatingPanelDoesNotCloseDockedPanelOnSameSide) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Float");

		scene.area.openPanel(scene.structure);

		CHECK(scene.project.isOpen());
		CHECK(scene.structure.isOpen());
	}

	TEST(DockingFromFloatReturnsToZone) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Float");
		driver.layout();

		chooseViewMode(driver, scene.project, "Dock Pinned");
		driver.layout();

		CHECK(scene.project.viewMode() == DockPanel::ViewMode::Docked);
		CHECK_CLOSE(
			scene.project.windowRect().pos.x + scene.project.size().x + kGap, scene.editor.windowRect().pos.x, 0.01f
		);
	}
}

SUITE(DockAnimation) {
	TEST(OpeningDockedPanelAnimatesLayoutOfPanelAndMainView) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		driver.setAutoSettle(false);
		driver.settle();
		const RectF editorBefore = scene.editor.rect();

		scene.area.openPanel(scene.project);
		driver.ui().updateLayout();
		driver.advance(16);

		CHECK(scene.project.rect().size.x > 0);
		CHECK(scene.project.rect().size.x < scene.project.targetRect().size.x);
		CHECK(scene.editor.rect().pos.x > editorBefore.pos.x);
		CHECK(scene.editor.rect().pos.x < scene.editor.targetRect().pos.x);
		CHECK_CLOSE(scene.project.rect().pos.x + scene.project.rect().size.x + kGap, scene.editor.rect().pos.x, 40);

		driver.settle();
		CHECK(scene.project.rect() == scene.project.targetRect());
		CHECK(scene.editor.rect() == scene.editor.targetRect());
		CHECK_CLOSE(scene.project.rect().pos.x + scene.project.rect().size.x + kGap, scene.editor.rect().pos.x, 0.01f);
	}

	TEST(ChildrenFollowAnimatedParentGeometry) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		driver.setAutoSettle(false);
		driver.settle();

		scene.area.openPanel(scene.project);
		driver.ui().updateLayout();
		driver.advance(32);

		const RectF panel = scene.project.windowRect();
		const RectF content = scene.projectContent.windowRect();
		CHECK(content.pos.x + content.size.x <= panel.pos.x + panel.size.x + 0.01f);
	}

	TEST(GeometrySpringMatchesPreviousLibraryCurve) {
		AnimatedRect rect(makeRect(0, 0, 0, 0), Motion::fluid(), AnimatedRect::kGeometryTolerance);
		rect.animateTo(makeRect(100, 0, 0, 0));

		halnf peak = 0;
		halnf elapsed = 0;
		halnf ninetyPercentAt = -1;
		while (rect.advance(1) && elapsed < 2000) {
			elapsed += 1;
			peak = std::max(peak, rect.value().pos.x);
			if (ninetyPercentAt < 0 && rect.value().pos.x >= 90) ninetyPercentAt = elapsed;
		}

		CHECK(peak <= 100.5f);
		CHECK(ninetyPercentAt > 100 && ninetyPercentAt < 170);
		CHECK(elapsed < 600);
	}

	TEST(DraggedPanelGlidesTowardsPointer) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();
		driver.setAutoSettle(false);

		driver.moveTo(scene.headerPoint(scene.project));
		driver.press();
		driver.moveTo(Vec2F(600.0f, 300.0f));
		driver.ui().updateLayout();
		driver.advance(16);

		CHECK(!(scene.project.rect() == scene.project.targetRect()));

		driver.settle();
		CHECK(scene.project.rect() == scene.project.targetRect());
		driver.release();
	}

	TEST(DroppedPanelSpringsIntoItsZone) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();
		driver.setAutoSettle(false);

		driver.drag(scene.headerPoint(scene.project), scene.dropPoint(DockSide::Right), 8);
		driver.ui().updateLayout();
		driver.advance(16);

		CHECK(!(scene.project.rect() == scene.project.targetRect()));
		CHECK(driver.ui().needsFrame());

		driver.settle();
		CHECK(scene.project.rect() == scene.project.targetRect());
		CHECK(scene.project.side() == DockSide::Right);
	}

	TEST(EdgeResizeTracksPointerExactly) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		driver.settle();
		driver.setAutoSettle(false);

		const RectF panel = scene.project.windowRect();
		const Vec2F edge = { panel.pos.x + panel.size.x + kGap / 2, panel.center().y };
		driver.moveTo(edge);
		driver.press();
		driver.moveTo(edge + Vec2F(100.0f, 0.0f));
		driver.ui().updateLayout();

		CHECK(scene.project.rect() == scene.project.targetRect());
		CHECK(scene.editor.rect() == scene.editor.targetRect());
		driver.release();
	}
}

SUITE(DockLifecycle) {
	TEST(DestroyingPanelRemovesItsStripeButton) {
		DockArea area;
		Box center;
		area.setCenter(&center);
		UiDriver driver(area, kWindow);
		{
			DockPanel temporary("Temporary");
			area.addPanel(temporary);
			area.openPanel(temporary);
			driver.layout();
			CHECK_EQUAL(1u, area.panels().size());
		}
		CHECK(area.panels().empty());
		driver.layout();
		driver.paint();
	}

	TEST(PaintingIsBalancedInEveryMode) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		scene.area.openPanel(scene.terminal);
		CHECK(driver.paint().isBalanced());

		chooseViewMode(driver, scene.project, "Float");
		CHECK(driver.paint().isBalanced());
		CHECK(driver.paint().hasText("Project"));
	}
}

SUITE(UnpinnedPanelResize) {
	TEST(UnpinnedPanelEdgeCanBeDraggedWithoutPushingMainView) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");
		driver.settle();
		const halnf editorLeft = scene.editor.windowRect().pos.x;
		const halnf width = scene.project.size().x;

		ResizeHandle& handle = scene.area.edgeHandle(DockSide::Left);
		CHECK(handle.isVisible());
		const RectF panel = scene.project.windowRect();
		CHECK_CLOSE(panel.pos.x + panel.size.x + kGap / 2, handle.windowRect().center().x, 1.0f);

		driver.drag(handle.windowRect().center(), handle.windowRect().center() + Vec2F(80.0f, 0.0f));
		driver.settle();
		CHECK(scene.project.isOpen());
		CHECK_CLOSE(width + 80, scene.project.size().x, 1.0f);
		CHECK_CLOSE(editorLeft, scene.editor.windowRect().pos.x, 0.01f);
	}

	TEST(UnpinnedHiddenPanelHasNoHandle) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");
		scene.area.hidePanel(scene.project);
		driver.settle();
		CHECK(!scene.area.edgeHandle(DockSide::Left).isVisible());
	}

	TEST(ClickingOutsideAfterResizingHidesUnpinnedPanel) {
		DockScene scene;
		UiDriver driver(scene.area, kWindow);
		scene.area.openPanel(scene.project);
		chooseViewMode(driver, scene.project, "Dock Unpinned");
		driver.settle();

		ResizeHandle& handle = scene.area.edgeHandle(DockSide::Left);
		driver.drag(handle.windowRect().center(), handle.windowRect().center() + Vec2F(60.0f, 0.0f));
		driver.settle();
		CHECK(scene.project.isOpen());

		driver.click(scene.editor.windowRect().center() + Vec2F(200.0f, 0.0f));
		driver.settle();
		CHECK(!scene.project.isOpen());
	}
}
