
#pragma once

#include <string>
#include "List.hpp"
#include "Map.hpp"

#include "Instructions.hpp"
#include "Statements.hpp"
#include "Constants.hpp"

namespace obj {
	struct MethodObject;

	namespace BCgen {

		struct FunctionDefinition {
			
			FunctionDefinition* mPrnt = nullptr;

			// signature
			std::string mFunctionId;
			tp::List<ConstObject*> mArgsOrder;

			ConstObjectsPool mConstants;
			tp::Map<std::string, ConstObject*> mLocals;
			tp::List<Instruction> mInstructions;

			FunctionDefinition(const std::string& function_id, tp::Buffer<std::string> args, FunctionDefinition* prnt);
			FunctionDefinition() {}

			void generateByteCode(ByteCode& out);

			tp::List<Instruction>::Node* inst(Instruction inst);
			
			void EvalExpr(Expression* expr);
			void EvalStatement(Statement* expr);

			ConstObject* defineLocal(const std::string& id);
		};

		void init();
		void deinit();
		void Genereate(ByteCode& out, StatementScope* body);
		bool Compile(MethodObject* obj);
	};
};