
#pragma once

#include "core/Object.hpp"
#include "Color.hpp"

namespace tp::obj {

	struct ColorObject : Object {
		RGBA mCol;

		static ObjectType TypeData;
		static ObjectTypeArithmetics TypeAriphm;

		static void constructor(ColorObject* self);
		static void copy(ColorObject* self, const ColorObject* in);

		static void from_int(ColorObject* self, alni in);
		static void from_float(ColorObject* self, alnf in);
		static std::string to_string(ColorObject* self);

		static ColorObject* create(RGBA in);
	};
}