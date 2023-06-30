
#include "Utils.hpp"

#include "ContainersCommon.hpp"

#include "Testing.hpp"

#include <random>

void initializeCallStackCapture();
void deinitializeCallStackCapture();

static bool initialize(const tp::ModuleManifest* self) {
	initializeCallStackCapture();
	return true;
}

static void deinitialize(const tp::ModuleManifest* self) {
	deinitializeCallStackCapture();
	tp::gTesting.reportState();
	if (tp::gTesting.hasFailed()) { exit(1); }
}

namespace tp {

	static ModuleManifest* sModuleUtilsDeps[] = { &gModuleContainers, nullptr };
	ModuleManifest gModuleUtils = ModuleManifest("Utils", initialize, deinitialize, sModuleUtilsDeps);

	void memsetv(void* p, uhalni bytesize, uint1 val) {
		MODULE_SANITY_CHECK(gModuleBase)

		alni alignedval = 0;
		for (ualni idx = 0; idx < sizeof(alni); idx++) {
			((uint1*) &alignedval)[idx] = val;
		}

		ualni alignedlen = bytesize / sizeof(alni);
		for (ualni idx = 0; idx < alignedlen; idx++) {
			((alni*) p)[idx] = alignedval;
		}

		ualni unalignedlen = bytesize - (alignedlen * sizeof(alni));
		for (ualni idx = 0; idx < unalignedlen; idx++) {
			((uint1*) p)[bytesize - idx - 1] = val;
		}
	}

	void memcp(void* left, const void* right, uhalni len) {
		MODULE_SANITY_CHECK(gModuleBase)

		ualni alignedlen = len / sizeof(alni);
		for (ualni idx = 0; idx < alignedlen; idx++) {
			((alni*) left)[idx] = ((alni*) right)[idx];
		}

		ualni unalignedlen = len - (alignedlen * sizeof(alni));
		for (ualni idx = 0; idx < unalignedlen; idx++) {
			((uint1*) left)[len - idx - 1] = ((uint1*) right)[len - idx - 1];
		}
	}

	int1 memecomp(const void* left, const void* right, uhalni len) {
		MODULE_SANITY_CHECK(gModuleBase)
		if (!len) return 0;

		ualni alignedLength = len / sizeof(alni);
		for (ualni idx = 0; idx < alignedLength; idx++) {
			if (((alni*) left)[idx] == ((alni*) right)[idx]) {
				continue;
			}
			if (((alni*) left)[idx] > ((alni*) right)[idx]) {
				return 1;
			}
			return -1;
		}

		ualni unalignedLength = len - (alignedLength * sizeof(alni));
		for (ualni idx = 0; idx < unalignedLength; idx++) {
			if (((uint1*) left)[len - idx - 1] == ((uint1*) right)[len - idx - 1]) {
				continue;
			}
			if (((uint1*) left)[len - idx - 1] > ((uint1*) right)[len - idx - 1]) {
				return 1;
			}
			return -1;
		}
		return 0;
	}

	alnf randf() {
		alnf r = static_cast<alnf>(std::rand()) / static_cast<alnf>(RAND_MAX);
		return r;
	}
}