#pragma once

#include "interpreter/ByteCode.hpp"

namespace obj {
	struct Script {
		obj::StringObject* mReadable;
		ByteCode mBytecode;

		void compile();
	};
};