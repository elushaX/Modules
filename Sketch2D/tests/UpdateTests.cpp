#include "Updates.hpp"

#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	const std::string kSha = std::string(64, 'a');

	std::string manifest(int8 versionCode) {
		return "{ \"versionCode\": " + std::to_string(versionCode) +
					 ", \"versionName\": \"0.1.7 (abc)\", \"apk\": \"https://example/sketch2d.apk\", \"sha256\": \"" + kSha +
					 "\", \"size\": 2097152, \"changelog\": \"- Faster brushes\\n- Folders\" }";
	}

	class FakeBackend : public UpdateBackend {
	public:
		int8 installed = 5;
		std::optional<std::string> manifestText;
		std::optional<std::string> downloadError;
		std::optional<std::string> installError;
		int installs = 0;
		std::string fetchedUrl;

		int8 installedVersionCode() override { return installed; }

		std::optional<std::string> fetchText(const std::string& url) override {
			fetchedUrl = url;
			return manifestText;
		}

		std::optional<std::string>
		download(const Release& release, const std::function<void(int8, int8)>& onProgress) override {
			onProgress(release.size / 2, release.size);
			onProgress(release.size, release.size);
			return downloadError;
		}

		std::optional<std::string> install() override {
			installs++;
			return installError;
		}
	};

	struct UpdateScene {
		Widget root;
		FakeBackend backend;
		UpdateManager manager{ backend, "https://example/latest.json" };
		UiDriver driver{ root, { 1280, 800 } };

		void settle() {
			for (int attempt = 0; attempt < 200 && manager.isBusy(); attempt++)
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			manager.poll(driver.ui());
			driver.layout();
		}
	};
}

