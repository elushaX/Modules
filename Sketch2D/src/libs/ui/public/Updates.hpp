#pragma once

#include "Collapsible.hpp"
#include "Dialog.hpp"
#include "Label.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace tp::sketch2d {

	struct ReleaseNotes {
		int8 versionCode = 0;
		std::string versionName;
		std::string changelog;
	};

	struct Release {
		int8 versionCode = 0;
		std::string versionName;
		std::string apkUrl;
		std::string sha256;
		int8 size = 0;
		std::string changelog;
		std::vector<ReleaseNotes> history;

		[[nodiscard]] static std::optional<Release> parse(const std::string& json);
		[[nodiscard]] std::vector<ReleaseNotes> notesSince(int8 installedVersionCode) const;
	};

	class UpdateBackend {
	public:
		virtual ~UpdateBackend() = default;

		[[nodiscard]] virtual int8 installedVersionCode() = 0;
		[[nodiscard]] virtual std::optional<std::string> fetchText(const std::string& url) = 0;
		[[nodiscard]] virtual std::optional<std::string>
		download(const Release& release, const std::function<void(int8 received, int8 total)>& onProgress) = 0;
		[[nodiscard]] virtual std::optional<std::string> install() = 0;
		virtual void wakeMainThread() {}
	};

	class UpdateDialog : public ui::Dialog {
	public:
		UpdateDialog();

		static constexpr size_t kMaxSections = 30;
		static constexpr halnf kMaxNotesHeight = 360;

		void showRelease(ui::Ui& ui, const Release& release, int8 installedVersionCode, std::function<void()> onUpdate);
		void showProgress(int8 received, int8 total);
		void showError(const std::string& message);

		struct Section {
			ui::Collapsible collapsible;
			ui::Label notes;
			explicit Section(std::string title) :
				collapsible(std::move(title)) {}
		};

		[[nodiscard]] const std::vector<std::unique_ptr<Section>>& sections() const { return mSections; }

		ui::ScrollView notesScroll{ ui::Axis::Vertical };
		ui::Widget notesColumn;
		ui::Label progress;
		ui::Button* updateNow = nullptr;
		ui::Button* later = nullptr;

	private:
		std::vector<std::unique_ptr<Section>> mSections;
		std::function<void()> mOnUpdate;
	};

	class UpdateManager {
	public:
		static constexpr const char* kManifestUrl = "https://ilusha-basic.online/sketch2d/latest.json";

		UpdateManager(UpdateBackend& backend, std::string manifestUrl = kManifestUrl);
		~UpdateManager();

		void checkInBackground();
		void poll(ui::Ui& ui);
		[[nodiscard]] bool isBusy() const { return mBusy; }
		[[nodiscard]] const std::optional<Release>& offered() const { return mOffered; }

		UpdateDialog dialog;

	private:
		enum class Stage { Idle, Checking, Offered, Downloading, Downloaded, Failed };

		void startDownload();
		void join();

		UpdateBackend& mBackend;
		std::string mManifestUrl;
		std::thread mWorker;
		std::mutex mMutex;
		std::atomic<bool> mBusy = false;
		Stage mStage = Stage::Idle;
		std::optional<Release> mFound;
		std::optional<Release> mOffered;
		int8 mInstalled = 0;
		std::optional<std::string> mError;
		int8 mReceived = 0;
		int8 mTotal = 0;
		bool mProgressChanged = false;
		bool mShowProgress = false;
	};
}
