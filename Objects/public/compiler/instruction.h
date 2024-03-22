
#pragma once

#include "interpreter/opcodes.h"

#include "core/object.h"

#include "List.hpp"

namespace obj {
	namespace BCgen {

		class ConstObject;

		class Instruction {
		public:

			OpCode mOp = OpCode::NONE;

			enum class ArgType : tp::ualni {
				NO_ARG,
				PARAM,
				CONST_ARG,
			} mArgType = ArgType::NO_ARG;

			enum class InstType : tp::ualni {
				NONE,
				JUMP,
				JUMP_IF,
				JUMP_IF_NOT,
				EXEC,
				PURE_CONST,
			} mInstType = InstType::NONE;

			tp::alni mParam = 0;
			tp::alni mParamBytes = 1;

			ConstObject* mConstData = nullptr;
			ConstObject* mConstData2 = nullptr;

			tp::alni mInstIdx = 0;
			Instruction* mInstTarget = nullptr;

			Instruction();
			Instruction(ConstObject* constData);
			Instruction(OpCode op);
			Instruction(OpCode op, ConstObject* constData);
			Instruction(OpCode op, ConstObject* constData, ConstObject* constData2);
			Instruction(OpCode op, tp::alni param, tp::alni nBytes);
			Instruction(Instruction* inst, InstType jump_type);
		};
	};
};