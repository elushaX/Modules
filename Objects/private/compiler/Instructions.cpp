
#include "compiler/Instructions.hpp"

using namespace tp;
using namespace obj;

Instruction::Instruction(OpCode op) :
	mOp(op),
	mArgType(ArgType::NO_ARG),
	mInstType(InstType::EXEC) {}

Instruction::Instruction() :
	mInstType(InstType::NONE) {}

Instruction::Instruction(ConstObject* constData) :
	mConstData(constData),
	mInstType(InstType::PURE_CONST) {}

Instruction::Instruction(OpCode op, ConstObject* constData) :
	mOp(op),
	mConstData(constData),
	mArgType(ArgType::CONST_ARG),
	mInstType(InstType::EXEC) {}

Instruction::Instruction(OpCode op, ConstObject* constData, ConstObject* constData2) :
	mOp(op),
	mConstData(constData),
	mConstData2(constData2),
	mArgType(ArgType::CONST_ARG),
	mInstType(InstType::EXEC) {}

Instruction::Instruction(OpCode op, tp::alni param, tp::alni nBytes) :
	mOp(op),
	mParam(param),
	mArgType(ArgType::PARAM),
	mParamBytes(nBytes),
	mInstType(InstType::EXEC) {}

Instruction::Instruction(Instruction* inst, InstType jump_type) :
	mInstTarget(inst) {
	mInstType = jump_type;
}
