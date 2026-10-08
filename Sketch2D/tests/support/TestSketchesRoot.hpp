#pragma once

#include <filesystem>
#include <string>
#include <unistd.h>

namespace tp::sketch2d::testing {

	class TestSketchesFolder {
	public:
		TestSketchesFolder() :
			mRoot(std::filesystem::temp_directory_path() / ("sketch2d-test-sketches-" + std::to_string(::getpid()))) {}

		~TestSketchesFolder() {
			std::error_code error;
			std::filesystem::remove_all(mRoot, error);
		}

		[[nodiscard]] std::string root() const { return mRoot.string(); }

	private:
		std::filesystem::path mRoot;
	};

	inline std::string testSketchesRoot() {
		static const TestSketchesFolder folder;
		return folder.root();
	}
}
