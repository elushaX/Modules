#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <vector>

template <bool ALLOW_BUFFER_OVERRIDE>
class RingBufferByte {
public:
	explicit RingBufferByte(size_t maxCapacity) { buffer_.resize(maxCapacity); }

	~RingBufferByte() = default;

	void pushBack(const uint8_t* buffer, size_t size) {
		const auto REQUIRED_SIZE = size + sizeof(size);
		const auto DEPOSIT = static_cast<int>(freeSpace()) - static_cast<int>(REQUIRED_SIZE);

		if (DEPOSIT < 0) {
			if (ALLOW_BUFFER_OVERRIDE) {
				freeupSpace(static_cast<size_t>(-DEPOSIT));
			} else {
				throw std::runtime_error("ring buffer overflow");
			}
		}

		push(reinterpret_cast<const uint8_t*>(&size), sizeof(size));
		push(buffer, size);
	}

	[[nodiscard]] auto frontSize() const -> size_t {
		size_t size = 0;

		if (dataSize() < sizeof(size)) {
			throw std::runtime_error("no data in the buffer");
		}

		peek(reinterpret_cast<uint8_t*>(&size), sizeof(size));

		return size;
	}

	auto popFront(uint8_t* buffer) -> size_t {
		size_t size = 0;

		if (dataSize() < sizeof(size)) {
			return 0;
		}

		pop(reinterpret_cast<uint8_t*>(&size), sizeof(size));
		pop(buffer, size);

		return size;
	}

	void clear() {
		end_ = 0;
		start_ = 0;
	}

	[[nodiscard]] bool empty() const { return start_ == end_; }

private:
	void push(const uint8_t* buffer, size_t size) {
		for (size_t simpleIdx = 0; simpleIdx < size; simpleIdx++) {
			buffer_[end_++] = buffer == nullptr ? 0 : buffer[simpleIdx];

			if (end_ == buffer_.size()) {
				end_ = 0;
			}
		}
	}

	void pop(uint8_t* buffer, size_t size) {
		for (size_t sampleIdx = 0; sampleIdx < size; sampleIdx++) {
			if (buffer != nullptr) {
				buffer[sampleIdx] = buffer_[start_];
			}

			start_++;

			if (start_ == buffer_.size()) {
				start_ = 0;
			}
		}
	}

	void peek(uint8_t* buffer, size_t size) const {
		auto iter = start_;
		for (size_t sampleIdx = 0; sampleIdx < size; sampleIdx++) {
			buffer[sampleIdx] = buffer_[iter++];

			if (iter == buffer_.size()) {
				iter = 0;
			}
		}
	}

	void freeupSpace(size_t additionalRequiredSpace) {
		const auto CURRENT_DEPOSIT = freeSpace();

		while (CURRENT_DEPOSIT + additionalRequiredSpace >= freeSpace()) {
			popFront(nullptr);
		}
	}

	[[nodiscard]] size_t freeSpace() const {
		if (start_ > end_) {
			return start_ - end_;
		}

		const auto OUT = buffer_.size() - (end_ - start_);
		return OUT;
	}

	[[nodiscard]] size_t dataSize() const { return buffer_.size() - freeSpace(); }

	[[nodiscard]] bool hasSpace(unsigned int length) const { return freeSpace() > length; }

	std::vector<uint8_t> buffer_;

	size_t end_ = 0;
	size_t start_ = 0;
};
