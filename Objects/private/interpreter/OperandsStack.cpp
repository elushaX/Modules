
#include "interpreter/OperandsStack.hpp"

using namespace obj;

OperandStack::OperandStack() {
	mBuff = new Operand[MAX_STACK_SIZE];
	mIdx = 0;
}

void OperandStack::push(Operand operand) {
	ASSERT(MAX_STACK_SIZE - 1 > mIdx && "stack overflow");
	mBuff[mIdx] = operand;
	mIdx++;
}

void OperandStack::pop() {
	ASSERT(mIdx != 0 && "stack overflow");
	mIdx--;
}

Operand OperandStack::getOperand() {
	auto ret = mBuff[mIdx - 1];
	mIdx--;
	return ret;
}

OperandStack::~OperandStack() { delete[] mBuff; }
