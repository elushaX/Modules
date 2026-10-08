#pragma once

#include "FileBrowser.hpp"

#include <map>
#include <set>
#include <string>
#include <utility>

namespace tp::ui::testing {

	class MemoryFileSystem : public FileSystem {
	public:
		explicit MemoryFileSystem(std::string home = "/") :
			mHome(std::move(home)) {
			addDirectory(mHome);
		}

		void addDirectory(const std::string& path) {
			if (path != "/") addDirectory(parentPath(path));
			mDirectories.insert(path);
		}

		void addFile(const std::string& path, uint64_t size) {
			addDirectory(parentPath(path));
			mFiles[path] = size;
		}

		void addTextFile(const std::string& path, const std::string& text) {
			addFile(path, text.size());
			mTexts[path] = text;
		}

		[[nodiscard]] std::optional<std::vector<FileEntry>> list(const std::string& directory) const override {
			if (!mDirectories.count(directory)) return std::nullopt;

			std::vector<FileEntry> entries;
			for (const std::string& path : mDirectories) {
				if (path != "/" && parentPath(path) == directory) entries.push_back({ fileName(path), true, 0 });
			}
			for (const auto& [path, size] : mFiles) {
				if (parentPath(path) == directory) entries.push_back({ fileName(path), false, size });
			}
			return entries;
		}

		[[nodiscard]] bool isDirectory(const std::string& path) const override { return mDirectories.count(path) > 0; }

		[[nodiscard]] bool exists(const std::string& path) const override {
			return mDirectories.count(path) > 0 || mFiles.count(path) > 0;
		}

		bool createDirectory(const std::string& path) override {
			if (exists(path) || !isDirectory(parentPath(path))) return false;
			mDirectories.insert(path);
			return true;
		}

		[[nodiscard]] std::optional<std::string> readText(const std::string& path) const override {
			const auto found = mTexts.find(path);
			if (found == mTexts.end()) return std::nullopt;
			return found->second;
		}

		bool writeText(const std::string& path, const std::string& text) override {
			if (!isDirectory(parentPath(path))) return false;
			mTexts[path] = text;
			mFiles[path] = text.size();
			return true;
		}

		[[nodiscard]] std::string home() const override { return mHome; }

	private:
		std::string mHome;
		std::set<std::string> mDirectories;
		std::map<std::string, uint64_t> mFiles;
		std::map<std::string, std::string> mTexts;
	};
}
