
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct StringObject : Object {
		std::string val;

		static ObjectType TypeData;
		static void constructor(StringObject* self);
		static void destructor(StringObject* self);
		static void copy(StringObject* self, const StringObject* in);

		static StringObject* create(const std::string& in);

		static void from_int(StringObject* self, alni in);
		static void from_float(StringObject* self, alnf in);
		static void from_string(StringObject* self, const std::string& in);
		static std::string to_string(StringObject* self);
		static alni to_int(StringObject* self);
		static alnf to_float(StringObject* self);
	};
}