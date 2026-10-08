#include "Updates.hpp"
#include "ThreadPlacement.hpp"

#include "BoxLayout.hpp"
#include "Ui.hpp"

#include <algorithm>

#include <cctype>
#include <cstdio>

namespace tp::sketch2d {

	using namespace tp::ui;

	namespace {
		class JsonReader {
		public:
			explicit JsonReader(const std::string& text) :
				mText(text) {}

			[[nodiscard]] std::optional<std::string> string(const std::string& key) const {
				const size_t value = valueStart(key);
				if (value == std::string::npos || mText[value] != '"') return std::nullopt;

				std::string result;
				for (size_t index = value + 1; index < mText.size(); index++) {
					const char character = mText[index];
					if (character == '"') return result;
					if (character != '\\') {
						result += character;
						continue;
					}
					if (++index >= mText.size()) return std::nullopt;
					const char escaped = mText[index];
					if (escaped == 'n') result += '\n';
					else if (escaped == 't') result += '\t';
					else if (escaped == 'u') {
						if (index + 4 >= mText.size()) return std::nullopt;
						appendCodepoint(result, (uint4) std::stoul(mText.substr(index + 1, 4), nullptr, 16));
						index += 4;
					} else result += escaped;
				}
				return std::nullopt;
			}

			[[nodiscard]] std::optional<int8> integer(const std::string& key) const {
				const size_t value = valueStart(key);
				if (value == std::string::npos) return std::nullopt;
				size_t end = value;
				while (end < mText.size() && (std::isdigit((unsigned char) mText[end]) || mText[end] == '-'))
					end++;
				if (end == value) return std::nullopt;
				return (int8) std::stoll(mText.substr(value, end - value));
			}

		private:
			[[nodiscard]] size_t valueStart(const std::string& key) const {
				const size_t found = mText.find("\"" + key + "\"");
				if (found == std::string::npos) return std::string::npos;
				size_t index = mText.find(':', found + key.size() + 2);
				if (index == std::string::npos) return std::string::npos;
				index++;
				while (index < mText.size() && std::isspace((unsigned char) mText[index]))
					index++;
				return index < mText.size() ? index : std::string::npos;
			}

			static void appendCodepoint(std::string& out, uint4 codepoint) {
				if (codepoint < 0x80) {
					out += (char) codepoint;
				} else if (codepoint < 0x800) {
					out += (char) (0xC0 | (codepoint >> 6));
					out += (char) (0x80 | (codepoint & 0x3F));
				} else {
					out += (char) (0xE0 | (codepoint >> 12));
					out += (char) (0x80 | ((codepoint >> 6) & 0x3F));
					out += (char) (0x80 | (codepoint & 0x3F));
				}
			}

			const std::string& mText;
		};

		std::vector<std::string> objectsIn(const std::string& text, const std::string& key) {
			std::vector<std::string> objects;
			const size_t found = text.find("\"" + key + "\"");
			if (found == std::string::npos) return objects;
			size_t index = text.find('[', found);
			if (index == std::string::npos) return objects;
			int4 depth = 0;
			size_t start = 0;
			bool inString = false;
			for (index++; index < text.size(); index++) {
				const char character = text[index];
				if (inString) {
					if (character == '\\') index++;
					else if (character == '"') inString = false;
					continue;
				}
				if (character == '"') inString = true;
				else if (character == '{') {
					if (depth++ == 0) start = index;
				} else if (character == '}') {
					if (--depth == 0) objects.push_back(text.substr(start, index - start + 1));
				} else if (character == ']' && depth == 0) break;
			}
			return objects;
		}

		std::string megabytes(int8 bytes) {
			char text[32];
			std::snprintf(text, sizeof(text), "%.1f MB", (double) bytes / (1024.0 * 1024.0));
			return text;
		}
	}

	std::optional<Release> Release::parse(const std::string& json) {
		const JsonReader reader(json);
		const std::optional<int8> versionCode = reader.integer("versionCode");
		const std::optional<std::string> apkUrl = reader.string("apk");
		const std::optional<std::string> sha256 = reader.string("sha256");
		if (!versionCode || !apkUrl || !sha256 || sha256->size() != 64) return std::nullopt;

		Release release;
		release.versionCode = *versionCode;
		release.versionName = reader.string("versionName").value_or(std::to_string(*versionCode));
		release.apkUrl = *apkUrl;
		release.sha256 = *sha256;
		release.size = reader.integer("size").value_or(0);
		release.changelog = reader.string("changelog").value_or("");
		for (const std::string& object : objectsIn(json, "releases")) {
			const JsonReader entry(object);
			const std::optional<int8> code = entry.integer("versionCode");
			if (!code) continue;
			release.history.push_back(
				{ *code, entry.string("versionName").value_or(std::to_string(*code)), entry.string("changelog").value_or("") }
			);
		}
		return release;
	}

	std::vector<ReleaseNotes> Release::notesSince(int8 installedVersionCode) const {
		std::vector<ReleaseNotes> notes;
		bool hasLatest = false;
		for (const ReleaseNotes& entry : history) {
			if (entry.versionCode <= installedVersionCode || entry.versionCode > versionCode) continue;
			hasLatest = hasLatest || entry.versionCode == versionCode;
			notes.push_back(entry);
		}
		if (!hasLatest) notes.push_back({ versionCode, versionName, changelog });
		std::sort(notes.begin(), notes.end(), [](const ReleaseNotes& a, const ReleaseNotes& b) {
			return a.versionCode > b.versionCode;
		});
		notes.erase(
			std::unique(
				notes.begin(),
				notes.end(),
				[](const ReleaseNotes& a, const ReleaseNotes& b) { return a.versionCode == b.versionCode; }
			),
			notes.end()
		);
		return notes;
	}

