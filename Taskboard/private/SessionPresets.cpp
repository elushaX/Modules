#include "SessionPresets.hpp"

#include "picojson.h"

#include <algorithm>

using namespace tp;
using namespace tp::taskboard;

namespace {
	constexpr const char* kPresetsFile = "session-presets.json";

	std::string text(const picojson::object& object, const char* key) {
		const auto found = object.find(key);
		if (found == object.end() || !found->second.is<std::string>()) return {};
		return found->second.get<std::string>();
	}

	std::optional<SessionPreset> presetFrom(const picojson::value& value) {
		if (!value.is<picojson::object>()) return std::nullopt;
		const picojson::object& object = value.get<picojson::object>();
		SessionPreset preset{
			text(object, "name"),
			{ text(object, "agent_model"), text(object, "harness"), text(object, "effort"), text(object, "permission_mode") },
		};
		if (preset.name.empty()) return std::nullopt;
		return preset;
	}

	picojson::value toJson(const SessionPreset& preset) {
		picojson::object object;
		object["name"] = picojson::value(preset.name);
		object["agent_model"] = picojson::value(preset.settings.agentModel);
		object["harness"] = picojson::value(preset.settings.harness);
		object["effort"] = picojson::value(preset.settings.effort);
		object["permission_mode"] = picojson::value(preset.settings.permissionMode);
		return picojson::value(object);
	}
}

SessionPresets::SessionPresets(ui::FileSystem& files, std::string storageDirectory) :
	mFiles(files),
	mPath(pathIn(storageDirectory)) {
	reload();
}

std::string SessionPresets::pathIn(const std::string& storageDirectory) {
	return ui::joinPath(storageDirectory, kPresetsFile);
}

void SessionPresets::reload() {
	mPresets.clear();
	const std::optional<std::string> stored = mFiles.readText(mPath);
	if (!stored) return;

	picojson::value document;
	if (!picojson::parse(document, *stored).empty() || !document.is<picojson::object>()) return;
	const picojson::object& root = document.get<picojson::object>();
	const auto list = root.find("presets");
	if (list == root.end() || !list->second.is<picojson::array>()) return;

	for (const picojson::value& entry : list->second.get<picojson::array>()) {
		std::optional<SessionPreset> preset = presetFrom(entry);
		if (preset && !indexOf(preset->name)) mPresets.push_back(std::move(*preset));
	}
}

std::optional<size_t> SessionPresets::indexOf(const std::string& name) const {
	const auto found =
		std::find_if(mPresets.begin(), mPresets.end(), [&](const SessionPreset& preset) { return preset.name == name; });
	if (found == mPresets.end()) return std::nullopt;
	return size_t(found - mPresets.begin());
}

std::optional<size_t> SessionPresets::matching(const LaunchSettings& settings) const {
	const auto found = std::find_if(mPresets.begin(), mPresets.end(), [&](const SessionPreset& preset) {
		return preset.settings == settings;
	});
	if (found == mPresets.end()) return std::nullopt;
	return size_t(found - mPresets.begin());
}

size_t SessionPresets::save(const std::string& name, const LaunchSettings& settings) {
	size_t index = 0;
	if (const std::optional<size_t> existing = indexOf(name)) {
		index = *existing;
		mPresets[index].settings = settings;
	} else {
		index = mPresets.size();
		mPresets.push_back({ name, settings });
	}
	store();
	return index;
}

void SessionPresets::remove(size_t index) {
	if (index >= mPresets.size()) return;
	mPresets.erase(mPresets.begin() + long(index));
	store();
}

void SessionPresets::store() {
	picojson::array list;
	for (const SessionPreset& preset : mPresets)
		list.push_back(toJson(preset));
	picojson::object root;
	root["presets"] = picojson::value(list);

	mSaveFailed = !ui::createDirectories(mFiles, ui::parentPath(mPath)) ||
								!mFiles.writeText(mPath, picojson::value(root).serialize(true));
}
