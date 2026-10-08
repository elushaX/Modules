#pragma once

#include "InputCodes.hpp"
#include "PointerSample.hpp"
#include "TouchGesture.hpp"
#include "Vec.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tp {

	class WindowInputObserver;

	enum class DeliveryPolicy { Every, Latest, Accumulate };

	struct InputDeliveryPolicies {
		DeliveryPolicy pointerMotion = DeliveryPolicy::Latest;
		DeliveryPolicy scroll = DeliveryPolicy::Accumulate;
	};

	struct WindowConfigure {
		int32_t width = 0;
		int32_t height = 0;
		bool maximized = false;
		bool fullscreen = false;
		bool activated = false;
		bool tiled = false;
		uint32_t serial = 0;
	};

	struct CoalescingStatistics {
		uint64_t received = 0;
		uint64_t delivered = 0;
		uint64_t configuresReceived = 0;
		uint64_t configuresApplied = 0;
	};

	class InputCoalescer {
	public:
		InputCoalescer();
		~InputCoalescer();

		InputCoalescer(const InputCoalescer&) = delete;
		InputCoalescer& operator=(const InputCoalescer&) = delete;

		[[nodiscard]] WindowInputObserver& recorder();
		void setTap(WindowInputObserver* tap) { mTap = tap; }
		[[nodiscard]] WindowInputObserver* tap() const { return mTap; }

		void setPolicies(const InputDeliveryPolicies& policies) { mPolicies = policies; }
		[[nodiscard]] const InputDeliveryPolicies& policies() const { return mPolicies; }

		void pointerSample(const PointerSample& sample) { mSample = sample; }
		void pointerMoved(Vec2F position);
		void pointerLeft();
		void pointerButton(InputID button, bool pressed);
		void scrolled(Vec2F delta);
		void key(InputID key, bool pressed);
		void keyRepeat(InputID key);
		void text(const std::string& utf8);
		void keyboardFocus(bool focused);
		void gesture(const TouchGesture& gesture);

		void configure(const WindowConfigure& configure);
		[[nodiscard]] bool hasPendingConfigure() const { return mHasConfigure; }
		[[nodiscard]] WindowConfigure takeConfigure();

		[[nodiscard]] bool isEmpty() const { return mEvents.empty(); }
		[[nodiscard]] size_t pendingEvents() const { return mEvents.size(); }
		void deliver(WindowInputObserver& observer);

		[[nodiscard]] const CoalescingStatistics& statistics() const { return mStatistics; }

	private:
		enum class Kind { Motion, Leave, Button, Scroll, Key, Repeat, Text, Focus, Gesture };

		struct Pending {
			Kind kind;
			Vec2F vector = { 0, 0 };
			InputID id = InputID::SPACE;
			bool flag = false;
			std::string text;
			PointerSample sample;
			TouchGesture gesture;
		};

		void push(Pending event, DeliveryPolicy policy);

		class Recorder;

		std::unique_ptr<Recorder> mRecorder;
		WindowInputObserver* mTap = nullptr;
		InputDeliveryPolicies mPolicies;
		std::vector<Pending> mEvents;
		PointerSample mSample;
		PointerSample mDeliveredSample;
		WindowConfigure mConfigure;
		bool mHasConfigure = false;
		CoalescingStatistics mStatistics;
	};
}