	UpdateDialog::UpdateDialog() :
		Dialog("Update available") {
		setDebugName("update dialog");
		notesColumn.emplaceLayout<ColumnLayout>(4);
		notesColumn.setSizing(AxisSizing::fill(), AxisSizing::fit());
		notesScroll.setSizing(AxisSizing::fill(), AxisSizing::fit().atMost(kMaxNotesHeight));
		notesScroll.setContent(&notesColumn);
		progress.setSizing(AxisSizing::fill(), AxisSizing::fit());
		progress.setWrap(true);
		progress.setTone(Label::Tone::Muted);
		progress.setVisible(false);
		body().addChild(notesScroll);
		body().addChild(progress);
		updateNow = &addButton(
			"Update now",
			[this]() {
				if (mOnUpdate) mOnUpdate();
			},
			true
		);
		later = &addButton("Later", {});
	}

	void
	UpdateDialog::showRelease(Ui& ui, const Release& release, int8 installedVersionCode, std::function<void()> onUpdate) {
		mOnUpdate = std::move(onUpdate);
		setTitle("Update to build " + std::to_string(release.versionCode));
		notesColumn.removeAllChildren();
		mSections.clear();
		std::vector<ReleaseNotes> notes = release.notesSince(installedVersionCode);
		if (notes.size() > kMaxSections) notes.resize(kMaxSections);
		for (const ReleaseNotes& entry : notes) {
			auto section = std::make_unique<Section>("Build " + std::to_string(entry.versionCode));
			section->notes.setText(entry.changelog.empty() ? "No changelog." : entry.changelog);
			section->notes.setWrap(true);
			section->notes.setSizing(AxisSizing::fill(), AxisSizing::fit());
			section->collapsible.header().setSummary(entry.versionName);
			section->collapsible.body().addChild(section->notes);
			section->collapsible.setExpanded(mSections.empty(), false);
			notesColumn.addChild(section->collapsible);
			mSections.push_back(std::move(section));
		}
		progress.setVisible(false);
		updateNow->setVisible(true);
		updateNow->setEnabled(true);
		later->setText("Later");
		if (!isOpen()) open(ui);
	}

	void UpdateDialog::showProgress(int8 received, int8 total) {
		progress.setVisible(true);
		updateNow->setEnabled(false);
		later->setText("Hide");
		const std::string amount = total > 0 ? megabytes(received) + " of " + megabytes(total) : megabytes(received);
		progress.setText("Downloading " + amount);
	}

	void UpdateDialog::showError(const std::string& message) {
		progress.setVisible(true);
		progress.setText(message);
		updateNow->setEnabled(true);
		updateNow->setText("Try again");
		later->setText("Later");
	}

	UpdateManager::UpdateManager(UpdateBackend& backend, std::string manifestUrl) :
		mBackend(backend),
		mManifestUrl(std::move(manifestUrl)) {}

	UpdateManager::~UpdateManager() { join(); }

	void UpdateManager::join() {
		if (mWorker.joinable()) mWorker.join();
	}

	void UpdateManager::checkInBackground() {
		if (mBusy) return;
		join();
		mBusy = true;
		mStage = Stage::Checking;
		mWorker = std::thread([this]() {
			ThreadPlacement::placeCallingThreadAsWorker();
			std::optional<Release> release;
			if (std::optional<std::string> text = mBackend.fetchText(mManifestUrl)) release = Release::parse(*text);
			const int8 installed = mBackend.installedVersionCode();
			const bool newer = release && release->versionCode > installed;
			{
				std::lock_guard lock(mMutex);
				mInstalled = installed;
				mFound = newer ? release : std::nullopt;
				mBusy = false;
			}
			mBackend.wakeMainThread();
		});
	}

	void UpdateManager::startDownload() {
		if (mBusy || !mOffered) return;
		join();
		mBusy = true;
		mStage = Stage::Downloading;
		mShowProgress = true;
		mReceived = 0;
		mTotal = mOffered->size;
		mProgressChanged = true;
		const Release release = *mOffered;
		mWorker = std::thread([this, release]() {
			ThreadPlacement::placeCallingThreadAsWorker();
			std::optional<std::string> error = mBackend.download(release, [this](int8 received, int8 total) {
				{
					std::lock_guard lock(mMutex);
					mReceived = received;
					mTotal = total;
					mProgressChanged = true;
				}
				mBackend.wakeMainThread();
			});
			{
				std::lock_guard lock(mMutex);
				mError = std::move(error);
				mBusy = false;
			}
			mBackend.wakeMainThread();
		});
	}

	void UpdateManager::poll(Ui& ui) {
		std::lock_guard lock(mMutex);
		if (mStage == Stage::Checking && !mBusy) {
			mStage = mFound ? Stage::Offered : Stage::Idle;
			mOffered = mFound;
			if (mOffered) dialog.showRelease(ui, *mOffered, mInstalled, [this]() { startDownload(); });
			return;
		}
		if (mStage != Stage::Downloading) return;

		if (mShowProgress) {
			mShowProgress = false;
			if (!dialog.isOpen()) dialog.open(ui);
		}
		if (mProgressChanged) {
			dialog.showProgress(mReceived, mTotal);
			mProgressChanged = false;
		}
		if (mBusy) return;

		if (mError) {
			mStage = Stage::Failed;
			if (!dialog.isOpen()) dialog.open(ui);
			dialog.showError(*mError);
			return;
		}
		mStage = Stage::Downloaded;
		dialog.close();
		if (std::optional<std::string> error = mBackend.install()) {
			mStage = Stage::Failed;
			dialog.open(ui);
			dialog.showError(*error);
		}
	}
}
