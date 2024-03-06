#include "compiler/statement.h"

using namespace obj;
using namespace BCgen;

StatementFuncDef::StatementFuncDef(const tp::String& function_id) :
	Statement(Type::DEF_FUNC) {
	mFunctionId = function_id;
}

StatementFuncDef::~StatementFuncDef() { delete mStatements; }

StatementLocalDef::StatementLocalDef(const tp::String& id, Expression* value) :
	mLocalId(id),
	Statement(Type::DEF_LOCAL) {

	if (value->mType == Expression::Type::CONST_EXPR) {
		mIsConstExpr = true;
		mConstExpr = (ExpressionConst*) value;
	} else {
		mIsConstExpr = false;
		mNewExpr = value;
	}
}

StatementLocalDef::~StatementLocalDef() {
	delete mNewExpr;
	delete mConstExpr;
}

StatementCopy::StatementCopy(Expression* left, Expression* right) :
	mLeft(left),
	mRight(right),
	Statement(Type::COPY) {}

StatementCopy::~StatementCopy() {
	delete mLeft;
	delete mRight;
}

StatementReturn::StatementReturn() :
	Statement(Type::RET) {}

StatementReturn::StatementReturn(Expression* ret) :
	mRet(ret),
	Statement(Type::RET) {}

StatementReturn::~StatementReturn() { delete mRet; }

StatementPrint::StatementPrint(Expression* target) :
	mTarget(target),
	Statement(Type::PRINT) {}

StatementPrint::~StatementPrint() { delete mTarget; }

StatementScope::StatementScope(tp::InitialierList<Statement*> statements, bool aPushToScopeStack) :
	Statement(Type::SCOPE) {
	mStatements = statements;
	mPushToScopeStack = aPushToScopeStack;
}

StatementScope::~StatementScope() {
	for (auto iter : mStatements) {
		delete iter.data();
	}
}

StatementIf::StatementIf(Expression* condition, StatementScope* on_true, StatementScope* on_false) :
	Statement(Type::IF) {
	mOnFalse = on_false;
	mOnTrue = on_true;
	mCondition = condition;
}

StatementIf::~StatementIf() {
	delete mCondition;
	delete mOnTrue;
	delete mOnFalse;
}

StatementWhile::StatementWhile(Expression* condition, StatementScope* scope) :
	Statement(Type::WHILE) {
	mScope = scope;
	mCondition = condition;
}

StatementWhile::~StatementWhile() {
	delete mCondition;
	delete mScope;
}

StatementIgnore::StatementIgnore(Expression* expr) :
	Statement(Type::IGNORE) {
	mExpr = expr;
}

StatementIgnore::~StatementIgnore() { delete mExpr; }

StatementClassDef::StatementClassDef(const tp::String& class_id, StatementScope* scope) :
	Statement(Type::CLASS_DEF) {
	mClassId = class_id;
	mScope = scope;
}

StatementClassDef::~StatementClassDef() { delete mScope; }

Statement::Statement(Statement::Type type) { mType = type; }
