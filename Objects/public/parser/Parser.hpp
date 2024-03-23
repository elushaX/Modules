#pragma once

#include "compiler/Statements.hpp"
#include "compiler/Expressions.hpp"

namespace obj {

	class Parser {
	public:
		Parser() = default;

		struct Result {
			obj::BCgen::StatementScope* scope = nullptr;
			bool isError = false;
			std::string description;
			tp::halni line = 0;
			tp::halni column = 0;
		};

		Result parse(const std::string& stream);
	};
};
