#include "UnitTest++/UnitTest++.h"

#include "UnitTest++/TestReporterStdout.h"

#include <cstdlib>
#include <cstring>
#include <string>

namespace {
	bool startsWith(const char* text, const char* prefix) { return std::strncmp(text, prefix, std::strlen(prefix)) == 0; }

	bool isLong(const char* suite) { return startsWith(suite, "Long") || startsWith(suite, "Bench"); }
}

int main() {
	const char* suite = std::getenv("TEST_SUITE");
	const char* skip = std::getenv("TEST_SKIP_SUITE");
	const char* level = std::getenv("TEST_LEVEL");
	const std::string mode = level ? level : "quick";

	UnitTest::TestReporterStdout reporter;
	UnitTest::TestRunner runner(reporter);
	return runner.RunTestsIf(
		UnitTest::Test::GetTestList(),
		nullptr,
		[&](const UnitTest::Test* test) {
			const char* name = test->m_details.suiteName;
			if (suite && *suite) return std::strcmp(name, suite) == 0;
			if (skip && *skip && std::strcmp(name, skip) == 0) return false;
			if (mode == "all") return true;
			if (mode == "long") return isLong(name);
			return !isLong(name);
		},
		0
	);
}
