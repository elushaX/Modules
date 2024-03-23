
#pragma once

#include "interpreter/ByteCode.hpp"
#include "core/Object.hpp"

namespace obj {
	namespace BCgen {

		class ConstObject {
		public:
			obj::Object* mObj = nullptr;
			tp::alni mConstIdx = 0;

			ConstObject();
			ConstObject(obj::Object* mObj);
		};


		class ConstObjectsPool {
		public:
			tp::Map<std::string, ConstObject*> mMethods;
			tp::Map<std::string, ConstObject*> mStrings;
			tp::Map<tp::alni, ConstObject*> mIntegers;
			tp::Map<tp::alnf, ConstObject*> mFloats;
			ConstObject mBoolTrue;
			ConstObject mBoolFalse;

			bool mDelete = true;
			tp::alni mTotalObjects = 0;

			ConstObject* get(tp::alni val);
			ConstObject* get(const std::string& val);
			ConstObject* get(tp::alnf val);
			ConstObject* get(bool val);

			ConstObject* addMethod(const std::string& method_id, obj::Object* method);
			ConstObject* registerObject(obj::Object* obj);
			void save(tp::Buffer<ConstData>& out);

			ConstObjectsPool() = default;
			~ConstObjectsPool();
		};
	};
};