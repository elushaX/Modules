
#include "Scene.hpp"

bool tp::Scene::load(const std::string& scenePath) {
	return loadLuaFormat(scenePath);
}