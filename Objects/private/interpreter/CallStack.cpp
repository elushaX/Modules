
#include "interpreter/CallStack.hpp"

#include "interpreter/ByteCode.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

void CallStack::enter(const CallStack::CallFrame& frame) {
	if (mStack.size()) {
		auto& last_frame = mStack.last();
		last_frame.mIp = last_frame.mMethod->mScript->mBytecode.mInstructionIdx;
	}

	frame.mMethod->mScript->mBytecode.mArgumentsLoaded = 0;
	frame.mMethod->mScript->mBytecode.mInstructionIdx = 0;
	obj::NDO->increaseReferenceCount(frame.mMethod);
	mStack.append(frame);
}

void CallStack::leave() {
	auto frame = mStack.last();
	obj::NDO->destroy(frame.mMethod);
	mStack.pop();

	if (mStack.size()) {
		auto& last_frame = mStack.last();
		last_frame.mMethod->mScript->mBytecode.mInstructionIdx = last_frame.mIp;
	}
}

ByteCode* CallStack::getBytecode() { return &mStack.last().mMethod->mScript->mBytecode; }

halni CallStack::len() const { return mStack.size(); }