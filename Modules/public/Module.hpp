
#pragma once

#include "Common.hpp"
#include "Assert.hpp"

#define MODULE_SANITY_CHECK(name) DEBUG_ASSERT(name.isInitialized() && "Modules Is Not Initialized" && #name)

namespace tp {

	class ModuleManifest {
	public:
		typedef bool (*ModuleInit)(const ModuleManifest*);
		typedef void (*ModuleDeinit)(const ModuleManifest*);

		ModuleManifest(const char* aModuleName, ModuleInit aInit, ModuleDeinit aDeinit, ModuleManifest** aDependencies);
		[[nodiscard]] bool isInitialized() const;
		bool initialize(const ModuleManifest* parent = nullptr);
		void deinitialize();
		[[nodiscard]] const char* getName() const;
		void setCustomData(void* data) { mCustomData = data; }
		void* getCustomData(void* data) const { return mCustomData; }

	private:
		const char* mModuleName = nullptr;
		ModuleManifest** mDependencies; // NULL terminated
		bool mInitialized = false;
		ModuleInit mInit = nullptr;
		ModuleDeinit mDeinit = nullptr;
		uhalni mInitCount = 0;
		void* mCustomData = nullptr;
	};

	extern ModuleManifest gModuleBase;
};
