
#include <chrono>
#include <thread>

#include "Timing.hpp"
#include "Utils.hpp"

#define GETTIMEMSC() (time_ms)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch()).count())

#define THREAD_SLEEP(time_ms) std::this_thread::sleep_for(std::chrono::milliseconds(time_ms))

tp::time_ms tp::gCurrentTime = tp::get_time();

namespace tp {
	time_ms get_time() {
		gCurrentTime = GETTIMEMSC();
		return gCurrentTime;
	}

	void sleep(time_ms mDuration) { THREAD_SLEEP(mDuration); }

	Timer::Timer() {
		mDuration = 0;
		mStart = GETTIMEMSC();
	}

	Timer::Timer(time_ms mDuration) {
		mStart = GETTIMEMSC();
		this->mDuration = mDuration;
	}

	bool Timer::isTimeout() { return mDuration < GETTIMEMSC() - mStart; }

	void Timer::reset() { mStart = GETTIMEMSC(); }

	time_ms Timer::timePassed() { return GETTIMEMSC() - mStart; }

	time_ms Timer::remainder() { return mDuration - (GETTIMEMSC() - mStart); }

	time_ms Timer::start() { return mStart; }
	time_ms Timer::duration() { return mDuration; }
	void Timer::setDuration(time_ms dur) { mDuration = dur; }

	void Timer::wait() {
		if (!isTimeout()) {
			sleep(remainder());
		}
	}

	float Timer::easeIn(time_ms pDuration) {
		if (!pDuration) {
			pDuration = mDuration;
		}
		float x = (1.f / pDuration) * timePassed();
		return clamp((1.1f * x) / (x + 0.1f), 0.f, 1.f);
	}

	float Timer::easeOut(time_ms pDuration) {
		if (!pDuration) {
			pDuration = mDuration;
		}
		float x = (1.f / pDuration) * timePassed();
		return clamp((0.1f * (1 - x)) / (x + 0.1f), 0.f, 1.f);
	}
}