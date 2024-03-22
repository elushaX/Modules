#pragma once

#include "core/object.h"
#include "primitives/classobject.h"

#include "Map.hpp"

namespace obj {
	struct MethodObject;
};

namespace obj {

	struct ByteCode;

	struct CallStack {

		struct CallFrame {
			enum { CALL_DEPTH = 1024 };
			obj::Object* mSelf = nullptr;
			obj::MethodObject* mMethod = nullptr;
			tp::ualni mIp = 0;
		};

		void enter(const CallFrame& frame);
		void leave();
		ByteCode* getBytecode();
		[[nodiscard]] tp::halni len() const;

		tp::ConstSizeBuffer<CallFrame, CallFrame::CALL_DEPTH> mStack;
	};
};