SUITE(Updates) {
	TEST(ReleaseManifestIsParsed) {
		const std::optional<Release> release = Release::parse(manifest(7));

		REQUIRE CHECK(release.has_value());
		CHECK_EQUAL(7, release->versionCode);
		CHECK_EQUAL("0.1.7 (abc)", release->versionName);
		CHECK_EQUAL("https://example/sketch2d.apk", release->apkUrl);
		CHECK_EQUAL(kSha, release->sha256);
		CHECK_EQUAL(2097152, release->size);
		CHECK_EQUAL("- Faster brushes\n- Folders", release->changelog);
	}

	TEST(BrokenManifestsAreIgnored) {
		CHECK(!Release::parse("").has_value());
		CHECK(!Release::parse("<html>not found</html>").has_value());
		CHECK(!Release::parse("{ \"versionCode\": 3 }").has_value());
		CHECK(!Release::parse("{ \"versionCode\": 3, \"apk\": \"x\", \"sha256\": \"short\" }").has_value());
	}

	TEST(NewerReleaseIsOfferedWithItsChangelog) {
		UpdateScene scene;
		scene.backend.manifestText = manifest(7);

		scene.manager.checkInBackground();
		scene.settle();

		CHECK_EQUAL("https://example/latest.json", scene.backend.fetchedUrl);
		REQUIRE CHECK(scene.manager.dialog.isOpen());
		CHECK_EQUAL("Update to build 7", scene.manager.dialog.title());
		CHECK(scene.manager.dialog.message().empty());
		REQUIRE CHECK_EQUAL(1u, scene.manager.dialog.sections().size());
		CHECK_EQUAL("Build 7", scene.manager.dialog.sections()[0]->collapsible.header().title());
		CHECK(scene.manager.dialog.sections()[0]->collapsible.isExpanded());
		CHECK_EQUAL("- Faster brushes\n- Folders", scene.manager.dialog.sections()[0]->notes.text());
	}

	TEST(LongChangelogLinesWrapInsideThePhoneDialog) {
		Widget root;
		FakeBackend backend;
		UpdateManager manager{ backend, "https://example/latest.json" };
		UiDriver driver{ root, { 360, 800 } };
		backend.manifestText = "{ \"versionCode\": 9, \"apk\": \"https://example/a.apk\", \"sha256\": \"" + kSha +
													 "\", \"changelog\": \"- Give the colour popup wheel, square, RGB, HSV and HSL pages\" }";
		manager.checkInBackground();
		for (int attempt = 0; attempt < 200 && manager.isBusy(); attempt++)
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		manager.poll(driver.ui());
		driver.layout();

		REQUIRE CHECK(manager.dialog.isOpen());
		const RectF dialog = manager.dialog.windowRect();
		REQUIRE CHECK_EQUAL(1u, manager.dialog.sections().size());
		const Label& notes = manager.dialog.sections()[0]->notes;
		const RectF changelog = notes.windowRect();
		CHECK(changelog.pos.x + changelog.size.x <= dialog.pos.x + dialog.size.x + 0.5f);
		CHECK(notes.linesFor(changelog.size.x).size() > 1);
		std::string shown;
		for (const PaintCommand& command : driver.paint().commands())
			if (
				command.kind == PaintCommand::Kind::Text && command.rect.pos.y >= changelog.pos.y - 0.5f &&
				command.rect.pos.y < changelog.pos.y + changelog.size.y
			)
				shown += (shown.empty() ? "" : " ") + command.text;
		CHECK_EQUAL("- Give the colour popup wheel, square, RGB, HSV and HSL pages", shown);
	}

	TEST(SameOrOlderVersionsAreNotOffered) {
		UpdateScene scene;
		scene.backend.installed = 7;
		scene.backend.manifestText = manifest(7);

		scene.manager.checkInBackground();
		scene.settle();

		CHECK(!scene.manager.dialog.isOpen());
		CHECK(!scene.manager.offered().has_value());
	}

	TEST(NoConnectionStaysSilent) {
		UpdateScene scene;
		scene.backend.manifestText = std::nullopt;

		scene.manager.checkInBackground();
		scene.settle();

		CHECK(!scene.manager.dialog.isOpen());
	}

	TEST(LaterClosesWithoutDownloading) {
		UpdateScene scene;
		scene.backend.manifestText = manifest(7);
		scene.manager.checkInBackground();
		scene.settle();

		scene.driver.click(*scene.manager.dialog.later);
		scene.settle();

		CHECK(!scene.manager.dialog.isOpen());
		CHECK_EQUAL(0, scene.backend.installs);
	}

	TEST(UpdateNowDownloadsThenStartsTheInstaller) {
		UpdateScene scene;
		scene.backend.manifestText = manifest(7);
		scene.manager.checkInBackground();
		scene.settle();

		scene.driver.click(*scene.manager.dialog.updateNow);
		scene.settle();

		CHECK_EQUAL(1, scene.backend.installs);
		CHECK(!scene.manager.dialog.isOpen());
	}

	TEST(DownloadFailureIsShownAndCanBeRetried) {
		UpdateScene scene;
		scene.backend.manifestText = manifest(7);
		scene.backend.downloadError = std::string("The download was damaged. Try again later.");
		scene.manager.checkInBackground();
		scene.settle();

		scene.driver.click(*scene.manager.dialog.updateNow);
		scene.settle();

		REQUIRE CHECK(scene.manager.dialog.isOpen());
		CHECK_EQUAL("The download was damaged. Try again later.", scene.manager.dialog.progress.text());
		CHECK_EQUAL(0, scene.backend.installs);
		CHECK(scene.manager.dialog.updateNow->isEnabled());

		scene.backend.downloadError = std::nullopt;
		scene.driver.click(*scene.manager.dialog.updateNow);
		scene.settle();
		CHECK_EQUAL(1, scene.backend.installs);
	}

	std::string historyManifest() {
		return "{ \"versionCode\": 9, \"versionName\": \"0.1.9 (c9)\", \"apk\": \"https://example/a.apk\", \"sha256\": \"" +
					 kSha +
					 "\", \"changelog\": \"- Nine\", \"releases\": [ "
					 "{ \"versionCode\": 9, \"versionName\": \"0.1.9 (c9)\", \"changelog\": \"- Nine\" }, "
					 "{ \"versionCode\": 8, \"versionName\": \"0.1.8 (c8)\", \"changelog\": \"- Eight {braces} [x]\" }, "
					 "{ \"versionCode\": 7, \"versionName\": \"0.1.7 (c7)\", \"changelog\": \"- Seven\" }, "
					 "{ \"versionCode\": 5, \"versionName\": \"0.1.5 (c5)\", \"changelog\": \"- Five\" } ] }";
	}

	TEST(ManifestHistoryIsParsed) {
		const std::optional<Release> release = Release::parse(historyManifest());
		REQUIRE CHECK(release.has_value());
		REQUIRE CHECK_EQUAL(4u, release->history.size());
		CHECK_EQUAL(8, release->history[1].versionCode);
		CHECK_EQUAL("0.1.8 (c8)", release->history[1].versionName);
		CHECK_EQUAL("- Eight {braces} [x]", release->history[1].changelog);
	}

	TEST(EveryMissedBuildGetsItsOwnCollapsibleSection) {
		UpdateScene scene;
		scene.backend.installed = 6;
		scene.backend.manifestText = historyManifest();
		scene.manager.checkInBackground();
		scene.settle();

		REQUIRE CHECK(scene.manager.dialog.isOpen());
		const auto& sections = scene.manager.dialog.sections();
		REQUIRE CHECK_EQUAL(3u, sections.size());
		CHECK_EQUAL("Build 9", sections[0]->collapsible.header().title());
		CHECK_EQUAL("Build 8", sections[1]->collapsible.header().title());
		CHECK_EQUAL("Build 7", sections[2]->collapsible.header().title());
		CHECK_EQUAL("0.1.8 (c8)", sections[1]->collapsible.header().summary());
		CHECK_EQUAL("- Seven", sections[2]->notes.text());
		CHECK(sections[0]->collapsible.isExpanded());
		CHECK(!sections[1]->collapsible.isExpanded());
		CHECK(!sections[2]->collapsible.isExpanded());

		scene.driver.click(sections[2]->collapsible.header());
		CHECK(sections[2]->collapsible.isExpanded());
		CHECK(sections[2]->collapsible.isBodyShown());
	}

	TEST(OldManifestsWithoutHistoryStillShowTheLatestNotes) {
		UpdateScene scene;
		scene.backend.installed = 1;
		scene.backend.manifestText = manifest(7);
		scene.manager.checkInBackground();
		scene.settle();
		REQUIRE CHECK_EQUAL(1u, scene.manager.dialog.sections().size());
		CHECK_EQUAL("- Faster brushes\n- Folders", scene.manager.dialog.sections()[0]->notes.text());
	}

	TEST(ReopeningShowsOnlyTheCurrentSections) {
		UpdateDialog dialog;
		Widget root;
		UiDriver driver{ root, { 1280, 800 } };
		const Release release = *Release::parse(historyManifest());
		dialog.showRelease(driver.ui(), release, 4, {});
		CHECK_EQUAL(4u, dialog.sections().size());
		dialog.showRelease(driver.ui(), release, 8, {});
		CHECK_EQUAL(1u, dialog.sections().size());
	}

	TEST(ALongHistoryScrollsInsideTheDialog) {
		std::string releases;
		for (int4 code = 40; code > 0; code--)
			releases += std::string(releases.empty() ? "" : ", ") + "{ \"versionCode\": " + std::to_string(code) +
									", \"changelog\": \"- Change " + std::to_string(code) + "\" }";
		const std::string text = "{ \"versionCode\": 40, \"apk\": \"https://example/a.apk\", \"sha256\": \"" + kSha +
														 "\", \"releases\": [ " + releases + " ] }";
		Widget root;
		UiDriver driver{ root, { 360, 800 } };
		UpdateDialog dialog;
		dialog.showRelease(driver.ui(), *Release::parse(text), 0, {});
		driver.layout();
		CHECK_EQUAL(UpdateDialog::kMaxSections, dialog.sections().size());
		CHECK(dialog.notesScroll.size().y <= UpdateDialog::kMaxNotesHeight + 0.5f);
		CHECK(dialog.windowRect().size.y < 800);
	}

	TEST(TabletDialogListsMissedBuildsInsideTheScreen) {
		Widget root;
		UiDriver driver{ root, { 1200, 1920 } };
		UpdateDialog dialog;
		dialog.showRelease(driver.ui(), *Release::parse(historyManifest()), 6, {});
		driver.layout();
		REQUIRE CHECK_EQUAL(3u, dialog.sections().size());
		const RectF window = dialog.windowRect();
		CHECK(window.pos.y >= 0 && window.pos.y + window.size.y <= 1920);
		for (const auto& section : dialog.sections()) {
			const RectF header = section->collapsible.header().windowRect();
			CHECK(header.pos.x >= window.pos.x - 0.5f && header.pos.x + header.size.x <= window.pos.x + window.size.x + 0.5f);
		}
	}
}
