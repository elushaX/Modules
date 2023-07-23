#pragma once

#include <pthread.h>

namespace tp {
	class Mutex {
		pthread_mutex_t mMutex {};
	public:
		Mutex() = default;

		void lock() {
			pthread_mutex_lock(&mMutex);
		}

		void unlock() {
			pthread_mutex_unlock(&mMutex);
		}
	};
}