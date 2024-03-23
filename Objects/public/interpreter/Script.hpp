#pragma once

#include "interpreter/ByteCode.hpp"

namespace tp::obj {
	struct Script {
		StringObject* mReadable;
		ByteCode mBytecode;

		void compile();
	};
}