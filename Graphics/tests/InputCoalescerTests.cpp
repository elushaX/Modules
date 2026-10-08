#include "InputCoalescer.hpp"
#include "WindowInput.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>
#include <string>
#include <vector>

using namespace tp;

namespace {
	class Recorder : public WindowInputObserver {
	public:
		void onPointerSample(const PointerSample& sample) override {
			log.push_back("sample " + std::to_string((int) std::lround(sample.pressure * 100)));
		}
		void onPointerMoved(Vec2F position) override { log.push_back("move " + point(position)); }
		void onPointerLeft() override { log.push_back("leave"); }
		void onPointerButton(InputID button, bool pressed) override {
			log.push_back(std::string(pressed ? "press " : "release ") + std::to_string((int) button));
		}
		void onScrolled(Vec2F delta) override { log.push_back("scroll " + point(delta)); }
		void onKey(InputID key, bool pressed) override {
			log.push_back(std::string(pressed ? "key down " : "key up ") + std::to_string((int) key));
		}
		void onKeyRepeat(InputID key) override { log.push_back("repeat " + std::to_string((int) key)); }
		void onText(const std::string& utf8) override { log.push_back("text " + utf8); }
		void onKeyboardFocus(bool focused) override { log.push_back(focused ? "focus" : "blur"); }

		std::vector<std::string> log;

	private:
		static std::string point(Vec2F value) {
			return std::to_string((int) value.x) + "," + std::to_string((int) value.y);
		}
	};

	std::vector<std::string> drain(InputCoalescer& coalescer) {
		Recorder recorder;
		coalescer.deliver(recorder);
		return recorder.log;
	}
}

SUITE(InputCoalescerPolicies) {
	TEST(OppositeScrollsThatCancelDeliverNothing) {
		InputCoalescer coalescer;
		coalescer.scrolled({ 0, 1 });
		coalescer.scrolled({ 0, -1 });
		CHECK(drain(coalescer).empty());
	}

	TEST(BurstOfMotionCollapsesToLatestPosition) {
		InputCoalescer coalescer;
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerMoved({ 2, 2 });
		coalescer.pointerMoved({ 3, 3 });

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(1u, log.size());
		CHECK_EQUAL("move 3,3", log[0]);
		CHECK_EQUAL(3u, coalescer.statistics().received);
		CHECK_EQUAL(1u, coalescer.statistics().delivered);
	}

	TEST(EveryPolicyKeepsEachMotionSampleForDrawingApps) {
		InputCoalescer coalescer;
		coalescer.setPolicies({ DeliveryPolicy::Every, DeliveryPolicy::Accumulate });
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerMoved({ 2, 2 });
		coalescer.pointerMoved({ 3, 3 });
		CHECK_EQUAL(3u, drain(coalescer).size());
	}

	TEST(MotionNeverMergesAcrossButtons) {
		InputCoalescer coalescer;
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerMoved({ 5, 5 });
		coalescer.pointerButton(InputID::MOUSE1, true);
		coalescer.pointerMoved({ 6, 6 });
		coalescer.pointerMoved({ 9, 9 });
		coalescer.pointerButton(InputID::MOUSE1, false);

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(4u, log.size());
		CHECK_EQUAL("move 5,5", log[0]);
		CHECK_EQUAL("press 501", log[1]);
		CHECK_EQUAL("move 9,9", log[2]);
		CHECK_EQUAL("release 501", log[3]);
	}

	TEST(ScrollDeltasAddUpAndDoNotCrossKeys) {
		InputCoalescer coalescer;
		coalescer.scrolled({ 0, 1 });
		coalescer.scrolled({ 0, 2 });
		coalescer.key(InputID::LEFT_CONTROL, true);
		coalescer.scrolled({ 0, -1 });

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(3u, log.size());
		CHECK_EQUAL("scroll 0,3", log[0]);
		CHECK_EQUAL("key down " + std::to_string((int) InputID::LEFT_CONTROL), log[1]);
		CHECK_EQUAL("scroll 0,-1", log[2]);
	}

	TEST(DiscreteEventsAreNeverMerged) {
		InputCoalescer coalescer;
		coalescer.key(InputID::A, true);
		coalescer.key(InputID::A, false);
		coalescer.key(InputID::A, true);
		coalescer.keyRepeat(InputID::A);
		coalescer.keyRepeat(InputID::A);
		coalescer.text("a");
		coalescer.text("b");
		coalescer.keyboardFocus(false);
		coalescer.pointerLeft();
		CHECK_EQUAL(9u, drain(coalescer).size());
	}

	TEST(DeliveryEmptiesTheQueueAndKeepsOrder) {
		InputCoalescer coalescer;
		coalescer.text("x");
		coalescer.pointerMoved({ 4, 4 });
		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(2u, log.size());
		CHECK_EQUAL("text x", log[0]);
		CHECK_EQUAL("move 4,4", log[1]);
		CHECK(coalescer.isEmpty());
		CHECK_EQUAL(0u, drain(coalescer).size());
	}

	TEST(OnlyTheNewestConfigureIsApplied) {
		InputCoalescer coalescer;
		CHECK(!coalescer.hasPendingConfigure());
		coalescer.configure({ 800, 600, false, false, true, false, 10 });
		coalescer.configure({ 820, 610, false, false, true, false, 11 });
		coalescer.configure({ 900, 640, true, false, true, false, 12 });
		CHECK(coalescer.hasPendingConfigure());

		const WindowConfigure latest = coalescer.takeConfigure();
		CHECK_EQUAL(900, latest.width);
		CHECK_EQUAL(640, latest.height);
		CHECK(latest.maximized);
		CHECK_EQUAL(12u, latest.serial);
		CHECK(!coalescer.hasPendingConfigure());
		CHECK_EQUAL(3u, coalescer.statistics().configuresReceived);
		CHECK_EQUAL(1u, coalescer.statistics().configuresApplied);
	}
}

