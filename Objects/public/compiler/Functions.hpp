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

		void EvalExpr(Expression* expr);
		void EvalStatement(Statement* expr);

		ConstObject* defineLocal(const std::string& id);
	};

	void initialize();
	void finalize();
	void Generate(ByteCode& out, StatementScope* body);
	bool Compile(MethodObject* obj);
}