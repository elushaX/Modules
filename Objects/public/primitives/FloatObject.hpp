
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct FloatObject : Object {
		alnf val;

		static ObjectType TypeData;
		static ObjectTypeAriphmetics TypeAriphm;

		FloatObject* set(alnf in);

		static void constructor(ObjectsContext* context, FloatObject* self);
		static void copy(ObjectsContext* context, FloatObject* self, const FloatObject* in);

		static void from_int(FloatObject* self, alni in);
		static void from_float(FloatObject* self, alnf in);
		static void from_string(FloatObject* self, const std::string& in);
		static std::string to_string(FloatObject* self);
		static alni to_int(FloatObject* self);
		static alnf to_float(FloatObject* self);
	};
}