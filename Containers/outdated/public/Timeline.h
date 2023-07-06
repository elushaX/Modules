#pragma once

#include "Array.h"

namespace tp {
	extern ModuleManifest gModuleContainer;

	template <typename Type, bool DEBUG = true>
	class TimeLine {
		tp::Array<Type> mMem;
		tp::halni mStart = 0;
		tp::halni mLoad = 0;

	public:
		TimeLine(tp::halni aNSnapshots) {
			MODULE_SANITY_CHECK(gModuleContainer);
			mMem.reserve(aNSnapshots);
		}

		Type* operator[](tp::halni aIdx) const {
			assert(aIdx < mLoad);
			auto const idx = mStart + aIdx;
			if (idx >= mMem.length()) {
				idx -= mMem.length();
			}
			return &mMem[idx];
		}

		void add(const Type& val) {
			auto const len = mMem.length() - 1;
			if (mLoad == len) {
				if (mStart == len) {
					mStart = 0;
				}
				mMem[mStart] = val;
				mStart++;
			}
			else {
				mMem[mLoad] = val;
				mLoad++;
			}
		}

		Type* left() {
			return &mMem[mStart];
		}

		Type* right() {
			if (mLoad == mMem.length() - 1) {
				return &mMem[0];
			}
			return NULL;
		}

		const Type* left() const {
			return &mMem[mStart];
		}

		const Type* right() const {
			if (mLoad == mMem.length() - 1) {
				return &mMem[0];
			}
			return NULL;
		}

		tp::halni leftLen() const {
			if (mLoad == mMem.length() - 1) {
				return mMem.length() - (mStart + 1);
			}
			return mLoad + 1;
		}

		tp::halni rightLen() const {
			if (mLoad == mMem.length() - 1) {
				return mStart + 1;
			}
			return 0;
		}
	};

	template <typename Type>
	class TimeLine<Type, false> {
	public:
		TimeLine(tp::halni dummy) {}
		Type* operator[](tp::halni aIdx) const { return NULL; }
		void add(const Type& val) {}
		Type* left() { return NULL; }
		Type* right() { return NULL; }
		const Type* left() const { return NULL;	}
		const Type* right() const { return NULL; }
		tp::halni leftLen() const { return NULL; }
		tp::halni rightLen() const { return NULL;	}
	};

	template <typename Type>
	using DebugTimeline = TimeLine<Type, ENV_BUILD_DEBUG_VAR>;
};