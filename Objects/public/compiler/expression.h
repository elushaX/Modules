
#pragma once

#include "Strings.hpp"
#include "Buffer.hpp"

#include "interpreter/opcodes.h"

namespace obj {
	namespace BCgen {

		struct ExpressionChild;
		struct ExpressionCall;

		struct Expression {
			
			enum class Type {
				NONE,
				NEW,
				LOCAL,
				CONST,
				CHILD,
				CALL,
				ARIPHM,
				FUNC,
				BOOLEAN,
				SELF,
			} mType = Type::NONE;

			bool mValueUsed = false;

			Expression();
			Expression(Type type);

			ExpressionChild* ExprChild(tp::String id);
			ExpressionCall* ExprCall(tp::InitialierList<Expression*> args);
		};

		struct ExpressionNew : public Expression {
			tp::String mNewType;
			ExpressionNew(tp::String type);
		};

		struct ExpressionLocal : public Expression {
			tp::String mLocalId;
			ExpressionLocal(tp::String id);
		};

		struct ExpressionFunc : public Expression {
			tp::String mFuncId;
			ExpressionFunc(tp::String id);
		};

		struct ExpressionChild : public Expression {
			Expression* mParent = NULL;
			tp::String mLocalId;
			bool mMethod = false;
			ExpressionChild(Expression* mParent, tp::String id);
		};

		struct ExpressionCall : public Expression {
			Expression* mParent = NULL;
			tp::Buffer<Expression*> mArgs;
			ExpressionCall(Expression* mParent, tp::InitialierList<Expression*> args);
		};

		struct ExpressionAriphm : public Expression {
			Expression* mLeft = NULL;
			Expression* mRight = NULL;
			OpCode mOpType;
			ExpressionAriphm(Expression* left, Expression* right, OpCode type);
		};

		struct ExpressionBoolean : public Expression {
			Expression* mLeft = NULL;
			Expression* mRight = NULL;

			enum class BoolType : tp::uint1 {
				AND = 24U, 
				OR,
				EQUAL,
				NOT_EQUAL,
				MORE,
				LESS,
				EQUAL_OR_MORE,
				EQUAL_OR_LESS,
				NOT,
			} mBoolType;

			ExpressionBoolean(Expression* left, Expression* right, BoolType type);
			ExpressionBoolean(Expression* invert);
		};

		struct ExpressionConst : public Expression {
			enum ConstType { STR, INT, BOOL, FLT } mConstType;
			tp::String str;
			tp::alni integer = 0;
			tp::alnf floating = 0;
			bool boolean = 0;

			ExpressionConst(tp::String val);
			ExpressionConst(const char* val);
			ExpressionConst(tp::alni val);
			ExpressionConst(tp::int4 val);
			ExpressionConst(tp::flt4 val);
			ExpressionConst(tp::alnf val);
			ExpressionConst(bool val);
		};

		struct ExpressionSelf : public Expression {
			ExpressionSelf();
		};

		ExpressionLocal* ExprLocal(tp::String id);
		ExpressionSelf* ExprSelf();
		ExpressionFunc* ExprFunc(tp::String id);
		ExpressionNew* ExprNew(tp::String id);
		ExpressionAriphm* ExprAriphm(Expression* left, Expression* right, OpCode type);
		ExpressionBoolean* ExprBool(Expression* left, Expression* right, OpCode type);
		ExpressionBoolean* ExprBoolNot(Expression* invert);
		template <typename ConstType>
		ExpressionConst* ExprConst(ConstType val) {
			return new ExpressionConst(val);
		}
	};
};