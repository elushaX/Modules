#pragma once

#include "primitives/StringObject.hpp"
#include "interpreter/ByteCode.hpp"

namespace tp::obj {

	struct BytecodeObject : Object {

		static ObjectType TypeData;

		StringObject* mReadable;
		ByteCode mBytecode;

		static void constructor(BytecodeObject* self);
		static void copy(BytecodeObject* self, BytecodeObject* in);
		static void destructor(BytecodeObject* self);

		static alni save_size(BytecodeObject* self);
		static void save(BytecodeObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, BytecodeObject* self);
	};
}