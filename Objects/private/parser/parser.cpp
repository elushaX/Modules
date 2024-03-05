

#include <parser/parser.h>

#include "Private.hpp"

using namespace obj;

Parser::Result Parser::parse(const tp::String& stream) {

	lalr::ErrorPolicy errorPolicy;

	LalrParser parser(oscript_parser_state_machine, &errorPolicy);

	bind(parser);
	
	std::string streamStd(stream.read());

	LALR_ASSERT(parser.valid());

	parser.parse(streamStd.begin(), streamStd.end());

	parser.accepted();
	parser.full();

	auto scope = (BCgen::StatementScope*) parser.user_data();

	return { nullptr, scope };
}