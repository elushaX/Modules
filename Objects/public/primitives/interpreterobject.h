#pragma once

#include "interpreter/interpreter.h"
#include "primitives/classobject.h"

namespace obj {
	struct InterpreterObject : ClassObject {
		static ObjectType TypeData;
		Interpreter mInterpreter;

		static void destructor(InterpreterObject* self);
		static void constructor(InterpreterObject* self);
		static void load(ArchiverIn& file_self, InterpreterObject* self);

		void exec(obj::ClassObject* self = NULL, tp::InitialierList<Interpreter::GlobalDef> globals = {});
		void debug();
		bool running();
	};
};