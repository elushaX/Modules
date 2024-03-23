#pragma once

#include "ByteCode.hpp"
#include "core/Object.hpp"

namespace tp::obj {

	// can be other aligned value as well
	typedef Object* Operand;

	class OperandStack {

		enum : alni {
			MAX_STACK_SIZE = 512
		};

	public:

		Operand* mBuff;
		ualni mIdx;

		OperandStack();
		void push(Operand operand);
		void pop();

		Operand getOperand();

		template <typename ObjectType>
		ObjectType* getOperand() {
			auto operand = getOperand();
			auto ret = NDO_CAST(ObjectType, operand);
			ASSERT(ret && "Operand Has Invalid Object Type");
			return ret;
		}

		~OperandStack();
	};
}