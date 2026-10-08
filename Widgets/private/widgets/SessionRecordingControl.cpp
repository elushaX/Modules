#include "SessionRecordingControl.hpp"

#include "WindowHost.hpp"

#include "Ui.hpp"

#include <cstdio>
#include <ctime>

using namespace tp;
using namespace tp::ui;

namespace {
	std::string seconds(double milliseconds) {
		char text[32];
		std::snprintf(text, sizeof(text), "%.1f s", milliseconds / 1000.0);
		return text;
	}
}

RecordingBadge::RecordingBadge(SessionRecordingControl& control) :
	mControl(control) {
	setDebugName("recording badge");
	setTooltip("Recording session. Click to stop and save");
	setStyle(Style::Accent);
	setSizing(AxisSizing::fixed(SessionRecordingControl::kBadgeWidth), AxisSizing::fill());
	setVisible(false);
	setOnActivate([this]() { mControl.stop(); });
}

void RecordingBadge::refresh() {
	const bool recording = mControl.isRecording();
	if (recording != isVisible()) setVisible(recording);
	if (!recording) {
		stopTicking();
		return;
	}
	const std::string clock = SessionRecordingControl::clockText(mControl.recorder().elapsedMs());
	if (text() != clock) setText(clock);
	startTicking();
}

void RecordingBadge::onTick(halnf) { refresh(); }

SessionRecordingControl::SessionRecordingControl(SessionRecorder& recorder, FileSystem& fileSystem) :
	saveDialog(fileSystem),
	mRecorder(recorder) {
	discardDialog.setDebugName("discard session dialog");
	discardDialog.setMessage("The recorded session has not been saved.");
	discardDialog.addButton(
		"Discard",
		[this]() {
			mPending.reset();
			notify("Recording discarded");
		},
		true
	);
	discardDialog.addButton("Back to saving", [this]() { askWhereToSave(); });
}

std::string SessionRecordingControl::clockText(double elapsedMs) {
	const int total = (int) (elapsedMs / 1000.0);
	char text[32];
	std::snprintf(text, sizeof(text), "\u25CF REC %d:%02d", total / 60, total % 60);
	return text;
}

std::string SessionRecordingControl::localTimestamp() {
	const std::time_t now = std::time(nullptr);
	char text[64];
	std::strftime(text, sizeof(text), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
	return text;
}

Ui* SessionRecordingControl::ui() const {
	if (mAnchor && mAnchor->ui()) return mAnchor->ui();
	return badge.ui();
}

void SessionRecordingControl::start() {
	if (mRecorder.isRecording()) return;
	if (mOnBeforeStart) mOnBeforeStart();
	mRecorder.start();
	badge.refresh();
	notify("Recording session");
}

void SessionRecordingControl::stop() {
	if (!mRecorder.isRecording()) return;
	mPending = std::make_unique<SessionRecording>(mRecorder.stop());
	badge.refresh();
	askWhereToSave();
}

void SessionRecordingControl::addMenuItems(Menu& menu) {
	MenuEntry record = MenuEntry::item("Record session", [this]() { start(); });
	record.isEnabled = [this]() { return !mRecorder.isRecording(); };
	menu.add(record);
	MenuEntry finish = MenuEntry::item("Stop recording and save", [this]() { stop(); });
	finish.isEnabled = [this]() { return mRecorder.isRecording(); };
	menu.add(finish);
}

std::string SessionRecordingControl::defaultSessionsDirectory() const {
	Ui* target = ui();
	WindowHost* host = target ? target->windowHost() : nullptr;
	if (host) return host->persistentAppStorage().sessionsDirectory();
	const std::string name = mPending->environment.application.empty() ? "modules" : mPending->environment.application;
	return AppStorage::inHome(name).sessionsDirectory();
}

void SessionRecordingControl::askWhereToSave() {
	Ui* target = ui();
	if (!mPending || !target) return;

	const std::string stamp = mTimestamp ? mTimestamp() : localTimestamp();
	const std::string folder = mFolder.empty() ? SessionRecorder::defaultDirectory(defaultSessionsDirectory()) : mFolder;
	if (!saveDialog.browser.fileSystem().isDirectory(folder)) saveDialog.browser.fileSystem().createDirectory(folder);
	saveDialog.setMessage("Recorded " + seconds(mPending->input.durationMs()) + ". The session is saved as a folder.");
	saveDialog.setOnCancel([this]() {
		if (mPending && ui()) discardDialog.open(*ui());
	});
	saveDialog.show(*target, FileDialog::Mode::Save, "Save session", folder, stamp, [this](const std::string& path) {
		saveTo(path);
	});
}

void SessionRecordingControl::saveTo(const std::string& directory) {
	if (!mPending) return;
	if (!mPending->save(directory)) {
		notify("Could not save the session to " + directory);
		askWhereToSave();
		return;
	}
	const std::string duration = seconds(mPending->input.durationMs());
	mPending.reset();
	mLastDirectory = directory;
	mFolder = parentPath(directory);
	notify("Saved " + duration + " session to " + directory);
}

void SessionRecordingControl::notify(const std::string& message) {
	mLastNotice = message;
	if (mOnNotice) {
		mOnNotice(message);
		return;
	}
	Widget* anchor = mAnchor ? mAnchor : (ui() ? ui()->root() : nullptr);
	if (anchor) notice.show(*anchor, message);
}
