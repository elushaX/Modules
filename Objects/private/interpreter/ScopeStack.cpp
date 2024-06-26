
#include "interpreter/ScopeStack.hpp"

using namespace tp;
using namespace obj;

Scope::~Scope() {
	for (auto local : mLocals) {
		objects_api::destroy(local->val);
	}
	for (auto tmp : mTemps) {
		objects_api::destroy(tmp.data());
	}
}

obj::Object* ScopeStack::getLocalUtil(Scope& scope, const std::string& id) {

	auto idx = scope.mLocals.presents(id);

	if (idx) {
		return scope.mLocals.getSlotVal(idx);
	} else {
		mIterIdx--;
		Scope& parent_scope = mBuff[mIterIdx];
		ASSERT(parent_scope.mChildReachable && "Undefined Local Reference");
		return getLocalUtil(parent_scope, id);
	}
}

ScopeStack::ScopeStack() {
	mBuff = (Scope*) malloc(sizeof(Scope) * MAX_STACK_SIZE);
	mIdx = 0;
	mIterIdx = 0;
}

void ScopeStack::enterScope(bool aChildReachable) {
	ASSERT(MAX_STACK_SIZE - 1 > mIdx && "stack overflow");
	new (&mBuff[mIdx]) Scope();
	mBuff[mIdx].mChildReachable = aChildReachable;
	mIdx++;
}

void ScopeStack::leaveScope() {
	ASSERT(mIdx != 0 && "stack overflow");
	mBuff[mIdx - 1].~Scope();
	mIdx--;
}

void ScopeStack::addTemp(obj::Object* tmp) {
	objects_api::increaseReferenceCount(tmp);
	mBuff[mIdx - 1].mTemps.pushBack(tmp);
}

void ScopeStack::popTemp() {
	objects_api::destroy(mBuff[mIdx - 1].mTemps.last());
	mBuff[mIdx - 1].mTemps.popBack();
}

void ScopeStack::addTempReturn(obj::Object* ret) {
	if (mIdx >= 2) {
		objects_api::increaseReferenceCount(ret);
		mBuff[mIdx - 2].mTemps.pushBack(ret);
	}
}

void ScopeStack::addLocal(obj::Object* local, const std::string& id) {
	DEBUG_ASSERT(mIdx != 0 && "No scope given");
	Scope::ObjDict& locals = mBuff[mIdx - 1].mLocals;
	auto idx = locals.presents(id);
	if (idx) {
		objects_api::destroy(locals.getSlotVal(idx));
	}
	objects_api::increaseReferenceCount(local);
	locals.put(id, local);
}

obj::Object* ScopeStack::getLocal(const std::string& str) {
	mIterIdx = mIdx - 1;
	return getLocalUtil(mBuff[mIdx - 1], str);
}

Scope* ScopeStack::getCurrentScope() { return &mBuff[mIdx - 1]; }

ScopeStack::~ScopeStack() { free(mBuff); }
