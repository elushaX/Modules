
#include "Module.hpp"

#include <iostream>

using namespace tp;

ModuleManifest::ModuleManifest(const char* aModuleName, ModuleInit aInit, ModuleDeinit aDeinit, ModuleManifest** aDependencies) {
	mInit = aInit;
	mDeinit = aDeinit;
	mDependencies = aDependencies;
	mModuleName = aModuleName;
}

bool ModuleManifest::isInitialized() const { return mInitialized; }

bool ModuleManifest::initialize(const ModuleManifest* parent) {

	if (!parent) std::cout << "===== Initialization Start =====\n";

	mInitCount++;

	if (isInitialized()) {
		return true;
	}

	mInitialized = true;

	for (auto module = mDependencies; module && *module; module++) {
		mInitialized &= (*module)->initialize(this);
	}

	std::cout << " * Initializing \"" << mModuleName << "\" from \"" << (parent ? parent->mModuleName : mModuleName) << "\"\n";

	if (mInit) mInitialized &= mInit(this);

	if (!mInitialized) {
		std::cout << "Failed to Initialize.\n";
	}

	if (!parent) std::cout << "===== Initialization End =====\n\n";

	return mInitialized;
}

void ModuleManifest::deinitialize() {
	mInitCount--;

	if (mInitCount > 0) {
		return;
	}

	if (!isInitialized()) {
		return;
	}

	if (mDeinit) mDeinit(this);
	mInitialized = false;

	auto len = 0;
	for (auto module = mDependencies; module && *module; module++) {
		len++;
	}

	for (auto i = 0; i < len; i++) {
		auto module = mDependencies + (len - i - 1);
		if ((*module)->isInitialized()) {
			(*module)->deinitialize();
		}
	}
}

const char* ModuleManifest::getName() const { return mModuleName; }
