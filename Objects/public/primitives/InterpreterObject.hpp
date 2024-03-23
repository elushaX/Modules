#pragma once

#include "interpreter/Interpreter.hpp"
#include "primitives/ClassObject.hpp"

namespace tp::obj {
	struct InterpreterObject : ClassObject {
		static ObjectType TypeData;
		Interpreter mInterpreter;

		static void destructor(InterpreterObject* self);
		static void constructor(InterpreterObject* self);
		static void load(ArchiverIn& file_self, InterpreterObject* self);

		void exec(obj::ClassObject* self = nullptr, InitialierList<Interpreter::GlobalDef> globals = {});
		void debug();
		bool running();
	};
}