#include "compiler/Expressions.hpp"

#include <utility>

using namespace tp;
using namespace obj;

Expression::Expression(Type type) :
	mType(type) {}

ExpressionChild* Expression::ExprChild(const std::string& id) { return new ExpressionChild(this, id); }

ExpressionCall* Expression::ExprCall(ExpressionList* args) { return new ExpressionCall(this, args); }

ExpressionBoolean::ExpressionBoolean(Expression* left, Expression* right, BoolType type) :
	Expression(Type::BOOLEAN),
	mLeft(left),
	mRight(right),
	mBoolType(type) {}

ExpressionBoolean::ExpressionBoolean(Expression* invert) :
	Expression(Type::BOOLEAN),
	mLeft(invert),
	mBoolType(BoolType::NOT) {}

ExpressionBoolean::~ExpressionBoolean() {
	delete mLeft;
	delete mRight;
}

ExpressionCall::ExpressionCall(Expression* mParent, ExpressionList* args) :
	Expression(Type::CALL),
	mParent(mParent) {
	mArgs = args;
}

ExpressionCall::~ExpressionCall() {
	delete mParent;
	delete mArgs;
}

ExpressionList::ExpressionList() :
	Expression(Type::LIST) {}

ExpressionList::~ExpressionList() {
	for (auto item : mItems) {
		delete item.data();
	}
}

ExpressionNew::ExpressionNew(std::string  type) :
	Expression(Type::NEW),
	mNewType(std::move(type)) {}

ExpressionNew::~ExpressionNew() = default;

ExpressionLocal::ExpressionLocal(std::string  id) :
	Expression(Type::LOCAL),
	mLocalId(std::move(id)) {}

ExpressionLocal::~ExpressionLocal() = default;

ExpressionSelf::ExpressionSelf() :
	Expression(Type::SELF) {}

ExpressionChild::ExpressionChild(Expression* mParent, std::string  id) :
	Expression(Type::CHILD),
	mParent(mParent),
	mLocalId(std::move(id)) {}

ExpressionChild::~ExpressionChild() { delete mParent; }

ExpressionArithmetics::ExpressionArithmetics(Expression* left, Expression* right, OpCode type) :
	Expression(Type::ARITHMETICS),
	mLeft(left),
	mRight(right),
	mOpType(type) {}

ExpressionArithmetics::~ExpressionArithmetics() {
	delete mLeft;
	delete mRight;
}

ExpressionFunc::ExpressionFunc(std::string  id) :
	Expression(Type::FUNC),
	mFuncId(std::move(id)) {}

ExpressionFunc::~ExpressionFunc() = default;

ExpressionConst::ExpressionConst(std::string  val) :
	Expression(Type::CONST_EXPR),
	mConstType(STR),
	str(std::move(val)) {}

ExpressionConst::ExpressionConst(const char* val) :
	Expression(Type::CONST_EXPR),
	mConstType(STR),
	str(val) {}

ExpressionConst::ExpressionConst(int4 val) :
	Expression(Type::CONST_EXPR),
	mConstType(INT),
	integer(val) {}

ExpressionConst::ExpressionConst(flt4 val) :
	Expression(Type::CONST_EXPR),
	mConstType(FLT),
	floating(val) {}

ExpressionConst::ExpressionConst(alni val) :
	Expression(Type::CONST_EXPR),
	mConstType(INT),
	integer(val) {}

ExpressionConst::ExpressionConst(alnf val) :
	Expression(Type::CONST_EXPR),
	mConstType(FLT),
	floating(val) {}

ExpressionConst::ExpressionConst(bool val) :
	Expression(Type::CONST_EXPR),
	mConstType(BOOL),
	boolean(val) {}

ExpressionConst::~ExpressionConst() = default;
