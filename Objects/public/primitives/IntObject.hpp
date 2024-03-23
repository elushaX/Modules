
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct IntObject : Object {
		alni val;

		static ObjectType TypeData;
		static ObjectTypeAriphmetics TypeAriphm;

		static void constructor(Object* self);
		static void copy(IntObject* self, const IntObject* in);
		static IntObject* create(alni in);

		static void from_int(Object* self, alni in);
		static void from_float(Object* self, alnf in);
		static void from_string(Object* self, const std::string& in);
		static std::string to_string(Object* self);
		static alni to_int(Object* self);
		static alnf to_float(Object* self);
	};
}