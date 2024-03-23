#pragma once

#include "primitives/StringObject.hpp"
#include "interpreter/Script.hpp"

namespace tp::obj {

	struct MethodObject : Object {

		static ObjectType TypeData;

		Script* mScript;

		static void constructor(ObjectsContext* context, MethodObject* self);
		static void copy(ObjectsContext* context, MethodObject* self, MethodObject* in);
		static void destructor(ObjectsContext* context, MethodObject* self);
		static alni save_size(MethodObject* self);
		static void save(ObjectsContext* context, MethodObject* self, ArchiverOut& file_self);
		static void load(ObjectsContext* context, ArchiverIn& file_self, obj::MethodObject* self);

		static void Initialize();
		static void UnInitialize();

		void compile();

		static MethodObject* create(const std::string& script);
	};
}