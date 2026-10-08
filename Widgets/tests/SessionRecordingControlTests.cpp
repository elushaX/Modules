#include "BoxLayout.hpp"
#include "Menus.hpp"
#include "SessionRecordingControl.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct FakeRecorderHost : SessionRecorderHost {
		WindowInputObserver* tap = nullptr;
		double clockMs = 1000;

		void setRawInputTap(WindowInputObserver* observer) override { tap = observer; }
		[[nodiscard]] SessionEnvironment environment() const override {
			SessionEnvironment environment;
			environment.application = "demo";
			environment.device = "fake gpu";
			environment.windowSize = { 800, 600 };
			return environment;
		}
		[[nodiscard]] double nowMs() const override { return clockMs; }
	};

	MenuItem& itemNamed(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return *item;
		}
		throw std::runtime_error("menu item not found: " + text);
	}

	std::string readText(const std::filesystem::path& path) {
		std::ifstream file(path);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	struct RecordingScene {
		FakeRecorderHost host;
		SessionRecorder recorder{ host };
		SessionRecordingControl control{ recorder };
		Widget root;
		MenuBar bar;
		Menu debug;
		std::filesystem::path directory = std::filesystem::temp_directory_path() / "widgets-session-tests";

		RecordingScene() {
			std::filesystem::remove_all(directory);
			setenv("MODULES_SESSION_DIR", directory.c_str(), 1);
			root.emplaceLayout<ColumnLayout>();
			root.setSizing(AxisSizing::fill(), AxisSizing::fill());
			control.addMenuItems(debug);
			control.setTimestamp([]() { return std::string("take-1"); });
			bar.addTrailingMenu("Debug", debug);
			bar.setTrailingWidget(&control.badge);
			root.addChild(bar);
		}

		~RecordingScene() {
			unsetenv("MODULES_SESSION_DIR");
			std::filesystem::remove_all(directory);
		}
	};
}

SUITE(SessionRecordingControl) {
	TEST(RecordingFromTheMenuShowsATickingBadgeAndSavesAFolder) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1000, 600 });
		CHECK(!scene.control.badge.isVisible());
		CHECK(!itemNamed(scene.debug, "Stop recording and save").entry().isEnabled());

		itemNamed(scene.debug, "Record session").entry().action();
		REQUIRE CHECK(scene.control.isRecording());
		CHECK(scene.host.tap != nullptr);
		CHECK(!itemNamed(scene.debug, "Record session").entry().isEnabled());
		driver.settle();
		CHECK(scene.control.badge.isVisible());
		CHECK(scene.control.badge.text().ends_with("REC 0:00"));

		scene.host.tap->onPointerMoved({ 10, 10 });
		scene.host.tap->onPointerButton(InputID::MOUSE1, true);
		scene.host.clockMs += 65000;
		scene.host.tap->onPointerButton(InputID::MOUSE1, false);
		driver.advance(32);
		CHECK(scene.control.badge.text().ends_with("REC 1:05"));

		driver.click(scene.control.badge);
		driver.settle();
		CHECK(!scene.control.isRecording());
		CHECK(scene.host.tap == nullptr);
		CHECK(!scene.control.badge.isVisible());
		REQUIRE CHECK(scene.control.saveDialog.isOpen());
		CHECK_EQUAL(scene.directory.string(), scene.control.saveDialog.browser.directory());
		CHECK_EQUAL("take-1", scene.control.saveDialog.name.text());
		CHECK(scene.control.saveDialog.message().find("65.0 s") != std::string::npos);

		driver.key(InputID::ENTER);
		const std::filesystem::path saved = scene.directory / "take-1";
		CHECK_EQUAL(saved.string(), scene.control.lastSessionDirectory());
		CHECK(!scene.control.hasUnsavedSession());
		CHECK(readText(saved / "environment.txt").find("device fake gpu") != std::string::npos);
		const std::optional<InputRecording> input = InputRecording::load((saved / "input.txt").string());
		REQUIRE CHECK(input.has_value());
		CHECK_EQUAL(3u, input->events.size());
		CHECK(readText(saved / "recorded-frames.tsv").rfind("frame\twall_ms\tcpu_ms", 0) == 0);
	}

	TEST(ASnapshotHookAddsApplicationState) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1000, 600 });
		std::string capturedFor;
		scene.recorder.setSnapshot([&](const SessionEnvironment& environment) -> SessionRecording::Saver {
			capturedFor = environment.application;
			return [](const std::string& directory) {
				std::ofstream(std::filesystem::path(directory) / "state.txt") << "state";
				return true;
			};
		});

		scene.control.start();
		CHECK_EQUAL("demo", capturedFor);
		scene.control.stop();
		driver.settle();
		driver.key(InputID::ENTER);

		CHECK_EQUAL("state", readText(scene.directory / "take-1" / "state.txt"));
	}

	TEST(CancellingTheSaveAsksBeforeDiscarding) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1000, 600 });
		scene.control.start();
		scene.control.stop();
		driver.settle();

		driver.click(*scene.control.saveDialog.buttons().back());
		driver.settle();
		REQUIRE CHECK(scene.control.discardDialog.isOpen());
		CHECK(scene.control.hasUnsavedSession());

		driver.click(*scene.control.discardDialog.buttons().front());
		CHECK(!scene.control.hasUnsavedSession());
		CHECK_EQUAL("Recording discarded", scene.control.lastNotice());
		CHECK(!std::filesystem::exists(scene.directory / "take-1"));
	}
}

SUITE(MenuBarTrailingMenus) {
	TEST(TrailingMenusStayAfterMenusAddedLater) {
		MenuBar bar;
		Menu debug;
		Menu file;
		Menu view;
		bar.addTrailingMenu("Debug", debug);
		bar.addMenu("File", file);
		bar.addMenu("View", view);

		REQUIRE CHECK_EQUAL(3u, bar.titles().size());
		CHECK_EQUAL("File", bar.titles()[0]->text());
		CHECK_EQUAL("View", bar.titles()[1]->text());
		CHECK_EQUAL("Debug", bar.titles()[2]->text());

		bar.removeMenu(debug);
		Menu help;
		bar.addMenu("Help", help);
		REQUIRE CHECK_EQUAL(3u, bar.titles().size());
		CHECK_EQUAL("Help", bar.titles()[2]->text());
	}

	TEST(TrailingWidgetSitsAtTheRightBeforeTheWindowControls) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		root.setSizing(AxisSizing::fill(), AxisSizing::fill());
		MenuBar bar;
		Menu file;
		Button badge{ "badge" };
		badge.setSizing(AxisSizing::fixed(80), AxisSizing::fill());
		bar.addMenu("File", file);
		bar.setTrailingWidget(&badge);
		root.addChild(bar);
		UiDriver driver(root, { 800, 400 });

		CHECK(badge.windowRect().pos.x > bar.titles()[0]->windowRect().pos.x + 100);
		CHECK(badge.windowRect().pos.x + badge.windowRect().size.x > bar.windowRect().size.x - 20);
		CHECK(bar.children()[bar.children().size() - 2] == &badge);

		bar.setTrailingWidget(nullptr);
		CHECK(badge.parent() == nullptr);
	}
}
