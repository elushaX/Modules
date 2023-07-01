
#include "Tests.hpp"

#include "Testing.hpp"

#include <cstdlib>

static bool init(const tp::ModuleManifest* self) {
	tp::gTesting.setRootName(self->getName());
	return true;
}

void* TestAllocator::allocate(tp::ualni size) {
	nAllocations++;
	return malloc(size);
}

void TestAllocator::deallocate(void* p) {
	nAllocations--;
	free(p);
}

tp::ualni TestAllocator::getAllocationsCount() const {
	return nAllocations;
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("ContainersTest", init, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testList();
	testMap();
	testAvl();
  testBuffer();

	testModule.deinitialize();
}
