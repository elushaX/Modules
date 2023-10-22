
#include "Utils.hpp"

#include "ContainersCommon.hpp"

#include "Testing.hpp"

#include <random>

void initializeCallStackCapture();
void deinitializeCallStackCapture();

static bool initialize(const tp::ModuleManifest*) {
	initializeCallStackCapture();
	return true;
}

static void deinitialize(const tp::ModuleManifest*) {
	deinitializeCallStackCapture();
	tp::gTesting.reportState();
	if (tp::gTesting.hasFailed()) {
		exit(1);
	}
}

namespace tp {

	static ModuleManifest* sModuleUtilsDeps[] = { &gModuleContainers, nullptr };
	ModuleManifest gModuleUtils = ModuleManifest("Utils", initialize, deinitialize, sModuleUtilsDeps);

	void memSetVal(void* p, uhalni byteSize, uint1 val) {
		MODULE_SANITY_CHECK(gModuleBase)

		alni alignedVal = val;
		alignedVal = (alignedVal << 8) | alignedVal;
		alignedVal = (alignedVal << 16) | alignedVal;
		alignedVal = (alignedVal << 32) | alignedVal;

		ualni alignedLen = byteSize / sizeof(alni);
		for (ualni idx = 0; idx < alignedLen; idx++) {
			((alni*) p)[idx] = alignedVal;
		}

		ualni unalignedLen = byteSize - (alignedLen * sizeof(alni));
		for (ualni idx = 0; idx < unalignedLen; idx++) {
			((uint1*) p)[byteSize - idx - 1] = val;
		}
	}

	void memCopy(void* left, const void* right, uhalni len) {
		MODULE_SANITY_CHECK(gModuleBase)

		ualni alignedLen = len / sizeof(alni);
		for (ualni idx = 0; idx < alignedLen; idx++) {
			((alni*) left)[idx] = ((alni*) right)[idx];
		}

		ualni unalignedLen = len - (alignedLen * sizeof(alni));
		for (ualni idx = 0; idx < unalignedLen; idx++) {
			((uint1*) left)[len - idx - 1] = ((uint1*) right)[len - idx - 1];
		}
	}

	int1 memCompare(const void* left, const void* right, uhalni len) {
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

	bool memEqual(const void* left, const void* right, uhalni len) { return memCompare(left, right, len) == 0; }

	int1 memCompareVal(const void* left, uhalni len, uint1 val) {
		MODULE_SANITY_CHECK(gModuleBase)
		if (!len) return 0;

		alni valAligned = val;
		valAligned = (valAligned << 8) | valAligned;
		valAligned = (valAligned << 16) | valAligned;
		valAligned = (valAligned << 32) | valAligned;

		ualni alignedLength = len / sizeof(alni);
		for (ualni idx = 0; idx < alignedLength; idx++) {
			if (((alni*) left)[idx] == valAligned) {
				continue;
			}
			if (((alni*) left)[idx] > valAligned) {
				return 1;
			}
			return -1;
		}

		ualni unalignedLength = len - (alignedLength * sizeof(alni));
		for (ualni idx = 0; idx < unalignedLength; idx++) {
			if (((uint1*) left)[len - idx - 1] == val) {
				continue;
			}
			if (((uint1*) left)[len - idx - 1] > val) {
				return 1;
			}
			return -1;
		}
		return 0;
	}

	alnf randomFloat() {
		alnf r = static_cast<alnf>(std::rand()) / static_cast<alnf>(RAND_MAX);
		return r;
	}
}