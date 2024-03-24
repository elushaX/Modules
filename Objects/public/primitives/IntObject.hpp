
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct IntObject : Object {
		alni val;

		static ObjectType TypeData;
		static ObjectTypeAriphmetics TypeAriphm;

		static void constructor(IntObject* self);
		static void copy(IntObject* self, const IntObject* in);
		static IntObject* create(alni in);

		static void from_int(IntObject* self, alni in);
		static void from_float(IntObject* self, alnf in);
		static void from_string(IntObject* self, const std::string& in);
		static std::string to_string(IntObject* self);
		static alni to_int(IntObject* self);
		static alnf to_float(IntObject* self);
	};
}