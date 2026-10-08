#include "ApplicationFrame.hpp"
#include "BoxLayout.hpp"
#include "Gallery.hpp"
#include "MediaControls.hpp"
#include "Probes.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::gallery;
using namespace tp::ui::testing;

namespace {
	constexpr size_t kMediaTab = 7;

	struct BarScene {
		Widget root;
		PlayerBar bar;
		NowPlaying track{ "Jumpsuit", "twenty one pilots", "Trench", 238, true };
		UiDriver driver{ root, { 1100, 300 } };

		BarScene() {
			root.emplaceLayout<ColumnLayout>();
			root.addChild(bar);
			driver.settle();
		}
	};
}

SUITE(DurationFormat) {
	TEST(MinutesAndHours) {
		CHECK_EQUAL("0:00", formatDuration(0));
		CHECK_EQUAL("3:58", formatDuration(238.9f));
		CHECK_EQUAL("1:02:05", formatDuration(3725));
		CHECK_EQUAL("0:00", formatDuration(-4));
	}
}

SUITE(TransportButtons) {
	TEST(GlyphsPaintAsShapesAndPrimaryIsLight) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		TransportButton play(TransportButton::Glyph::Play, true);
		TransportButton next(TransportButton::Glyph::Next);
		root.addChild(play);
		root.addChild(next);
		UiDriver driver(root);

		CHECK(play.size().x > next.size().x);
		RecordingPainter& painter = driver.paint();
		int polygons = 0;
		for (const PaintCommand& command : painter.commands())
			polygons += command.kind == PaintCommand::Kind::Polygon ? 1 : 0;
		CHECK_EQUAL(2, polygons);

		int clicks = 0;
		play.setOnActivate([&clicks]() { clicks++; });
		driver.click(play);
		CHECK_EQUAL(1, clicks);
	}
}

SUITE(SeekBarInteraction) {
	TEST(ClickSeeksAndShowsTimes) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		SeekBar seek;
		seek.setDuration(200);
		root.addChild(seek);
		UiDriver driver(root, { 600, 100 });
		halnf reported = -1;
		seek.setOnSeek([&reported](halnf position) { reported = position; });

		driver.click(seek.windowPointForPosition(0.25f));
		CHECK_CLOSE(0.25f, reported, 0.01f);
		CHECK_CLOSE(0.25f, seek.position(), 0.01f);
		CHECK(driver.paint().hasText("0:50"));
		CHECK(driver.paint().hasText("3:20"));
	}

	TEST(ScrubbingPreviewsUntilRelease) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		SeekBar seek;
		seek.setDuration(100);
		root.addChild(seek);
		UiDriver driver(root, { 600, 100 });
		int seeks = 0;
		seek.setOnSeek([&seeks](halnf) { seeks++; });

		driver.moveTo(seek.windowPointForPosition(0.1f));
		driver.press();
		driver.moveTo(seek.windowPointForPosition(0.7f));
		CHECK(seek.isScrubbing());
		CHECK_CLOSE(0.7f, seek.shownPosition(), 0.01f);
		CHECK_CLOSE(0, seek.position(), 0.001f);
		seek.setPosition(0.05f);
		CHECK_CLOSE(0.7f, seek.shownPosition(), 0.01f);
		CHECK_EQUAL(0, seeks);

		driver.release();
		CHECK_EQUAL(1, seeks);
		CHECK_CLOSE(0.7f, seek.position(), 0.01f);
	}

	TEST(ArrowKeysSkipFiveSeconds) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		SeekBar seek;
		seek.setDuration(100);
		root.addChild(seek);
		UiDriver driver(root, { 600, 100 });
		driver.click(seek.windowPointForPosition(0.5f));
		driver.key(InputID::RIGHT);
		CHECK_CLOSE(0.55f, seek.position(), 0.01f);
		driver.key(InputID::LEFT);
		driver.key(InputID::LEFT);
		CHECK_CLOSE(0.45f, seek.position(), 0.01f);
	}

	TEST(UnknownDurationShowsPlaceholders) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		SeekBar seek;
		root.addChild(seek);
		UiDriver driver(root, { 600, 100 });
		CHECK(driver.paint().hasText("-:--"));
	}
}

SUITE(VolumeControlBehaviour) {
	TEST(SliderAndMuteReportEffectiveVolume) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		VolumeControl volume;
		root.addChild(volume);
		UiDriver driver(root);
		halnf reported = -1;
		volume.setOnChanged([&reported](halnf value) { reported = value; });

		driver.setSlider(volume.slider(), 0.4f);
		CHECK_CLOSE(0.4f, reported, 0.03f);

		driver.click(volume.muteButton());
		CHECK(volume.isMuted());
		CHECK_CLOSE(0, reported, 0.001f);
		CHECK(volume.muteButton().glyph() == TransportButton::Glyph::Muted);

		driver.click(volume.muteButton());
		CHECK(!volume.isMuted());
		CHECK_CLOSE(0.4f, reported, 0.03f);
	}

	TEST(MovingSliderUnmutes) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		VolumeControl volume;
		root.addChild(volume);
		UiDriver driver(root);
		volume.setMuted(true);
		driver.setSlider(volume.slider(), 0.8f);
		CHECK(!volume.isMuted());
	}
}