SUITE(InputCoalescerPenSamples) {
	PointerSample pen(halnf pressure) { return PointerSample{ PointerTool::Pen, pressure }; }

	TEST(EachDeliveredMotionCarriesThePressureItWasRecordedWith) {
		InputCoalescer coalescer;
		coalescer.setPolicies({ DeliveryPolicy::Every, DeliveryPolicy::Accumulate });
		coalescer.pointerSample(pen(0.25f));
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerSample(pen(0.75f));
		coalescer.pointerMoved({ 2, 2 });

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(4u, log.size());
		CHECK_EQUAL("sample 25", log[0]);
		CHECK_EQUAL("move 1,1", log[1]);
		CHECK_EQUAL("sample 75", log[2]);
		CHECK_EQUAL("move 2,2", log[3]);
	}

	TEST(LatestMotionKeepsTheNewestSample) {
		InputCoalescer coalescer;
		coalescer.pointerSample(pen(0.2f));
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerSample(pen(0.9f));
		coalescer.pointerMoved({ 4, 4 });

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(2u, log.size());
		CHECK_EQUAL("sample 90", log[0]);
		CHECK_EQUAL("move 4,4", log[1]);
	}

	TEST(UnchangedSampleIsNotRepeated) {
		InputCoalescer coalescer;
		coalescer.setPolicies({ DeliveryPolicy::Every, DeliveryPolicy::Accumulate });
		coalescer.pointerSample(pen(0.5f));
		coalescer.pointerMoved({ 1, 1 });
		coalescer.pointerButton(InputID::MOUSE1, true);
		coalescer.pointerMoved({ 2, 2 });

		const std::vector<std::string> log = drain(coalescer);
		REQUIRE CHECK_EQUAL(4u, log.size());
		CHECK_EQUAL("sample 50", log[0]);
		CHECK_EQUAL("move 2,2", log[3]);
	}
}
