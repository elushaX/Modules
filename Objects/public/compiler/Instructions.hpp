
#pragma once

#include "interpreter/OperatoinCodes.hpp"
#include "core/Object.hpp"
#include "List.hpp"

namespace tp::obj {
	class ConstObject;

	class Instruction {
	public:

		OpCode mOp = OpCode::NONE;

		enum class ArgType : ualni {
			NO_ARG,
			PARAM,
			CONST_ARG,
		} mArgType = ArgType::NO_ARG;

		enum class InstType : ualni {
			NONE,
			JUMP,
			JUMP_IF,
			JUMP_IF_NOT,
			EXEC,
			PURE_CONST,
		} mInstType = InstType::NONE;

		alni mParam = 0;
		alni mParamBytes = 1;

		ConstObject* mConstData = nullptr;
		ConstObject* mConstData2 = nullptr;

		alni mInstIdx = 0;
		Instruction* mInstTarget = nullptr;

		Instruction();
		Instruction(ConstObject* constData);
		Instruction(OpCode op);
		Instruction(OpCode op, ConstObject* constData);
		Instruction(OpCode op, ConstObject* constData, ConstObject* constData2);
		Instruction(OpCode op, alni param, alni nBytes);
		Instruction(Instruction* inst, InstType jump_type);
	};
}
