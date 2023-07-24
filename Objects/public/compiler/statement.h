
#pragma once

#include "expression.h"

namespace obj {
	namespace BCgen {
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

			Statement() {}
			Statement(Type type);
		};

		struct StatementScope : public Statement {
			tp::Buffer<Statement*> mStatements;
			bool mPushToScopeStack = false;

			StatementScope(tp::init_list<Statement*> statements, bool aPushToScopeStack);
		};

		struct StatementFuncDef : public Statement {
			tp::Buffer<tp::String> mArgs;
			tp::String mFunctionId;
			tp::Buffer<Statement*> mStatements;

			StatementFuncDef(tp::String function_id, tp::init_list<tp::String> args, tp::init_list<Statement*> statements);
		};

		struct StatementLocalDef : public Statement {
			tp::String mLocalId;
			Expression* mNewExpr = NULL;
			ExpressionConst* mConstExpr = NULL;
			bool mIsConstExpr = false;

			StatementLocalDef(tp::String id, Expression* value);
			StatementLocalDef(tp::String id, ExpressionConst* value);
		};

		struct StatementCopy : public Statement {
			Expression* mLeft = NULL;
			Expression* mRight = NULL;

			StatementCopy(Expression* left, Expression* right);
		};

		struct StatementReturn : public Statement {
			Expression* mRet = NULL;

			StatementReturn(Expression* ret);
			StatementReturn();
		};

		struct StatementPrint : public Statement {
			Expression* mTarget = NULL;

			StatementPrint(Expression* mTarget);
		};

		struct StatementIgnore : public Statement {
			Expression* mExpr = NULL;

			StatementIgnore(Expression* expr);
		};

		struct StatementIf : public Statement {
			Expression* mCondition = NULL;
			StatementScope* mOnTrue = NULL;
			StatementScope* mOnFalse = NULL;

			StatementIf(Expression* condition, StatementScope* on_true, StatementScope* on_false);
		};

		struct StatementWhile : public Statement {
			Expression* mCondition = NULL;
			StatementScope* mScope = NULL;

			StatementWhile(Expression* condition, StatementScope* scope);
		};

		struct StatementClassDef : public Statement {
			tp::String mClassId;
			StatementScope* mScope = NULL;

			StatementClassDef(tp::String class_id, StatementScope* scope);
		};

		// Helpers
		StatementFuncDef* StmDefFunc(tp::String id, tp::init_list<tp::String> args, tp::init_list<Statement*> stms);
		StatementLocalDef* StmDefLocal(tp::String id, Expression* value);
		StatementCopy* StmCopy(Expression* left, Expression* right);
		StatementPrint* StmPrint(Expression* target);
		StatementReturn* StmReturn(Expression* obj);
		StatementReturn* StmReturn();
		StatementIf* StmIf(Expression* condition, StatementScope* on_true, StatementScope* on_false);
		StatementScope* StmScope(tp::init_list<Statement*> statements, bool aPushToScopeStack);
		StatementWhile* StmWhile(Expression* condition, StatementScope* scope);
		StatementIgnore* StmIgnore(Expression* expr);
		StatementClassDef* StmClassDef(tp::String id, StatementScope* scope);
	};
};