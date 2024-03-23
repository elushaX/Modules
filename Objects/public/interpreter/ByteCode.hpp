
#pragma once

#include "OperatoinCodes.hpp"

#include "primitives/IntObject.hpp"
#include "primitives/StringObject.hpp"

#include "Buffer.hpp"
#include <string>

namespace obj {

	typedef Object* ConstData;

	struct ByteCode {
		tp::Buffer<ConstData> mConstants;
		tp::Buffer<OpCode> mInstructions;
		tp::ualni mInstructionIdx = 0;
		tp::ualni mArgumentsLoaded = 0;

		~ByteCode() {
			for (auto const_obj : mConstants) {
				NDO->destroy(const_obj.data());
			}
		}
	};
};