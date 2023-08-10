#include "NewPlacement.hpp"

#include "Testing.hpp"
#include "Tokenizer.hpp"
#include <cstdio>

#define LOG(val) // std::cout << #val << " "

using namespace tp;

ualni outputHashPassed = 1156;

const char* script =
R"(

/* yea
comment  "and string inside"  with int 123 and float 0.123f */

-1.f;

if (+1.f) {
	var string = "string value some &&characters inside string&& -= asdf 09a
	another line";

}

while

return

)";

TEST_DEF_STATIC(General) {

	enum class TokType {
		NONE,
		FAILED,
		TOK_SOURCE_END,

		VAR,
		CLASS_DEF,
		SELF,
		SCOPE_IN,
		SCOPE_OUT,
		ASSIGN,
		DEF_FUNC,
		PRINT,
		IF,
		ELSE,
		WHILE,
		BRACKET_IN,
		BRACKET_OUT,
		COMMA,
		NEW,
		CHILD,
		RETURN,
		EQUAL,
		NOT_EQUAL,
		MORE,
		LESS,
		QE_OR_MORE,
		QE_OR_LESS,
		BOOL_NOT,
		BOOL_AND,
		BOOL_OR,
		ADD,
		MUL,
		SUB,
		DIV,
		STM_END,
		CONST_TRUE,
		CONST_FALSE,
		CONST_INT,
		CONST_FLOAT,
		SPACE,
		CONST_STRING,
		ID,

		//COMMENT_LINE,

		COMMENT_BLOCK,
	};

	SimpleTokenizer<char, TokType, TokType::NONE, TokType::FAILED, TokType::TOK_SOURCE_END> lexer;

	lexer.build({
			{ "\n|\t| |\r", TokType::SPACE },
			{ "var", TokType::VAR },
			{ "class", TokType::CLASS_DEF },
			{ "self",	 TokType::SELF },
			{ "\\{", TokType::SCOPE_IN },
			{ "\\}", TokType::SCOPE_OUT },
			{ "=", TokType::ASSIGN },
			{ "def", TokType::DEF_FUNC },
			{ "<<",	 TokType::PRINT },
			{ "if",	 TokType::IF },
			{ "else",	 TokType::ELSE },
			{ "while", TokType::WHILE },
			{ "\\(", TokType::BRACKET_IN },
			{ "\\)", TokType::BRACKET_OUT },
			{ ",", TokType::COMMA },
			{ "new", TokType::NEW },
			{ "\\.",	 TokType::CHILD },
			{ "return",	 TokType::RETURN },
			{ "==",	 TokType::EQUAL },
			{ "!=",	 TokType::NOT_EQUAL },
			{ ">", TokType::MORE },
			{ "<", TokType::LESS },
			{ ">=",	 TokType::QE_OR_MORE },
			{ "<=",	 TokType::QE_OR_LESS },
			{ "!", TokType::BOOL_NOT },
			{ "&&",	 TokType::BOOL_AND },
			{ "\\|\\|",	 TokType::BOOL_OR },
			{ "\\+", TokType::ADD },
			{ "\\*", TokType::MUL },
			{ "\\-", TokType::SUB },
			{ "/", TokType::DIV },
			{ ";", TokType::STM_END },
			{ "true",	 TokType::CONST_TRUE },
			{ "false", TokType::CONST_FALSE },
			{ "((\\-)|(\\+))?[0-9]+i?",	 TokType::CONST_INT },
			{ R"(((\-)|(\+))?([0-9]+)(\.)([0-9]*)?f?)",	 TokType::CONST_FLOAT },
			{ R"((/\*){\*-\*}*(\*/))",	 TokType::COMMENT_BLOCK },
			{ R"("{"-"}*")",	 TokType::CONST_STRING },
			{ "([a-z]|[A-Z]|_)+([a-z]|[A-Z]|[0-9]|_)*",	 TokType::ID },
	});

	if (!lexer.isBuild()) {
		printf("Error : %s", lexer.getBuildError().description);
		return;
	}

	lexer.bindSource(script);

	TokType tok;
	ualni outputHash = 0;
	do {

		tok = lexer.readTok();

		outputHash += ualni(tok);

		switch (tok) {
			case TokType::SPACE: { printf(" "); } break;
			case TokType::VAR: { LOG(VAR); } break;
			case TokType::CLASS_DEF: { LOG(CLASS_DEF); } break;
			case TokType::SELF: { LOG(SELF); } break;
			case TokType::SCOPE_IN: { LOG(SCOPE_IN); } break;
			case TokType::SCOPE_OUT: { LOG(SCOPE_OUT); } break;
			case TokType::ASSIGN: { LOG(ASSIGN); } break;
			case TokType::DEF_FUNC: { LOG(DEF_FUNC); } break;
			case TokType::PRINT: { LOG(PRINT); } break;
			case TokType::IF: { LOG(IF); } break;
			case TokType::ELSE: { LOG(ELSE); } break;
			case TokType::WHILE: { LOG(WHILE); } break;
			case TokType::BRACKET_IN: { LOG(BRACKET_IN); } break;
			case TokType::BRACKET_OUT: { LOG(BRACKET_OUT); } break;
			case TokType::COMMA: { LOG(COMMA); } break;
			case TokType::NEW: { LOG(NEW); } break;
			case TokType::CHILD: { LOG(CHILD); } break;
			case TokType::RETURN: { LOG(RETURN); } break;
			case TokType::EQUAL: { LOG(EQUAL); } break;
			case TokType::NOT_EQUAL: { LOG(NOT_EQUAL); } break;
			case TokType::MORE: { LOG(MORE); } break;
			case TokType::LESS: { LOG(LESS); } break;
			case TokType::QE_OR_MORE: { LOG(QE_OR_MORE); } break;
			case TokType::QE_OR_LESS: { LOG(QE_OR_LESS); } break;
			case TokType::BOOL_NOT: { LOG(BOOL_NOT); } break;
			case TokType::BOOL_AND: { LOG(BOOL_AND); } break;
			case TokType::BOOL_OR: { LOG(BOOL_OR); } break;
			case TokType::ADD: { LOG(ADD); } break;
			case TokType::MUL: { LOG(MUL); } break;
			case TokType::SUB: { LOG(SUB); } break;
			case TokType::DIV: { LOG(DIV); } break;
			case TokType::STM_END: { LOG(STM_END); } break;
			case TokType::CONST_TRUE: { LOG(TRUE); } break;
			case TokType::CONST_FALSE: { LOG(FALSE); } break;
			case TokType::CONST_INT: { LOG(INT); } break;
			case TokType::CONST_FLOAT: { LOG(FLOAT); } break;
			case TokType::CONST_STRING: { LOG(STRING); } break;
			case TokType::ID: { LOG(ID); } break;
			//case TokType::COMMENT_LINE: { LOG(COMMENT_LINE) } break;
			case TokType::COMMENT_BLOCK: { LOG(COMMENT_BLOCK); } break;
			case TokType::FAILED: { LOG(FAILED); } break;
			case TokType::NONE:
			case TokType::TOK_SOURCE_END: break;
		}

	} while (tok != TokType::TOK_SOURCE_END && tok != TokType::FAILED);

	printf("\n\nOutputHash : %llu", outputHash);

	TEST(outputHash == outputHashPassed);
}

TEST_DEF(Simple) {
	enum class TokType {
		START = 0,
		NONE = 1,
		FAILED = 2,
		SPACE = 3,
		ID = 4,
		COMMENT_BLOCK = 5,
		TOK_SOURCE_END = 6,
	};

	SimpleTokenizer<char, TokType, TokType::NONE, TokType::FAILED, TokType::TOK_SOURCE_END> lexer;

	lexer.build({
			{ " ", TokType::SPACE },
			{ "([a-c]|A)+",	 TokType::ID },
			{ "A", TokType::START },
	});

	if (!lexer.isBuild()) {
		printf("Error : %s", lexer.getBuildError().description);
		return;
	}

	lexer.bindSource(" A bc cb cc ");

	TokType tok;
	ualni outputHash = 0;
	printf("\n\n OutputHash : ");
	do {
		tok = lexer.readTok();
		outputHash += ualni(tok);
		printf(" %i ", int(tok));
	} while (tok != TokType::TOK_SOURCE_END && tok != TokType::FAILED);
	printf(" : %llu", outputHash);
	TEST(outputHash == 33);
}

TEST_DEF(Tokenizer) {
	testGeneral();
	testSimple();
}
