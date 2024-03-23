#pragma once

#include "compiler/Statements.hpp"
#include "compiler/Expressions.hpp"

namespace tp::obj {

	class Parser {
	public:
		Parser() = default;

		struct Result {
			StatementScope* scope = nullptr;
			bool isError = false;
			std::string description;
			halni line = 0;
			halni column = 0;
		};

		Result parse(const std::string& stream);
	};
}
