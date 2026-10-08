#pragma once

#include <string>

namespace tp {

	class AppStorage {
	public:
		explicit AppStorage(std::string root);

		[[nodiscard]] static AppStorage inHome(const std::string& applicationName);
		[[nodiscard]] static std::string homeDirectory();

		[[nodiscard]] const std::string& root() const { return mRoot; }
		[[nodiscard]] std::string path(const std::string& relative) const;
		[[nodiscard]] std::string imagesDirectory() const { return path("images"); }
		[[nodiscard]] std::string sessionsDirectory() const { return path("sessions"); }
		[[nodiscard]] std::string hangsDirectory() const { return path("hangs"); }

	private:
		std::string mRoot;
	};
}
