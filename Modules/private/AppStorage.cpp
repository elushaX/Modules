#include "AppStorage.hpp"

#include <cstdlib>
#include <filesystem>
#include <utility>

using namespace tp;

AppStorage::AppStorage(std::string root) :
	mRoot(std::move(root)) {}

std::string AppStorage::homeDirectory() {
	const char* home = std::getenv("HOME");
	if (home && *home) return home;
	return std::filesystem::current_path().string();
}

AppStorage AppStorage::inHome(const std::string& applicationName) {
	return AppStorage((std::filesystem::path(homeDirectory()) / applicationName).string());
}

std::string AppStorage::path(const std::string& relative) const {
	return (std::filesystem::path(mRoot) / relative).string();
}
