#pragma once

#include "OperandsStack.hpp"
#include "ScopeStack.hpp"
#include "CallStack.hpp"

namespace tp::obj {
	struct Interpreter {
		OperandStack mOperandsStack;
		ScopeStack mScopeStack;
		CallStack mCallStack;

		Object* mLastParent = nullptr;
		bool mIsTypeMethod = false;
		const TypeMethod* mTypeMethod = nullptr;

		typedef struct { Object* obj; std::string id; } GlobalDef;

		void exec(MethodObject* method, ClassObject* self = nullptr, DictObject* globals = nullptr, InitialierList<GlobalDef> globals2 = {});

		void stepBytecode();
		void stepBytecodeIn();
		void stepBytecodeOut();

		bool finished();

		void execAll(MethodObject* method, ClassObject* self = nullptr, DictObject* globals = nullptr, InitialierList<GlobalDef> globals2 = {});
	};
}