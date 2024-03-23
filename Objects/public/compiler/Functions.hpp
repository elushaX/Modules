#pragma once

#include "List.hpp"
#include "Map.hpp"

#include "Instructions.hpp"
#include "Statements.hpp"
#include "Constants.hpp"

#include <string>

namespace tp::obj {
	struct MethodObject;

	struct FunctionDefinition {

		FunctionDefinition* mPrnt = nullptr;

		// signature
		std::string mFunctionId;
		List<ConstObject*> mArgsOrder;

		ConstObjectsPool mConstants;
		Map<std::string, ConstObject*> mLocals;
		List<Instruction> mInstructions;

		FunctionDefinition(const std::string& function_id, const Buffer<std::string>& args, FunctionDefinition* prnt);
		FunctionDefinition() {}

		void generateByteCode(ByteCode& out);

		List<Instruction>::Node* inst(Instruction inst);

		void EvalExpr(ObjectsContext* context, Expression* expr);
		void EvalStatement(ObjectsContext* context, Statement* expr);

		ConstObject* defineLocal(const std::string& id);

		static void generate(ObjectsContext* context, ByteCode& out, StatementScope* body);
		static bool compile(ObjectsContext* context, MethodObject* obj);
	};
}