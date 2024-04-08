
#include "ContainersCommon.hpp"

#include <cstdlib>

namespace tp {
	void* DefaultAllocator::allocate(ualni size) { return malloc(size); }
	void DefaultAllocator::deallocate(void* p) { free(p); }
}