
#pragma once

#include "Strings.hpp"
#include "Buffer.hpp"

#include "interpreter/opcodes.h"

namespace obj::BCgen {

	struct Expression {
		enum class Type {
			NONE,
			NEW,
			LOCAL,
			CONST_EXPR,
			CHILD,
			CALL,
			ARITHMETICS,
			FUNC,
			BOOLEAN,
			SELF,
			LIST,
		} mType = Type::NONE;

		bool mValueUsed = false;

		explicit Expression(Type type);
		virtual ~Expression() = default;

		struct ExpressionChild* ExprChild(const tp::String& id);
		struct ExpressionCall* ExprCall(class ExpressionList* args);
	};

	struct ExpressionList : public Expression {
		tp::Buffer<Expression*> mItems;
		ExpressionList();
		~ExpressionList() override;
	};

	struct ExpressionNew : public Expression {
		tp::String mNewType;
		explicit ExpressionNew(const tp::String& type);
		~ExpressionNew() override;
	};

	struct ExpressionLocal : public Expression {
		tp::String mLocalId;
		explicit ExpressionLocal(const tp::String& id);
		~ExpressionLocal() override;
	};

	struct ExpressionFunc : public Expression {
		tp::String mFuncId;
		explicit ExpressionFunc(const tp::String& id);
		~ExpressionFunc() override;
	};

	struct ExpressionChild : public Expression {
		Expression* mParent = nullptr;
		tp::String mLocalId;
		bool mMethod = false;
		ExpressionChild(Expression* mParent, const tp::String& id);
		~ExpressionChild() override;
	};

	struct ExpressionCall : public Expression {
		Expression* mParent = nullptr;
		ExpressionList* mArgs = nullptr;
		ExpressionCall(Expression* mParent, ExpressionList* args);
		~ExpressionCall() override;
	};

	struct ExpressionArithmetics : public Expression {
		Expression* mLeft = nullptr;
		Expression* mRight = nullptr;
		OpCode mOpType = OpCode::NONE;
		ExpressionArithmetics(Expression* left, Expression* right, OpCode type);
		~ExpressionArithmetics() override;
	};

	struct ExpressionBoolean : public Expression {
		Expression* mLeft = nullptr;
		Expression* mRight = nullptr;

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
		explicit ExpressionBoolean(Expression* invert);
		~ExpressionBoolean() override;
	};

	struct ExpressionConst : public Expression {
		enum ConstType { STR, INT, BOOL, FLT } mConstType;
		tp::String str;
		tp::alni integer = 0;
		tp::alnf floating = 0;
		bool boolean = false;

		explicit ExpressionConst(const tp::String& val);
		explicit ExpressionConst(const char* val);
		explicit ExpressionConst(tp::alni val);
		explicit ExpressionConst(tp::int4 val);
		explicit ExpressionConst(tp::flt4 val);
		explicit ExpressionConst(tp::alnf val);
		explicit ExpressionConst(bool val);

		~ExpressionConst() override;
	};

	struct ExpressionSelf : public Expression {
		ExpressionSelf();
		~ExpressionSelf() override = default;
	};
}