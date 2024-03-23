
#pragma once

#include "core/Object.hpp"
#include "Color.hpp"

namespace tp::obj {

	struct ColorObject : Object {
		RGBA mCol;

		static ObjectType TypeData;
		static ObjectTypeAriphmetics TypeAriphm;

		static void constructor(ObjectsContext* context, Object* self);
		static void copy(ObjectsContext* context, ColorObject* self, const ColorObject* in);

		static void from_int(ColorObject* self, alni in);
		static void from_float(ColorObject* self, alnf in);
		static std::string to_string(ColorObject* self);

		ColorObject* set(RGBA in);
	};
}