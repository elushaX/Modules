
#pragma once

#include "interpreter/bytecode.h"
#include "core/object.h"

namespace obj {
	namespace BCgen {

		class ConstObject {
		public:
			obj::Object* mObj = NULL;
			tp::alni mConstIdx = 0;

			ConstObject();
			ConstObject(obj::Object* mObj);
		};


		class ConstObjectsPool {
		public:
			tp::Map<tp::String, ConstObject*> mMethods;
			tp::Map<tp::String, ConstObject*> mStrings;
			tp::Map<tp::alni, ConstObject*> mIntegers;
			tp::Map<tp::alnf, ConstObject*> mFloats;
			ConstObject mBoolTrue;
			ConstObject mBoolFalse;

			bool mDelete = true;
			tp::alni mTotalObjects = 0;

			ConstObject* get(tp::alni val);
			ConstObject* get(tp::String val);
			ConstObject* get(tp::alnf val);
			ConstObject* get(bool val);

			ConstObject* addMethod(tp::String method_id, obj::Object* method);
			ConstObject* registerObject(obj::Object* obj);
			void save(tp::Buffer<ConstData>& out);

			ConstObjectsPool() = default;
			~ConstObjectsPool();
		};
	};
};