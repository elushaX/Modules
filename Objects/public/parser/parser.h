#pragma once

#include "compiler/statement.h"
#include "compiler/expression.h"

namespace obj {

	class Parser {
	public:
		Parser() = default;

		struct Error {
			tp::String mDescription = "No Description";
			tp::alni mAdvancedIdx = 0;

			Error() = default;
			Error(const tp::String& description, tp::alni idx) {
				mDescription = description;
				mAdvancedIdx = idx;
			}

			tp::Pair<tp::alni, tp::alni> getErrorLocation(const char* stream) const {
				tp::alni line = 1;
				tp::alni column = 1;

				for (auto i : tp::Range(mAdvancedIdx)) {
					if (stream[i] == '\n') {
						column = 0;
						line++;
					}
					column++;
				}

				return { line, column };
			}
		};

		struct Result {
			Error* err = nullptr;
			obj::BCgen::StatementScope* scope = nullptr;
		};

		Result parse(const tp::String& stream);
	};
};
