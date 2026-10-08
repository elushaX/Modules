#include "InputCoalescer.hpp"
#include "InputRecording.hpp"

#include "UnitTest++/UnitTest++.h"

#include <vector>

using namespace tp;

namespace {
	struct ManualClock {
		double now = 0;
		InputRecorder::Clock clock() {
			return [this]() { return now; };
		}
	};

	struct Collector : WindowInputObserver {
		std::vector<std::string> calls;
		void onPointerSample(const PointerSample& sample) override {
			calls.push_back("sample " + std::to_string(sample.pressure));
		}
		void onPointerMoved(Vec2F position) override {
			calls.push_back("move " + std::to_string((int) position.x) + " " + std::to_string((int) position.y));
		}
		void onPointerButton(InputID, bool pressed) override { calls.push_back(pressed ? "down" : "up"); }
		void onKey(InputID, bool pressed) override { calls.push_back(pressed ? "key down" : "key up"); }
	};

	InputRecording penStroke() {
		ManualClock time;
		InputRecorder recorder(time.clock());
		recorder.setWindowSize({ 1600, 1000 });
		PointerSample pen;
		pen.tool = PointerTool::Pen;
		pen.pressure = 0.25f;
		pen.tilt = { 0.1f, -0.2f };
		pen.timeMs = 77;

		time.now = 1000;
		recorder.onKey(InputID::LEFT_SHIFT, true);
		time.now = 1010;
		recorder.onPointerSample(pen);
		recorder.onPointerMoved({ 10, 20 });
		recorder.onPointerButton(InputID::MOUSE1, true);
		time.now = 1012.5;
		recorder.onPointerMoved({ 30, 40 });
		time.now = 1030;
		recorder.onPointerButton(InputID::MOUSE1, false);
		time.now = 1200;
		recorder.onKey(InputID::LEFT_SHIFT, false);
		return recorder.recording();
	}
}

SUITE(InputRecording) {
	TEST(RecorderTimesEventsFromTheFirstOne) {
		const InputRecording recording = penStroke();
		REQUIRE CHECK_EQUAL(7u, recording.events.size());
		CHECK_EQUAL(0.0, recording.events[0].timeMs);
		CHECK_EQUAL(12.5, recording.events[4].timeMs);
		CHECK_EQUAL(200.0, recording.durationMs());
	}

	TEST(ExplicitClockStartTimesTheFirstEventFromThere) {
		ManualClock time;
		InputRecorder recorder(time.clock());
		time.now = 500;
		recorder.startClockNow();
		time.now = 740;
		recorder.onPointerMoved({ 1, 1 });
		CHECK_EQUAL(240.0, recorder.recording().events.front().timeMs);
	}

	TEST(SerializedRecordingParsesBackIdentically) {
		const InputRecording recording = penStroke();
		const std::optional<InputRecording> parsed = InputRecording::parse(recording.serialize());
		REQUIRE CHECK(parsed.has_value());
		CHECK_EQUAL(1600.0f, parsed->windowSize.x);
		REQUIRE CHECK_EQUAL(recording.events.size(), parsed->events.size());
		for (size_t index = 0; index < recording.events.size(); index++)
			CHECK(recording.events[index] == parsed->events[index]);
	}

	TEST(ParseRejectsForeignText) {
		CHECK(!InputRecording::parse("hello\n").has_value());
		CHECK(!InputRecording::parse(std::string(InputRecording::kHeader) + "\n1.0 teleport 3\n").has_value());
	}

	TEST(ReplayDeliversEventsInOrder) {
		Collector collector;
		for (const RecordedInput& event : penStroke().events)
			event.deliverTo(collector);
		const std::vector<std::string> expected = { "key down", "sample 0.250000", "move 10 20", "down", "move 30 40",
																								"up",       "key up" };
		CHECK(expected == collector.calls);
	}

	TEST(SliceKeepsThePointerActivityAndRebasesTime) {
		const InputRecording sliced = penStroke().slicedToPointerActivity();
		REQUIRE CHECK_EQUAL(5u, sliced.events.size());
		CHECK(sliced.events.front().kind == RecordedInput::Kind::Sample);
		CHECK_EQUAL(0.0, sliced.events.front().timeMs);
		CHECK(sliced.events.back().kind == RecordedInput::Kind::Button);
		CHECK_EQUAL(20.0, sliced.events.back().timeMs);
	}

	TEST(TranslateMovesOnlyPointerPositions) {
		const InputRecording moved = penStroke().translated({ 5, -5 });
		CHECK_EQUAL(15.0f, moved.events[2].vector.x);
		CHECK_EQUAL(15.0f, moved.events[2].vector.y);
		CHECK_EQUAL(0.25f, moved.events[1].sample.pressure);
	}

	TEST(CoalescerTapSeesEveryRawEventBeforeCoalescing) {
		InputCoalescer coalescer;
		Collector tap;
		coalescer.setTap(&tap);
		coalescer.recorder().onPointerMoved({ 1, 1 });
		coalescer.recorder().onPointerMoved({ 2, 2 });
		coalescer.recorder().onPointerMoved({ 3, 3 });

		CHECK_EQUAL(3u, tap.calls.size());
		CHECK_EQUAL(1u, coalescer.pendingEvents());
	}
}
