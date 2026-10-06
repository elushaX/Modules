#pragma once

#include "SimpleParser.hpp"

const char* gGrammar = R"(
# Grammar in the CF-RE United Format (Defined by language module)

Rules : {
	ScopeList : ScopeList Scope | Scope ;
	Scope : \ScopeBegin StatementList \ScopeEnd;
	StatementList : StatementList Statement \StatementEnd;
	StatementList : Statement \StatementEnd;
	Statement : \StatementBody;
}

Terminals : {
	Space : " " | "\t" | "\n" | "\r";
	ScopeBegin : "{";
	ScopeEnd : "}";
  StatementEnd : ";";
  StatementBody : "a" | "b";
}

Start : Scope;
Ignore : Space;
)";

const char* gSentence = R"(
{}

{ }

{
a;
 a ; a ;

 }

 {
a;
 a;
}
)";