#include "BoxLayout.hpp"
#include "Dialog.hpp"
#include "FakeWindowHost.hpp"
#include "TextField.hpp"
#include "Probes.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct DialogScene {
		Widget root;
		EventProbe behind{ { 200, 100 }, "behind" };

		DialogScene() {
			root.emplaceLayout<ColumnLayout>();
			root.addChild(behind);
		}
	};
}

SUITE(DialogBehaviour) {
	TEST(OpensCenteredAndModal) {
		DialogScene scene;
		Dialog dialog("Title");
		dialog.addButton("Ok", {}, true);
		UiDriver driver(scene.root, { 800, 600 });

		dialog.open(driver.ui());
		driver.settle();

		CHECK(dialog.isOpen());
		const RectF rect = dialog.windowRect();
		CHECK_CLOSE(400, rect.pos.x + rect.size.x / 2, 1.0f);
		CHECK_CLOSE(150, rect.pos.y, 1.0f);

		driver.click(scene.behind);
		CHECK_EQUAL(0, scene.behind.count(Event::Type::Click));
		CHECK(dialog.isOpen());
	}

	TEST(DialogOwnedByAPopupKeepsThatPopupOpenAfterClosing) {
		DialogScene scene;
		EventProbe sheet{ { 300, 200 }, "sheet" };
		Dialog dialog("Title");
		dialog.addButton("Ok", {}, true);
		UiDriver driver(scene.root, { 800, 600 });
		PopupRequest request;
		request.content = &sheet;
		request.anchor = driver.ui().viewport();
		request.options.placement = Placement::Over;
		request.options.modal = true;
		driver.ui().openPopup(request);

		dialog.open(driver.ui(), &sheet);
		driver.click(*dialog.buttons().front());

		CHECK(!dialog.isOpen());
		CHECK(driver.ui().isPopupOpen(sheet));
	}

	TEST(ClosingADialogWithAFocusedFieldReleasesTextInput) {
		DialogScene scene;
		FakeWindowHost host;
		Dialog dialog("Name");
		TextField field;
		dialog.body().addChild(field);
		dialog.addButton("Ok", {}, true);
		UiDriver driver(scene.root, { 800, 600 });
		driver.ui().setWindowHost(&host);
		dialog.open(driver.ui());
		driver.click(field);
		REQUIRE CHECK(host.textInputEnabled);

		driver.click(*dialog.buttons().front());

		CHECK(!dialog.isOpen());
		CHECK(!host.textInputEnabled);
		driver.ui().setWindowHost(nullptr);
	}

	TEST(ButtonsCloseAndRunActions) {
		DialogScene scene;
		Dialog dialog("Title");
		int accepted = 0;
		dialog.addButton("Ok", [&accepted]() { accepted++; }, true);
		dialog.addButton("Cancel", {});
		UiDriver driver(scene.root, { 800, 600 });

		dialog.open(driver.ui());
		driver.click(*dialog.buttons()[0]);
		CHECK_EQUAL(1, accepted);
		CHECK(!dialog.isOpen());

		dialog.open(driver.ui());
		driver.click(*dialog.buttons()[1]);
		CHECK_EQUAL(1, accepted);
		CHECK(!dialog.isOpen());
	}

	TEST(EnterAcceptsAndEscapeCancels) {
		DialogScene scene;
		Dialog dialog("Title");
		int accepted = 0;
		int cancelled = 0;
		dialog.addButton("Ok", [&accepted]() { accepted++; }, true);
		dialog.setOnCancel([&cancelled]() { cancelled++; });
		UiDriver driver(scene.root, { 800, 600 });

		dialog.open(driver.ui());
		driver.key(InputID::ENTER);
		CHECK_EQUAL(1, accepted);
		CHECK_EQUAL(0, cancelled);

		dialog.open(driver.ui());
		driver.key(InputID::ESCAPE);
		CHECK_EQUAL(1, accepted);
		CHECK_EQUAL(1, cancelled);
		CHECK(!dialog.isOpen());
	}

	TEST(CancelButtonCancelsLikeEscape) {
		DialogScene scene;
		Dialog dialog("Title");
		int accepted = 0;
		int cancelled = 0;
		dialog.addButton("Ok", [&accepted]() { accepted++; }, true);
		dialog.addCancelButton("Back");
		dialog.setOnCancel([&cancelled]() { cancelled++; });
		UiDriver driver(scene.root, { 800, 600 });

		dialog.open(driver.ui());
		driver.click(*dialog.buttons()[1]);
		CHECK_EQUAL("Back", dialog.buttons()[1]->text());
		CHECK_EQUAL(0, accepted);
		CHECK_EQUAL(1, cancelled);
		CHECK(!dialog.isOpen());

		dialog.open(driver.ui());
		driver.click(*dialog.buttons()[0]);
		CHECK_EQUAL(1, cancelled);
	}

	TEST(ScrimIsPaintedBelowDialog) {
		DialogScene scene;
		Dialog dialog("Title");
		UiDriver driver(scene.root, { 800, 600 });
		dialog.open(driver.ui());
		driver.settle();

		RecordingPainter& painter = driver.paint();
		bool scrim = false;
		for (const PaintCommand& command : painter.commands()) {
			if (command.rect == makeRect(0, 0, 800, 600) && command.color == driver.ui().theme().palette.scrim) scrim = true;
		}
		CHECK(scrim);
	}
}

SUITE(ConfirmDialogBehaviour) {
	TEST(AskShowsTextsAndConfirms) {
		DialogScene scene;
		ConfirmDialog confirm;
		UiDriver driver(scene.root, { 800, 600 });
		bool cleared = false;

		confirm.ask(driver.ui(), "Clear history", "Are you sure you want to clear history?", "Clear history", [&cleared]() {
			cleared = true;
		});
		driver.settle();

		CHECK_EQUAL("Clear history", confirm.title());
		CHECK_EQUAL("Are you sure you want to clear history?", confirm.message());
		CHECK_EQUAL("Clear history", confirm.buttons()[0]->text());
		RecordingPainter& painter = driver.paint();
		CHECK(painter.hasText("Are you sure you want to clear history?"));

		driver.click(*confirm.buttons()[0]);
		CHECK(cleared);
	}

	TEST(CancelDoesNotConfirm) {
		DialogScene scene;
		ConfirmDialog confirm;
		UiDriver driver(scene.root, { 800, 600 });
		bool cleared = false;

		confirm.ask(
			driver.ui(), "Clear canvas", "Are you sure you would like to clear canvas?", "Clear canvas", [&cleared]() {
				cleared = true;
			}
		);
		driver.click(*confirm.buttons()[1]);
		CHECK(!cleared);
		CHECK(!confirm.isOpen());
	}
}
