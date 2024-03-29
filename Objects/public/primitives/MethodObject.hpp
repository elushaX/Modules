#pragma once

#include "primitives/BytecodeObject.hpp"

namespace tp::obj {

	struct MethodObject : Object {

		static ObjectType TypeData;

		BytecodeObject* mBytecodeLink;

		static void constructor(MethodObject* self);
		static void copy(MethodObject* self, MethodObject* in);
		static void destructor(MethodObject* self);
		static alni save_size(MethodObject* self);
		static void save(MethodObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, obj::MethodObject* self);

		void compile();

		static MethodObject* create(const std::string& script);
	};
}