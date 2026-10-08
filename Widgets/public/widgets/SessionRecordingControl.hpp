#pragma once

#include "Button.hpp"
#include "Dialog.hpp"
#include "FileBrowser.hpp"
#include "Menus.hpp"
#include "Notice.hpp"
#include "SessionRecording.hpp"

#include <functional>
#include <memory>
#include <string>

namespace tp::ui {

	class SessionRecordingControl;

	class RecordingBadge : public Button {
	public:
		explicit RecordingBadge(SessionRecordingControl& control);

		void refresh();

	protected:
		void onTick(halnf deltaMs) override;

	private:
		SessionRecordingControl& mControl;
	};

	class SessionRecordingControl {
	public:
		static constexpr halnf kBadgeWidth = 112;

		explicit SessionRecordingControl(SessionRecorder& recorder, FileSystem& fileSystem = FileSystem::local());

		SessionRecordingControl(const SessionRecordingControl&) = delete;
		SessionRecordingControl& operator=(const SessionRecordingControl&) = delete;

		[[nodiscard]] SessionRecorder& recorder() { return mRecorder; }
		[[nodiscard]] bool isRecording() const { return mRecorder.isRecording(); }
		[[nodiscard]] bool hasUnsavedSession() const { return mPending != nullptr; }
		[[nodiscard]] const std::string& lastSessionDirectory() const { return mLastDirectory; }
		[[nodiscard]] const std::string& lastNotice() const { return mLastNotice; }

		void setAnchor(Widget* anchor) { mAnchor = anchor; }
		void setOnBeforeStart(std::function<void()> callback) { mOnBeforeStart = std::move(callback); }
		void setOnNotice(std::function<void(const std::string&)> callback) { mOnNotice = std::move(callback); }
		void setTimestamp(std::function<std::string()> timestamp) { mTimestamp = std::move(timestamp); }

		void start();
		void stop();
		void addMenuItems(Menu& menu);

		[[nodiscard]] static std::string clockText(double elapsedMs);
		[[nodiscard]] static std::string localTimestamp();

		RecordingBadge badge{ *this };
		FileDialog saveDialog;
		Dialog discardDialog{ "Discard the recording?" };
		Notice notice;

	private:
		[[nodiscard]] Ui* ui() const;
		void askWhereToSave();
		[[nodiscard]] std::string defaultSessionsDirectory() const;
		void saveTo(const std::string& directory);
		void notify(const std::string& message);

		SessionRecorder& mRecorder;
		Widget* mAnchor = nullptr;
		std::string mFolder;
		std::string mLastDirectory;
		std::string mLastNotice;
		std::unique_ptr<SessionRecording> mPending;
		std::function<void()> mOnBeforeStart;
		std::function<void(const std::string&)> mOnNotice;
		std::function<std::string()> mTimestamp;
	};
}
