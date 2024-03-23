#pragma once

#include "primitives/StringObject.hpp"
#include "interpreter/Script.hpp"

namespace obj {

	struct MethodObject : Object {

		static ObjectType TypeData;

		Script* mScript;

		static void constructor(MethodObject* self);
		static void copy(MethodObject* self, MethodObject* in);
		static void destructor(MethodObject* self);
		static tp::alni save_size(MethodObject* self);
		static void save(MethodObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, obj::MethodObject* self);

		static void Initialize();
		static void UnInitialize();

		void compile();

		static MethodObject* create(const std::string& script);
	};
};