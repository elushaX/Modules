#include "compiler/Constants.hpp"
#include "primitives/MethodObject.hpp"
#include "primitives/PrimitiveObjects.hpp"

using namespace tp;
using namespace obj;

ConstObject::ConstObject() = default;
ConstObject::ConstObject(Object* mObj) :
	mObj(mObj) {}

ConstObjectsPool::~ConstObjectsPool() {
	if (mDelete) {
		for (auto obj : mStrings) {
			objects_api::destroy(obj->val->mObj);
		}
		for (auto obj : mIntegers) {
			objects_api::destroy(obj->val->mObj);
		}
		for (auto obj : mFloats) {
			objects_api::destroy(obj->val->mObj);
		}
		for (auto obj : mMethods) {
			objects_api::destroy(obj->val->mObj);
		}
		if (mBoolFalse.mObj) {
			objects_api::destroy(mBoolFalse.mObj);
		}
		if (mBoolTrue.mObj) {
			objects_api::destroy(mBoolTrue.mObj);
		}
	}
}

ConstObject* ConstObjectsPool::registerObject(Object* obj) {
	auto out = new ConstObject(obj);
	mTotalObjects++;
	return out;
}

ConstObject* ConstObjectsPool::get(alni val) {
	auto idx = mIntegers.presents(val);
	ConstObject* const_obj;
	if (idx) {
		const_obj = mIntegers.getSlotVal(idx);
	} else {
		const_obj = registerObject(IntObject::create(val));
		mIntegers.put(val, const_obj);
	}
	return const_obj;
}

ConstObject* ConstObjectsPool::get(const std::string& val) {
	auto idx = mStrings.presents(val);
	ConstObject* const_obj;
	if (idx) {
		const_obj = mStrings.getSlotVal(idx);
	} else {
		const_obj = registerObject(StringObject::create(val));
		mStrings.put(val, const_obj);
	}
	return const_obj;
}

ConstObject* ConstObjectsPool::get(alnf val) {
	auto idx = mFloats.presents(val);
	ConstObject* const_obj;
	if (idx) {
		const_obj = mFloats.getSlotVal(idx);
	} else {
		const_obj = registerObject(FloatObject::create(val));
		mFloats.put(val, const_obj);
	}
	return const_obj;
}

ConstObject* ConstObjectsPool::get(bool val) {
	if (val) {
		if (!mBoolTrue.mObj) {
			mBoolTrue.mObj = BoolObject::create(val);
			mTotalObjects++;
		}
		return &mBoolTrue;
	} else {
		if (!mBoolFalse.mObj) {
			mBoolFalse.mObj = BoolObject::create(val);
			mTotalObjects++;
		}
		return &mBoolFalse;
	}
}

ConstObject* ConstObjectsPool::addMethod(const std::string& method_id, Object* method) {
	ASSERT(objects_api::cast<MethodObject>(method) && "Object is not a method object")
	ASSERT(!mMethods.presents(method_id) && "Method Redefinition")
	auto out = registerObject(method);
	mMethods.put(method_id, out);
	return out;
}

void ConstObjectsPool::save(Buffer<ConstData>& out) {
	out.reserve(mTotalObjects);
	alni data_idx = 0;
	for (auto obj : mMethods) {
		out[data_idx] = obj->val->mObj;
		obj->val->mConstIdx = data_idx;
		data_idx++;
	}
	for (auto obj : mStrings) {
		out[data_idx] = obj->val->mObj;
		obj->val->mConstIdx = data_idx;
		data_idx++;
	}
	for (auto obj : mIntegers) {
		out[data_idx] = obj->val->mObj;
		obj->val->mConstIdx = data_idx;
		data_idx++;
	}
	for (auto obj : mFloats) {
		out[data_idx] = obj->val->mObj;
		obj->val->mConstIdx = data_idx;
		data_idx++;
	}
	if (mBoolFalse.mObj) {
		out[data_idx] = mBoolFalse.mObj;
		mBoolFalse.mConstIdx = data_idx;
		data_idx++;
	}
	if (mBoolTrue.mObj) {
		out[data_idx] = mBoolTrue.mObj;
		mBoolTrue.mConstIdx = data_idx;
	}
	mDelete = false;
}
