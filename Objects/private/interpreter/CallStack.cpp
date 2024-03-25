
#include "interpreter/CallStack.hpp"

#include "interpreter/ByteCode.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

void CallStack::enter(const CallStack::CallFrame& frame) {
	if (mStack.size()) {
		auto& last_frame = mStack.last();
		last_frame.mIp = last_frame.mMethod->mBytecodeLink->mBytecode.mInstructionIdx;
	}

	frame.mMethod->mBytecodeLink->mBytecode.mArgumentsLoaded = 0;
	frame.mMethod->mBytecodeLink->mBytecode.mInstructionIdx = 0;
	objects_api::increaseReferenceCount(frame.mMethod);
	mStack.append(frame);
}

void CallStack::leave() {
	auto frame = mStack.last();
	objects_api::destroy(frame.mMethod);
	mStack.pop();

	if (mStack.size()) {
		auto& last_frame = mStack.last();
		last_frame.mMethod->mBytecodeLink->mBytecode.mInstructionIdx = last_frame.mIp;
	}
}

ByteCode* CallStack::getBytecode() { return &mStack.last().mMethod->mBytecodeLink->mBytecode; }

halni CallStack::len() const { return mStack.size(); }
