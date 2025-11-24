#include "LinearRingBuffer.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <cstdint>
#include <random>

static void fill(void* ptr, size_t n, uint8_t value) { std::memset(ptr, value, n); }

static bool check(const void* ptr, size_t n, uint8_t value) {
	const uint8_t* p = static_cast<const uint8_t*>(ptr);
	for (size_t i = 0; i < n; ++i) {
		if (p[i] != value) return false;
	}
	return true;
}

static bool expect_mem_eq(const void* ptr, size_t n, uint8_t value) {
	const uint8_t* p = static_cast<const uint8_t*>(ptr);
	for (size_t i = 0; i < n; ++i) {
		if (p[i] != value) return false;
	}
	return true;
}

using Data = std::vector<uint8_t>;
using DataStorage = std::vector<Data>;

typedef ::testing::Types<
	std::integral_constant<size_t, 64>,
	std::integral_constant<size_t, 128>,
	std::integral_constant<size_t, 1000>,
	std::integral_constant<size_t, 4096>,
	std::integral_constant<size_t, 4096 * 2>>
	BufferSizes;

template <typename TypeParam>
class LinearRingBufferTest : public ::testing::Test {
protected:
	LinearRingBuffer rb;

	LinearRingBufferTest() :
		historySize_(10),
		rb(size_t(TypeParam::value), size_t(historySize_)) {}

	static constexpr auto ITERATIONS = TypeParam::value * 10;

	size_t messagesPushed_ = 0;
	size_t historySize_ = 0;

	auto iterations() { return ITERATIONS; }

	auto historySize() { return historySize_; }

	size_t randomSize() {
		static std::random_device rnd;
		std::uniform_int_distribution<size_t> dist(0, 1000);
		return dist(rnd);
	}

	auto write_and_advance(size_t messageSize, DataStorage* storage) {
		Data pushedSample(messageSize);

		for (auto idx = 0; idx < messageSize; idx++) {
			pushedSample[idx] = (idx + messagesPushed_++ * 53) % 321;
		}

		std::memcpy(this->rb.write_data(), pushedSample.data(), pushedSample.size());
		this->rb.write_advance(pushedSample.size());

		if (storage) {
			storage->push_back(pushedSample);
		}
	};

	auto read_and_advance(size_t messageSize, DataStorage* storage) {
		Data poppedSample(messageSize);

		std::memcpy(poppedSample.data(), this->rb.read_data(), poppedSample.size());

		this->rb.read_advance(poppedSample.size());

		if (storage) {
			storage->push_back(poppedSample);
		}
	};

	auto reset_with_history() { this->rb.read_reset_with_history(); };

	auto read_with_history(size_t messageSize, DataStorage* storage) {
		Data poppedSample(messageSize);

		std::memcpy(poppedSample.data(), this->rb.read_data(), poppedSample.size());

		if (storage) {
			storage->push_back(poppedSample);
		}
	};
};

TYPED_TEST_SUITE(LinearRingBufferTest, BufferSizes);

TYPED_TEST(LinearRingBufferTest, LinearWriteAcrossBoundary) {
	constexpr size_t N = TypeParam::value;

	if (N % 4096 != 0) {
		return;
	}

	auto* base = this->rb.write_data();

	size_t half = N - 16;
	fill(base, half, 0xAA);
	fill(base + half, 32, 0xBB);

	EXPECT_EQ(expect_mem_eq(base, 16, 0xBB), true);
	EXPECT_EQ(expect_mem_eq(base + 16, half - 16, 0xAA), true);
	EXPECT_EQ(expect_mem_eq(base + half, 32, 0xBB), true);
}

TYPED_TEST(LinearRingBufferTest, Chase) {

	DataStorage pushed;
	DataStorage popped;

	static constexpr auto MSG_SIZE = 31;

	for (auto iteration = 0; iteration < this->iterations(); iteration++) {
		this->write_and_advance(MSG_SIZE, &pushed);
		this->read_and_advance(MSG_SIZE, &popped);
	}

	ASSERT_EQ(pushed.size(), popped.size());

	for (auto idx = 0; idx < popped.size(); idx++) {
		ASSERT_EQ(pushed[idx], popped[idx]);
	}
}

Data makeMessageWithHistory(const Data& prev, const Data& current, size_t historySize) {
	if (historySize > prev.size()) historySize = prev.size();

	std::vector<uint8_t> result;

	result.reserve(historySize + current.size());

	result.insert(result.end(), prev.end() - historySize, prev.end());
	result.insert(result.end(), current.begin(), current.end());

	return result;
}

TYPED_TEST(LinearRingBufferTest, History) {

	DataStorage pushed;
	DataStorage popped;

	pushed.push_back(Data(this->historySize()));

	static constexpr auto MAX_MSG_SIZE = 111;

	for (auto iteration = 0; iteration < this->iterations(); iteration++) {

		const auto MSG_SIZE = this->historySize() + this->randomSize() % MAX_MSG_SIZE;

		auto prevPushed = pushed.back();
		pushed.clear();

		this->reset_with_history();

		this->write_and_advance(MSG_SIZE, &pushed);
		auto currentPushed = pushed.back();

		this->read_with_history(this->historySize() + MSG_SIZE, &popped);
		auto poppedWithHistory = popped.back();
		popped.clear();

		auto correctResult = makeMessageWithHistory(prevPushed, currentPushed, this->historySize());

		ASSERT_EQ(correctResult, poppedWithHistory);
	}
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
