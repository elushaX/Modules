
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

#include "liniar_ring.hpp"

class LinearRingBuffer {
public:
	LinearRingBuffer(size_t size, size_t history) {
		const auto& [mem, aligned_size] = lrbm::create_linear_ring(size);

		mem_ = (uint8_t*) mem;
		memSize_ = aligned_size;
		bufferSize_ = size;
		historySize_ = history;

		write_ptr_ = mem_;
		read_ptr_ = mem_;
	}

	~LinearRingBuffer() { lrbm::destroy_linear_ring((void*) mem_, memSize_ * 2); }

	uint8_t* write_data() { return write_ptr_; }
	uint8_t* read_data() { return read_ptr_; }

	void write_advance(size_t bytes) { write_ptr_ = advancePointer(write_ptr_, bytes); }
	void read_advance(size_t bytes) { read_ptr_ = advancePointer(read_ptr_, bytes); }

	void read_reset_with_history() {
		const auto offset = size_t(write_ptr_ - mem_);

		if (historySize_ < offset) {
			read_ptr_ = write_ptr_ - historySize_;
			return;
		}

		read_ptr_ = mem_ + (memSize_ + offset - historySize_);
	}

private:
	inline uint8_t* advancePointer(const uint8_t* ptr, size_t bytes) {
		return mem_ + (size_t(ptr - mem_) + bytes) % memSize_;
	}

	size_t bufferSize_ = 0;
	size_t historySize_ = 0;
	size_t memSize_ = 0;

	uint8_t* mem_ = nullptr;

	uint8_t* read_ptr_ = 0;
	uint8_t* write_ptr_ = 0;
};
