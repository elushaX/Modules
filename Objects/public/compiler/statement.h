
#pragma once

#include "expression.h"

namespace obj::BCgen {
	struct Statement {

		enum class Type {
			NONE,
			SCOPE,
			DEF_FUNC,
			DEF_LOCAL,
			RET,
			PRINT,
			COPY,
			IF,
			WHILE,
			IGNORE,
			CALL,
			CLASS_DEF,
		} mType = Type::NONE;

		bool mValueUsed = false;

		explicit Statement(Type type);
		virtual ~Statement() = default;
	};

	struct StatementScope : public Statement {
		tp::Buffer<Statement*> mStatements;
		bool mPushToScopeStack = false;

		StatementScope(tp::InitialierList<Statement*> statements, bool aPushToScopeStack);
		~StatementScope() override;
	};

	struct StatementFuncDef : public Statement {
		tp::Buffer<tp::String> mArgs;
		tp::String mFunctionId;
		StatementScope* mStatements = nullptr;

		explicit StatementFuncDef(const tp::String& function_id);
		~StatementFuncDef() override;
	};

	struct StatementLocalDef : public Statement {
		tp::String mLocalId;
		Expression* mNewExpr = nullptr;
		ExpressionConst* mConstExpr = nullptr;
		bool mIsConstExpr = false;

		StatementLocalDef(const tp::String& id, Expression* value);
		~StatementLocalDef() override;
	};

	struct StatementCopy : public Statement {
		Expression* mLeft = nullptr;
		Expression* mRight = nullptr;

		StatementCopy(Expression* left, Expression* right);
		~StatementCopy() override;
	};

	struct StatementReturn : public Statement {
		Expression* mRet = nullptr;

		explicit StatementReturn(Expression* ret);
		StatementReturn();
		~StatementReturn() override;
	};

	struct StatementPrint : public Statement {
		Expression* mTarget = nullptr;

		explicit StatementPrint(Expression* mTarget);
		~StatementPrint() override;
	};

	struct StatementIgnore : public Statement {
		Expression* mExpr = nullptr;

		explicit StatementIgnore(Expression* expr);
		~StatementIgnore() override;
	};

	struct StatementIf : public Statement {
		Expression* mCondition = nullptr;
		StatementScope* mOnTrue = nullptr;
		StatementScope* mOnFalse = nullptr;

		StatementIf(Expression* condition, StatementScope* on_true, StatementScope* on_false);
		~StatementIf() override;
	};

	struct StatementWhile : public Statement {
		Expression* mCondition = nullptr;
		StatementScope* mScope = nullptr;

		StatementWhile(Expression* condition, StatementScope* scope);
		~StatementWhile() override;
	};

	struct StatementClassDef : public Statement {
		tp::String mClassId;
		StatementScope* mScope = nullptr;

		StatementClassDef(const tp::String& class_id, StatementScope* scope);
		~StatementClassDef() override;
	};
}