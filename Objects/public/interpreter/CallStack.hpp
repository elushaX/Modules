#pragma once

#include "core/Object.hpp"
#include "primitives/ClassObject.hpp"

#include "Map.hpp"

namespace tp::obj {
	struct MethodObject;

	struct ByteCode;

	struct CallStack {

		struct CallFrame {
			enum { CALL_DEPTH = 1024 };
			Object* mSelf = nullptr;
			MethodObject* mMethod = nullptr;
			ualni mIp = 0;
		};

		void enter(const CallFrame& frame);
		void leave();
		ByteCode* getBytecode();
		[[nodiscard]] halni len() const;

		ConstSizeBuffer<CallFrame, CallFrame::CALL_DEPTH> mStack;
	};
}