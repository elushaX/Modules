#pragma once

#include "FileBrowser.hpp"
#include "Task.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tp::taskboard {

	struct SessionPreset {
		std::string name;
		LaunchSettings settings;

		bool operator==(const SessionPreset&) const = default;
	};

	class SessionPresets {
	public:
		SessionPresets(ui::FileSystem& files, std::string storageDirectory);

		[[nodiscard]] const std::vector<SessionPreset>& all() const { return mPresets; }
		[[nodiscard]] std::optional<size_t> indexOf(const std::string& name) const;
		[[nodiscard]] std::optional<size_t> matching(const LaunchSettings& settings) const;
		[[nodiscard]] const std::string& storagePath() const { return mPath; }
		[[nodiscard]] bool lastSaveFailed() const { return mSaveFailed; }

		size_t save(const std::string& name, const LaunchSettings& settings);
		void remove(size_t index);
		void reload();

		[[nodiscard]] static std::string pathIn(const std::string& storageDirectory);

	private:
		void store();

		ui::FileSystem& mFiles;
		std::string mPath;
		std::vector<SessionPreset> mPresets;
		bool mSaveFailed = false;
	};
}
