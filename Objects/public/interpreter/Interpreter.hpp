#pragma once

#include "OperandsStack.hpp"
#include "ScopeStack.hpp"
#include "CallStack.hpp"

namespace obj {
	struct Interpreter {
		OperandStack mOperandsStack;
		ScopeStack mScopeStack;
		CallStack mCallStack;

		obj::Object* mLastParent = nullptr;
		bool mIsTypeMethod = false;
		const TypeMethod* mTypeMethod = nullptr;

		typedef struct { obj::Object* obj; std::string id; } GlobalDef;

		void exec(obj::MethodObject* method, obj::ClassObject* self = nullptr, obj::DictObject* globals = nullptr, tp::InitialierList<GlobalDef> globals2 = {});

		void stepBytecode();
		void stepBytecodeIn();
		void stepBytecodeOut();

		bool finished();

		void execAll(obj::MethodObject* method, obj::ClassObject* self = nullptr, obj::DictObject* globals = nullptr, tp::InitialierList<GlobalDef> globals2 = {});
	};
};