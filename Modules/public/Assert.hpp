
#pragma once

#include "Environment.hpp"

namespace tp {
	void _assert_(const char* exp, const char* file, int line);
	void terminate(tp::alni code = 0);
};

#define FAIL(exp) tp::_assert_(#exp, __FILE__, __LINE__);
#define ASSERT(exp) \
	if (!(exp)) {     \
		FAIL(exp)       \
	}

#undef assert
#ifdef ENV_BUILD_DEBUG
#define DEBUG_ASSERT(exp) ASSERT(exp)
#else
#define DEBUG_ASSERT(exp) \
	{}
#endif

#if defined(ENV_OS_WINDOWS)
#define DEBUG_BREAK(expr) \
	if (expr) {             \
		__debugbreak();       \
	}
#elif defined(ENV_OS_ANDROID)
#define DEBUG_BREAK(expr)  \
	if (expr) {              \
		__builtin_debugtrap(); \
	}
#elif defined(ENV_OS_LINUX)

#include <csignal>

#define DEBUG_BREAK(expr) \
	if (expr) {             \
		raise(SIGTRAP);       \
	}
#else
#define DEBUG_BREAK(expr) ()
#endif