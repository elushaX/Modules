
#pragma once

#include "OperatoinCodes.hpp"

#include "primitives/IntObject.hpp"
#include "primitives/StringObject.hpp"

#include "Buffer.hpp"
#include <string>

namespace tp::obj {

	typedef Object* ConstData;

	struct ByteCode {
		ObjectsContext* context = nullptr;

		Buffer<ConstData> mConstants;
		Buffer<OpCode> mInstructions;
		ualni mInstructionIdx = 0;
		ualni mArgumentsLoaded = 0;

		~ByteCode() {
			for (auto const_obj : mConstants) {
				context->destroy(const_obj.data());
			}
		}
	};
}