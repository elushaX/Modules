
#pragma once

#include "opcodes.h"

#include "primitives/intobject.h"
#include "primitives/stringobject.h"

#include "Buffer.hpp"
#include "Strings.hpp"

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