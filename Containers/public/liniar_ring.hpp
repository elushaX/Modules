
#include <cstddef>
#include <utility>

namespace lrbm {
	std::pair<void*, size_t> create_linear_ring(size_t size);
	void destroy_linear_ring(void* base, size_t size);
	void test();
}
