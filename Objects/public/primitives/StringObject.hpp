
#pragma once

#include "core/Object.hpp"

namespace obj {

	struct StringObject : Object {
		std::string val;

		static ObjectType TypeData;
		static void constructor(Object* self);
		static void destructor(StringObject* self);
		static void copy(Object* self, const Object* in);
		static StringObject* create(const std::string& in);

		static void from_int(StringObject* self, tp::alni in);
		static void from_float(StringObject* self, tp::alnf in);
		static void from_string(StringObject* self, const std::string& in);
		static std::string to_string(StringObject* self);
		static tp::alni to_int(StringObject* self);
		static tp::alnf to_float(StringObject* self);
	};
};