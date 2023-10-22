
#include "Assert.hpp"

#include <cstdio>
#include <cstdlib>

using namespace tp;

void tp::_assert_(const char* exp, const char* file, int line) {
	if (!exp) {
		exp = "no info";
	}
	printf("\nERROR: Assertion Failure - %s -- %s:%i\n", exp, file, line);

#ifdef ENV_BUILD_DEBUG
	DEBUG_BREAK(true);
#else
	exit(1);
#endif
}

void tp::terminate(tp::alni code) { exit((int) code); }