SUITE(PlayerBarBehaviour) {
	TEST(EmptyBarIsInert) {
		BarScene scene;
		CHECK(!scene.bar.hasTrack());
		CHECK_EQUAL("Nothing playing", scene.bar.title.text());
		CHECK(!scene.bar.seek.isEnabled());
	}

	TEST(TrackFillsInfoAndTransportForwardsClicks) {
		BarScene scene;
		scene.bar.setTrack(&scene.track);
		CHECK_EQUAL("Jumpsuit", scene.bar.title.text());
		CHECK_EQUAL("twenty one pilots", scene.bar.artist.text());
		CHECK_CLOSE(238, scene.bar.seek.duration(), 0.01f);

		int playPause = 0;
		int next = 0;
		int previous = 0;
		scene.bar.onPlayPause = [&playPause]() { playPause++; };
		scene.bar.onNext = [&next]() { next++; };
		scene.bar.onPrevious = [&previous]() { previous++; };
		scene.driver.click(scene.bar.playPause);
		scene.driver.click(scene.bar.next);
		scene.driver.click(scene.bar.previous);
		CHECK_EQUAL(1, playPause);
		CHECK_EQUAL(1, next);
		CHECK_EQUAL(1, previous);

		scene.bar.setPlaying(true);
		CHECK(scene.bar.playPause.glyph() == TransportButton::Glyph::Pause);
		CHECK_EQUAL("Pause", scene.bar.playPause.tooltip());
	}

	TEST(LayoutKeepsTransportCenteredAndVolumeRight) {
		BarScene scene;
		scene.bar.setTrack(&scene.track);
		scene.driver.settle();
		const RectF bar = scene.bar.windowRect();
		const RectF play = scene.bar.playPause.windowRect();
		const RectF volume = scene.bar.volume.windowRect();
		CHECK_CLOSE(bar.center().x, play.center().x, 2.0f);
		CHECK(volume.pos.x + volume.size.x > bar.pos.x + bar.size.x - 20);
		CHECK(scene.bar.art.windowRect().pos.x < play.pos.x);
	}
}

SUITE(ApplicationFrameMenu) {
	TEST(FrameShowsMenuBarAboveContentAndCanHideIt) {
		ApplicationFrame frame;
		Box content(Vec2F(100.0f, 100.0f), "content");
		frame.setContent(&content);
		Menu file;
		file.addItem("Open", {});
		frame.menuBar.addMenu("File", file);
		UiDriver driver(frame, { 800, 600 });

		CHECK(frame.isMenuBarVisible());
		CHECK(content.windowRect().pos.y >= frame.menuBar.windowRect().pos.y + frame.menuBar.size().y - 0.5f);
		CHECK(driver.paint().hasText("File"));

		frame.setMenuBarVisible(false);
		driver.settle();
		CHECK_CLOSE(0, content.windowRect().pos.y, 0.5f);
		CHECK(!driver.paint().hasText("File"));
	}

	TEST(SwappingContentReplacesIt) {
		ApplicationFrame frame;
		Box first(Vec2F(100.0f, 100.0f), "first");
		Box second(Vec2F(100.0f, 100.0f), "second");
		frame.setContent(&first);
		frame.setContent(&second);
		CHECK(first.parent() == nullptr);
		CHECK(second.parent() == &frame);
		frame.setContent(nullptr);
		CHECK(second.parent() == nullptr);
	}
}

SUITE(GalleryMediaPage) {
	TEST(PlaybackAdvancesAndSkipsTracks) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		driver.click(gallery.tabs.tabButton(kMediaTab));
		MediaPage& page = gallery.mediaPage;
		CHECK_EQUAL("Media", gallery.tabs.tabButton(kMediaTab).text());
		CHECK_EQUAL("Jumpsuit", page.player.title.text());

		driver.click(page.player.playPause);
		CHECK(page.player.isPlaying());
		driver.advance(2000);
		CHECK_CLOSE(2.0f / 238.5f, page.player.seek.position(), 0.004f);

		driver.click(page.player.next);
		CHECK_EQUAL("Mystery Lady", page.player.title.text());
		driver.click(page.player.previous);
		driver.click(page.player.previous);
		CHECK_EQUAL("No Idea", page.player.title.text());

		driver.click(page.player.playPause);
		CHECK(!page.player.isPlaying());
		CHECK_EQUAL("Paused", page.status.text());
	}
}

SUITE(ApplicationFrameKeys) {
	TEST(UnfocusedKeysReachContentThroughFrame) {
		ApplicationFrame frame;
		EventProbe content(Vec2F(100.0f, 100.0f), "content");
		frame.setContent(&content);
		UiDriver driver(frame, { 800, 600 });

		driver.key(InputID::Z);
		CHECK_EQUAL(1, content.count(Event::Type::KeyDown));
	}
}
