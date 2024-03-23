
#include "parser/Parser.hpp"

#include "Private.hpp"

using namespace obj;

lalr::ErrorPolicy::~ErrorPolicy() = default;
void lalr::ErrorPolicy::lalr_error(int line, int column, int error, const char* format, va_list args) {}
void lalr::ErrorPolicy::lalr_vprintf(const char* format, va_list args){};

class CustomErrorPolicy : public lalr::ErrorPolicy {
public:
	virtual ~CustomErrorPolicy() override {}

	virtual void lalr_error(int line, int column, int error, const char* format, va_list args) override {
		printf("Parser Error: %i %i \n", line, column);
		vfprintf(stdout, format, args);
		printf("\n");
	}

	virtual void lalr_vprintf(const char* format, va_list args) override { printf(format, args); }
};

Parser::Result Parser::parse(const std::string& stream) {

	CustomErrorPolicy errorPolicy;

	LalrParser parser(oscript_parser_state_machine, &errorPolicy);

	bind(parser);

	std::string streamStd(stream.c_str());
	streamStd += "\n"; // for Windows os to be happy

	ASSERT(parser.valid());

	parser.parse(streamStd.begin(), streamStd.end());

	BCgen::StatementScope* out = nullptr;

	if (parser.accepted() && parser.full()) {
		out = (BCgen::StatementScope*) parser.user_data().statement;
	}

	return { out, !(parser.accepted() && parser.full()) };
}