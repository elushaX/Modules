#include "RenderTargetRegistry.hpp"

#include <unordered_map>

namespace tp {

	namespace {
		std::unordered_map<uint32_t, RenderTargetEntry>& registry() {
			static std::unordered_map<uint32_t, RenderTargetEntry> instance;
			return instance;
		}

		uint32_t& nextId() {
			static uint32_t id = 1;
			return id;
		}
	}

	uint32_t RenderTargetRegistry::registerTarget(const RenderTargetEntry& entry) {
		const uint32_t id = nextId()++;
		registry()[id] = entry;
		return id;
	}

	void RenderTargetRegistry::updateTarget(uint32_t id, const RenderTargetEntry& entry) {
		registry()[id] = entry;
	}

	void RenderTargetRegistry::unregisterTarget(uint32_t id) {
		registry().erase(id);
	}

	bool RenderTargetRegistry::resolve(uint32_t id, RenderTargetEntry& out) {
		auto it = registry().find(id);
		if (it == registry().end()) return false;
		out = it->second;
		return true;
	}
}
