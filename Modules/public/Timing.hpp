#pragma once

#include "Environment.hpp"

namespace tp {

	typedef alni time_ms;
	typedef alni time_ns;

	extern time_ms gCurrentTime;

	class Timer {

		time_ms mStart;
		time_ms mDuration;

	public:
		Timer();
		explicit Timer(time_ms time);

		time_ms start();
		time_ms duration();
		void setDuration(time_ms dur);

		bool isTimeout();
		void reset();
		time_ms timePassed();
		time_ms remainder();
		void wait();

		float easeIn(time_ms duration = 0);
		float easeOut(time_ms duration = 0);
	};

	void sleep(time_ms duration);
	time_ms get_time();
	void updateGlobalTime();

	struct FpsCounter {
		halni frames = 0;
		Timer time;
		halni fps = 0;

		FpsCounter() :
			time(1000) {}

		void update(bool log = true) {
			frames++;
			if (time.isTimeout()) {
				fps = frames;
				if (log) {
					// printf("fps %i \n", fps);
				}
				frames = 0;
				time.reset();
			}
		}
	};
}