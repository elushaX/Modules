#pragma once

#include "core/Object.hpp"
#include "interpreter/ByteCode.hpp"

namespace tp::obj {

	class ConstObject {
	public:
		Object* mObj = nullptr;
		alni mConstIdx = 0;

		ConstObject();
		ConstObject(Object* mObj);
	};


	class ConstObjectsPool {
	public:
		Map<std::string, ConstObject*> mMethods;
		Map<std::string, ConstObject*> mStrings;
		Map<alni, ConstObject*> mIntegers;
		Map<alnf, ConstObject*> mFloats;
		ConstObject mBoolTrue;
		ConstObject mBoolFalse;

		bool mDelete = true;
		alni mTotalObjects = 0;

		ConstObject* get(alni val);
		ConstObject* get(const std::string& val);
		ConstObject* get(alnf val);
		ConstObject* get(bool val);

		ConstObject* addMethod(const std::string& method_id, Object* method);
		ConstObject* registerObject(Object* obj);
		void save(Buffer<ConstData>& out);

		ConstObjectsPool() = default;
		~ConstObjectsPool();
	};
}