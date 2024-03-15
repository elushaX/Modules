
#include "UnitTest++/UnitTest++.h"

#include "Callstack.hpp"

#include <cstdio>

using namespace tp;

void common(CallStackCapture& cs) { auto tmp = cs.getSnapshot(); }

void first(CallStackCapture& cs) {
	common(cs);
	common(cs);
	common(cs);
}

void second(CallStackCapture& cs) {
	common(cs);
	common(cs);
	common(cs);
	common(cs);
}

void third(CallStackCapture& cs) {
	common(cs);
	common(cs);
}

void root(CallStackCapture& cs) {
	first(cs);
	second(cs);
	third(cs);
}

SUITE(Utils) {
	TEST(CallStackCapture) {
		CallStackCapture callstack;
		root(callstack);
		callstack.logAll();
	}
}

int main() { return UnitTest::RunAllTests(); }