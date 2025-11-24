#include <cstdint>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>

namespace lrbm {
	static int create_memfd(const char* name) {
		int fd = syscall(SYS_memfd_create, name, 0);
		if (fd == -1) throw std::runtime_error("memfd_create failed");
		return fd;
	}

	std::pair<void*, size_t> create_linear_ring(size_t size) {
		const size_t page = sysconf(_SC_PAGESIZE);

		size_t aligned = (size + page - 1) & ~(page - 1);

		// Create RAM-backed file descriptor
		int fd = create_memfd("linear_ring");
		if (ftruncate(fd, aligned) != 0) throw std::runtime_error("ftruncate failed");

		// Reserve 2× VA space
		uint8_t* base = (uint8_t*) mmap(nullptr, aligned * 2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (base == MAP_FAILED) throw std::runtime_error("mmap reserve failed");

		// First mapping
		void* p1 = mmap(base, aligned, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
		if (p1 != base) throw std::runtime_error("first mmap failed");

		// Mirror mapping
		void* p2 = mmap(base + aligned, aligned, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
		if (p2 != base + aligned) throw std::runtime_error("mirror mmap failed");

		close(fd);
		return { base, aligned }; // linear region [buf][buf]
	}

	void destroy_linear_ring(void* base, size_t size) { munmap(base, size); }

	// void test() {
	// 	const size_t RING_SIZE = 4096;
	// 	uint8_t* ring = (uint8_t*) create_linear_ring(RING_SIZE);

	// 	printf("Linear ring created at %p (size = %zu)\n", ring, RING_SIZE);

	// 	const char* msg = "HELLO_LINEAR_RING";
	// 	size_t msg_len = strlen(msg);

	// 	size_t head = RING_SIZE - 4; // force boundary crossing
	// 	std::memcpy(ring + head, msg, msg_len);

	// 	char out[64];
	// 	std::memcpy(out, ring + head, msg_len);
	// 	out[msg_len] = 0;

	// 	printf("Expected: %s\n", msg);
	// 	printf("Readback: %s\n", out);

	// 	if (std::strcmp(msg, out) == 0) printf("SUCCESS: Linear-address ring works!\n");
	// 	else printf("FAILURE: Ring broken!\n");
	// }
}
