
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct EnumObject : Object {

		// one entry is 2 * sizeof(alni) in size
		uhalni active;
		uhalni nentries;
		alni* entries;

		static ObjectType TypeData;
		static void constructor(EnumObject* self);
		static void destructor(EnumObject* self);
		static void copy(EnumObject* self, const EnumObject* in);

		void init(InitialierList<const char*> list);
		const char* getActiveName();
		const char* getItemName(uhalni idx);

		static void from_int(EnumObject* self, alni in);
		static void from_float(EnumObject* self, alnf in);
		static void from_string(EnumObject* self, const std::string& in);
		static std::string to_string(EnumObject* self);
		static alni to_int(EnumObject* self);
		static alnf to_float(EnumObject* self);

		static bool compare(EnumObject* first, EnumObject* second);
		static EnumObject* create(InitialierList<const char*> list);
	};
}