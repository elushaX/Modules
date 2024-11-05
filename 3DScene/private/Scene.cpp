
#include "Scene.hpp"

bool tp::Scene::load(const std::string& scenePath) {
	try {
		auto res = loadLuaFormat(scenePath);
		if (!res) throw IOError("Failed loading lua script");
	} catch (const IOError& err) {
		printf("Failed loading scene : %s\n", err.description.c_str());
		return false;
	}

	return true;
}