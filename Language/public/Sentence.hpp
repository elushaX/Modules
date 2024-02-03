
#pragma once

#include "LanguageCommon.hpp"

namespace tp {
	struct Sentence {
		const int1* buffer = nullptr;
		alni symbolSize = 1;

		explicit Sentence(const char* buff) { buffer = buff; }
	};
}
