#pragma once

#include "compiler/statement.h"
#include "compiler/expression.h"

namespace obj {

	class Parser {
	public:
		Parser() = default;

		struct Result {
			obj::BCgen::StatementScope* scope = nullptr;
			bool isError = false;
			tp::String description;
			tp::halni line = 0;
			tp::halni column = 0;
		};

		Result parse(const tp::String& stream);
	};
};
