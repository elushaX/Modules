#pragma once

#include "DebugInspector.hpp"
#include "Menus.hpp"
#include "SessionRecordingControl.hpp"

namespace tp::ui {

	class DebugTools {
	public:
		DebugTools(SessionRecorderHost& host, DebugInspector& inspector, Widget& anchor);
		~DebugTools();

		DebugTools(const DebugTools&) = delete;
		DebugTools& operator=(const DebugTools&) = delete;

		void attachTo(MenuBar& bar);
		void detach();
		[[nodiscard]] MenuBar* attachedBar() const { return mBar; }

		void addMenuItems(Menu& menu);

		[[nodiscard]] Menu& menu() { return mMenu; }
		[[nodiscard]] SessionRecorder& recorder() { return mRecorder; }
		[[nodiscard]] SessionRecordingControl& recording() { return mRecording; }

	private:
		DebugInspector& mInspector;
		SessionRecorder mRecorder;
		SessionRecordingControl mRecording{ mRecorder };
		Menu mMenu;
		MenuBar* mBar = nullptr;
	};
}
