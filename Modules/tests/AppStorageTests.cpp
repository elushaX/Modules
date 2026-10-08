#include "AppStorage.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <string>

using namespace tp;

SUITE(AppStorage) {
	TEST(EveryApplicationKeepsItsDataInOneFolderInTheHomeDirectory) {
		const char* previous = std::getenv("HOME");
		const std::string saved = previous ? previous : "";
		setenv("HOME", "/home/someone", 1);

		const AppStorage storage = AppStorage::inHome("taskboard");
		CHECK_EQUAL("/home/someone/taskboard", storage.root());
		CHECK_EQUAL("/home/someone/taskboard/images", storage.imagesDirectory());
		CHECK_EQUAL("/home/someone/taskboard/sessions", storage.sessionsDirectory());
		CHECK_EQUAL("/home/someone/taskboard/hangs", storage.hangsDirectory());
		CHECK_EQUAL("/home/someone/taskboard/session-presets.json", storage.path("session-presets.json"));

		if (previous) setenv("HOME", saved.c_str(), 1);
		else unsetenv("HOME");
	}
}
