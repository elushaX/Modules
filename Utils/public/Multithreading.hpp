#pragma once

#include <mutex>

namespace tp {
	class Mutex {
		std::mutex mMutex;

	public:
		Mutex() = default;

		void lock() { mMutex.lock(); }

		void unlock() { mMutex.unlock(); }
	};
}
