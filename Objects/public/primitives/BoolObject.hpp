
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct BoolObject : Object {
		alni val;

		static ObjectType TypeData;
		static void constructor(ObjectsContext* context, BoolObject* self);
		static void copy(ObjectsContext* context, BoolObject* self, const BoolObject* in);
		
		BoolObject* set(bool in);

		static void from_int(BoolObject* self, alni in);
		static void from_float(BoolObject* self, alnf in);
		static void from_string(BoolObject* self, const std::string& in);
		static std::string to_string(BoolObject* self);
		static alni to_int(BoolObject* self);
		static alnf to_float(BoolObject* self);
	};
}