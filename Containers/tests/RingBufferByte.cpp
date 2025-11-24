
#include "Tests.hpp"
#include "RingBufferByte.hpp"

SUITE(RingBufferBytes) {
	TEST(Simple) {

		using Data = std::vector<uint8_t>;
		using DataStorage = std::vector<Data>;

		constexpr auto BUFFER_SIZE = 395;
		constexpr auto MAX_MSG_SIZE = 31;
		constexpr auto ITERATIONS = 100000;

		DataStorage pushed;
		DataStorage popped;

		RingBufferByte<true> buffer(BUFFER_SIZE);

		Data randomSamples(MAX_MSG_SIZE);

		for (auto idx = 0; idx < MAX_MSG_SIZE; idx++) {
			randomSamples[idx] = (idx + 5 * 31) % 13;
		}

		const auto PUSH = [&](RingBufferByte<true>& buffer, DataStorage* storage) {
			const Data& pushedSample = randomSamples;

			buffer.pushBack(pushedSample.data(), pushedSample.size());

			if (storage) {
				storage->push_back(pushedSample);
			}
		};

		const auto POP = [](RingBufferByte<true>& buffer, DataStorage* storage) {
			const auto poppedSampleSize = buffer.frontSize();

			Data poppedSample(poppedSampleSize);

			const auto SIZE = buffer.popFront(poppedSample.data());

			CHECK(poppedSample.size() == SIZE);

			if (storage) {
				storage->push_back(poppedSample);
			}
		};

		for (auto iteration = 0; iteration < ITERATIONS; iteration++) {
			PUSH(buffer, nullptr);
		}

		while (!buffer.empty()) {
			POP(buffer, nullptr);
		}

		for (auto iteration = 0; iteration < ITERATIONS; iteration++) {
			PUSH(buffer, &pushed);
			POP(buffer, &popped);
		}

		CHECK(pushed.size() == popped.size());

		for (auto idx = 0; idx < popped.size(); idx++) {
			CHECK(pushed[idx] == popped[idx]);
		}
	}
}
