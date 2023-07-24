
#pragma once

#include "Strings.hpp"
#include "List.hpp"
#include "Map.hpp"

#include "instruction.h"
#include "statement.h"
#include "constants.h"

namespace obj {
	struct MethodObject;

	namespace BCgen {

		struct FunctionDefinition {
			
			FunctionDefinition* mPrnt = NULL;

			// signature
			tp::String mFunctionId;
			tp::List<ConstObject*> mArgsOrder;

			ConstObjectsPool mConstants;
			tp::Map<tp::String, ConstObject*> mLocals;
			tp::List<Instruction> mInstructions;

			FunctionDefinition(tp::String function_id, tp::Buffer<tp::String> args, FunctionDefinition* prnt);
			FunctionDefinition() {}

			void generateByteCode(ByteCode& out);

			tp::List<Instruction>::Node* inst(Instruction inst);
			
			void EvalExpr(Expression* expr);
			void EvalStatement(Statement* expr);

			ConstObject* defineLocal(tp::String id);
		};

		void init();
		void deinit();
		void Genereate(ByteCode& out, StatementScope* body);
		bool Compile(MethodObject* obj);
	};
};