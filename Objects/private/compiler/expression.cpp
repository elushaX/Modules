#include "NewPlacement.hpp"
#include "compiler/expression.h"

using namespace obj;
using namespace BCgen;

Expression::Expression() {}
Expression::Expression(Type type) : mType(type) {}

ExpressionChild* Expression::ExprChild(tp::String id) {
	return new ExpressionChild(this, id);
}

ExpressionCall* Expression::ExprCall(tp::InitialierList<Expression*> args) {
	return new ExpressionCall(this, args);
}

ExpressionLocal* obj::BCgen::ExprLocal(tp::String id) {
	return new ExpressionLocal(id);
}

ExpressionSelf* obj::BCgen::ExprSelf() {
	return new ExpressionSelf();
}

ExpressionNew* obj::BCgen::ExprNew(tp::String type) {
	return new ExpressionNew(type);
}

ExpressionAriphm* obj::BCgen::ExprAriphm(Expression* left, Expression* right, OpCode type) {
	return new ExpressionAriphm(left, right, type);
}

ExpressionFunc* obj::BCgen::ExprFunc(tp::String id) {
	return new ExpressionFunc(id);
}

ExpressionBoolean* obj::BCgen::ExprBool(Expression* left, Expression* right, obj::OpCode type) {
	return new ExpressionBoolean(left, right, ExpressionBoolean::BoolType(type));
}

ExpressionBoolean* obj::BCgen::ExprBoolNot(Expression* invert) {
	return new ExpressionBoolean(invert);
}

ExpressionBoolean::ExpressionBoolean(Expression* left, Expression* right, BoolType type) 
: Expression(Type::BOOLEAN), mLeft(left), mRight(right), mBoolType(type) {
}

ExpressionBoolean::ExpressionBoolean(Expression* invert)
	: Expression(Type::BOOLEAN), mLeft(invert), mBoolType(BoolType::NOT){
}

ExpressionCall::ExpressionCall(Expression * mParent, tp::InitialierList<Expression*> args) : Expression(Type::CALL), mParent(mParent) {
	mArgs = args;
}

ExpressionNew::ExpressionNew(tp::String type) : Expression(Type::NEW), mNewType(type) {}
ExpressionLocal::ExpressionLocal(tp::String id) : Expression(Type::LOCAL), mLocalId(id) {}
ExpressionSelf::ExpressionSelf() : Expression(Type::SELF) {}
ExpressionChild::ExpressionChild(Expression* mParent, tp::String id) : Expression(Type::CHILD), mParent(mParent), mLocalId(id) {}
ExpressionAriphm::ExpressionAriphm(Expression* left, Expression* right, OpCode type) : Expression(Type::ARIPHM), mLeft(left), mRight(right), mOpType(type) {}
ExpressionFunc::ExpressionFunc(tp::String id) : Expression(Type::FUNC), mFuncId(id) {}
ExpressionConst::ExpressionConst(tp::String val) : Expression(Type::CONST), mConstType(STR), str(val) {}
ExpressionConst::ExpressionConst(const char* val) : Expression(Type::CONST), mConstType(STR), str(val) {}
ExpressionConst::ExpressionConst(tp::int4 val) : Expression(Type::CONST), mConstType(INT), integer(val) {}
ExpressionConst::ExpressionConst(tp::flt4 val) : Expression(Type::CONST), mConstType(FLT), floating(val) {}
ExpressionConst::ExpressionConst(tp::alni val) : Expression(Type::CONST), mConstType(INT), integer(val) {}
ExpressionConst::ExpressionConst(tp::alnf val) : Expression(Type::CONST), mConstType(FLT), floating(val) {}
ExpressionConst::ExpressionConst(bool val) : Expression(Type::CONST), mConstType(BOOL), boolean(val) {}
