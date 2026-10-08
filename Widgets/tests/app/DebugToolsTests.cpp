#include "AppDriver.hpp"
#include "Label.hpp"
#include "WidgetApplication.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;

namespace {
	class PlainApplication : public WidgetApplication {
	public:
		PlainApplication() { setRoot(content); }

		[[nodiscard]] AppStorage persistentAppStorage() const override { return AppStorage("/tmp/plain-application"); }

		Label content{ "content" };
	};

	MenuItem* itemNamed(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return item.get();
		}
		return nullptr;
	}
}

SUITE(DefaultDebugTools) {
	TEST(EveryApplicationHasADebugMenuWithRecording) {
		PlainApplication application;
		AppDriver driver(application);

		REQUIRE CHECK_EQUAL(1u, application.menuBar().titles().size());
		CHECK_EQUAL("Debug", application.menuBar().titles()[0]->text());
		Menu& debug = application.debugTools().menu();
		CHECK(itemNamed(debug, "Debug UI") != nullptr);
		REQUIRE CHECK(itemNamed(debug, "Record session") != nullptr);
		CHECK(itemNamed(debug, "Stop recording and save") != nullptr);

		itemNamed(debug, "Debug UI")->entry().action();
		CHECK(application.inspector().isVisible());
		itemNamed(debug, "Debug UI")->entry().action();
		CHECK(!application.inspector().isVisible());
	}

	TEST(RecordingCapturesRealWindowInput) {
		PlainApplication application;
		AppDriver driver(application);
		SessionRecordingControl& recording = application.debugTools().recording();

		recording.start();
		driver.frame();
		CHECK(recording.badge.isVisible());
		driver.click(application.content);
		const SessionRecording session = application.debugTools().recorder().stop();

		CHECK(session.input.events.size() >= 3);
		CHECK(!session.environment.device.empty());
		CHECK(session.recordedFrames.find("ui.dispatch") != std::string::npos);
	}

	TEST(DebugToolsCanBeTurnedOff) {
		PlainApplication application;
		application.setDebugToolsEnabled(false);
		AppDriver driver(application);

		CHECK(application.menuBar().titles().empty());
		CHECK(application.debugTools().attachedBar() == nullptr);
	}
}
