
#pragma once

#include <lalr/Parser.hpp>
#include <lalr/string_literal.hpp>

typedef void* UserData;
typedef char SymbolType;

typedef std::basic_string<SymbolType>::iterator IteratorType;

extern const lalr::ParserStateMachine* oscript_parser_state_machine;

typedef lalr::Parser<IteratorType, UserData> LalrParser;

void bind(LalrParser& parser);