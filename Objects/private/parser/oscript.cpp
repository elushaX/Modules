
#include <lalr/ParserStateMachine.hpp>
#include <lalr/ParserState.hpp>
#include <lalr/ParserTransition.hpp>
#include <lalr/ParserSymbol.hpp>
#include <lalr/ParserAction.hpp>
#include <lalr/LexerStateMachine.hpp>
#include <lalr/LexerState.hpp>
#include <lalr/LexerTransition.hpp>
#include <lalr/LexerAction.hpp>

namespace
{

using namespace lalr;

extern const LexerAction lexer_actions [];
extern const LexerTransition lexer_transitions [];
extern const LexerState lexer_states [];
extern const LexerAction whitespace_lexer_actions [];
extern const LexerTransition whitespace_lexer_transitions [];
extern const LexerState whitespace_lexer_states [];
extern const ParserAction actions [];
extern const ParserSymbol symbols [];
extern const ParserTransition transitions [];
extern const ParserState states [];

const ParserAction actions [] = 
{
    {0, "scope"},
    {1, "stm_scope_append"},
    {2, "stm_scope_create"},
    {3, "tmp"},
    {4, "stm_defVar"},
    {5, "stm_log"},
    {6, "expr_bool"},
    {7, "expr_int"},
    {8, "expr_float"},
    {9, "expr_string"},
    {-1, nullptr}
};

const ParserSymbol symbols [] = 
{
    {0, (SymbolType) 2, "dot_start", ".start", "dot_start"},
    {1, (SymbolType) 3, "dot_end", ".end", "dot_end"},
    {2, (SymbolType) 1, "error", "error", "error"},
    {3, (SymbolType) 1, "dot_whitespace", ".whitespace", "dot_whitespace"},
    {4, (SymbolType) 2, "scope", "scope", "scope"},
    {5, (SymbolType) 1, "left_curly_brace_terminal", "{", "{"},
    {6, (SymbolType) 2, "statements", "statements", "statements"},
    {7, (SymbolType) 1, "right_curly_brace_terminal", "}", "}"},
    {8, (SymbolType) 1, "semi_colon_terminal", ";", ";"},
    {9, (SymbolType) 2, "statement", "statement", "statement"},
    {10, (SymbolType) 2, "statementVar", "statementVar", "statementVar"},
    {11, (SymbolType) 2, "statementLog", "statementLog", "statementLog"},
    {12, (SymbolType) 1, "var_terminal", "var", "var"},
    {13, (SymbolType) 1, "print_terminal", "print", "print"},
    {14, (SymbolType) 2, "value", "value", "value"},
    {15, (SymbolType) 1, "id", "[a-z]", "id"},
    {16, (SymbolType) 1, "boolean", "true|false", "boolean"},
    {17, (SymbolType) 1, "integer", "(\\+|\\-)?[0-9]+", "integer"},
    {18, (SymbolType) 1, "real", "(\\+|\\-)?[0-9]+(\\.[0-9]+)?((e|E)(\\+|\\-)?[0-9]+)?", "real"},
    {19, (SymbolType) 1, "string", "[\\\"']:string:", "string"},
    {-1, (SymbolType) 0, nullptr, nullptr, nullptr}
};

const ParserTransition transitions [] = 
{
    {&symbols[4], &states[1], nullptr, 0, 0, 0, 0},
    {&symbols[5], &states[2], nullptr, 0, 0, 0, 1},
    {&symbols[1], nullptr, &symbols[4], 0, 0, -1, 2},
    {&symbols[1], nullptr, &symbols[0], 1, 0, -1, 3},
    {&symbols[6], &states[3], nullptr, 0, 0, 0, 4},
    {&symbols[9], &states[8], nullptr, 0, 0, 0, 5},
    {&symbols[10], &states[9], nullptr, 0, 0, 0, 6},
    {&symbols[11], &states[10], nullptr, 0, 0, 0, 7},
    {&symbols[12], &states[11], nullptr, 0, 0, 0, 8},
    {&symbols[13], &states[13], nullptr, 0, 0, 0, 9},
    {&symbols[7], nullptr, &symbols[6], 0, 0, -1, 10},
    {&symbols[8], nullptr, &symbols[6], 0, 0, -1, 11},
    {&symbols[7], &states[4], nullptr, 0, 0, 0, 12},
    {&symbols[8], &states[5], nullptr, 0, 0, 0, 13},
    {&symbols[1], nullptr, &symbols[4], 3, 0, 0, 14},
    {&symbols[9], &states[6], nullptr, 0, 0, 0, 15},
    {&symbols[10], &states[9], nullptr, 0, 0, 0, 16},
    {&symbols[11], &states[10], nullptr, 0, 0, 0, 17},
    {&symbols[12], &states[11], nullptr, 0, 0, 0, 18},
    {&symbols[13], &states[13], nullptr, 0, 0, 0, 19},
    {&symbols[8], &states[7], nullptr, 0, 0, 0, 20},
    {&symbols[7], nullptr, &symbols[6], 4, 0, 1, 21},
    {&symbols[8], nullptr, &symbols[6], 4, 0, 1, 22},
    {&symbols[7], nullptr, &symbols[6], 1, 0, 2, 23},
    {&symbols[8], nullptr, &symbols[6], 1, 0, 2, 24},
    {&symbols[7], nullptr, &symbols[9], 1, 0, 3, 25},
    {&symbols[8], nullptr, &symbols[9], 1, 0, 3, 26},
    {&symbols[7], nullptr, &symbols[9], 1, 0, 3, 27},
    {&symbols[8], nullptr, &symbols[9], 1, 0, 3, 28},
    {&symbols[15], &states[12], nullptr, 0, 0, 0, 29},
    {&symbols[7], nullptr, &symbols[10], 2, 0, 4, 30},
    {&symbols[8], nullptr, &symbols[10], 2, 0, 4, 31},
    {&symbols[14], &states[14], nullptr, 0, 0, 0, 32},
    {&symbols[16], &states[15], nullptr, 0, 0, 0, 33},
    {&symbols[17], &states[16], nullptr, 0, 0, 0, 34},
    {&symbols[18], &states[17], nullptr, 0, 0, 0, 35},
    {&symbols[19], &states[18], nullptr, 0, 0, 0, 36},
    {&symbols[7], nullptr, &symbols[14], 0, 0, -1, 37},
    {&symbols[8], nullptr, &symbols[14], 0, 0, -1, 38},
    {&symbols[7], nullptr, &symbols[11], 2, 0, 5, 39},
    {&symbols[8], nullptr, &symbols[11], 2, 0, 5, 40},
    {&symbols[7], nullptr, &symbols[14], 1, 0, 6, 41},
    {&symbols[8], nullptr, &symbols[14], 1, 0, 6, 42},
    {&symbols[7], nullptr, &symbols[14], 1, 0, 7, 43},
    {&symbols[8], nullptr, &symbols[14], 1, 0, 7, 44},
    {&symbols[7], nullptr, &symbols[14], 1, 0, 8, 45},
    {&symbols[8], nullptr, &symbols[14], 1, 0, 8, 46},
    {&symbols[7], nullptr, &symbols[14], 1, 0, 9, 47},
    {&symbols[8], nullptr, &symbols[14], 1, 0, 9, 48},
    {nullptr, nullptr, nullptr, 0, 0, 0, -1}
};

const ParserState states [] = 
{
    {0, 3, &transitions[0], "(null)"},
    {1, 1, &transitions[3], "(null)"},
    {2, 8, &transitions[4], "(null)"},
    {3, 2, &transitions[12], "(null)"},
    {4, 1, &transitions[14], "(null)"},
    {5, 5, &transitions[15], "(null)"},
    {6, 1, &transitions[20], "(null)"},
    {7, 2, &transitions[21], "(null)"},
    {8, 2, &transitions[23], "(null)"},
    {9, 2, &transitions[25], "(null)"},
    {10, 2, &transitions[27], "(null)"},
    {11, 1, &transitions[29], "(null)"},
    {12, 2, &transitions[30], "(null)"},
    {13, 7, &transitions[32], "(null)"},
    {14, 2, &transitions[39], "(null)"},
    {15, 2, &transitions[41], "(null)"},
    {16, 2, &transitions[43], "(null)"},
    {17, 2, &transitions[45], "(null)"},
    {18, 2, &transitions[47], "(null)"},
    {-1, 0, nullptr}
};

const LexerAction lexer_actions [] = 
{
    {0, "string"},
    {-1, nullptr}
};

const LexerTransition lexer_transitions [] = 
{
    {34, 35, &lexer_states[45], nullptr},
    {39, 40, &lexer_states[45], nullptr},
    {43, 44, &lexer_states[38], nullptr},
    {45, 46, &lexer_states[38], nullptr},
    {46, 47, &lexer_states[6], nullptr},
    {48, 58, &lexer_states[37], nullptr},
    {59, 60, &lexer_states[19], nullptr},
    {97, 101, &lexer_states[28], nullptr},
    {101, 102, &lexer_states[1], nullptr},
    {102, 103, &lexer_states[30], nullptr},
    {103, 112, &lexer_states[28], nullptr},
    {112, 113, &lexer_states[23], nullptr},
    {113, 116, &lexer_states[28], nullptr},
    {116, 117, &lexer_states[29], nullptr},
    {117, 118, &lexer_states[28], nullptr},
    {118, 119, &lexer_states[20], nullptr},
    {119, 123, &lexer_states[28], nullptr},
    {123, 124, &lexer_states[17], nullptr},
    {125, 126, &lexer_states[18], nullptr},
    {114, 115, &lexer_states[2], nullptr},
    {114, 115, &lexer_states[3], nullptr},
    {111, 112, &lexer_states[4], nullptr},
    {114, 115, &lexer_states[5], nullptr},
    {119, 120, &lexer_states[7], nullptr},
    {104, 105, &lexer_states[8], nullptr},
    {105, 106, &lexer_states[9], nullptr},
    {116, 117, &lexer_states[10], nullptr},
    {101, 102, &lexer_states[11], nullptr},
    {115, 116, &lexer_states[12], nullptr},
    {112, 113, &lexer_states[13], nullptr},
    {97, 98, &lexer_states[14], nullptr},
    {99, 100, &lexer_states[15], nullptr},
    {101, 102, &lexer_states[16], nullptr},
    {97, 98, &lexer_states[21], nullptr},
    {114, 115, &lexer_states[22], nullptr},
    {114, 115, &lexer_states[24], nullptr},
    {105, 106, &lexer_states[25], nullptr},
    {110, 111, &lexer_states[26], nullptr},
    {116, 117, &lexer_states[27], nullptr},
    {114, 115, &lexer_states[31], nullptr},
    {97, 98, &lexer_states[33], nullptr},
    {117, 118, &lexer_states[32], nullptr},
    {101, 102, &lexer_states[36], nullptr},
    {108, 109, &lexer_states[34], nullptr},
    {115, 116, &lexer_states[35], nullptr},
    {101, 102, &lexer_states[36], nullptr},
    {46, 47, &lexer_states[39], nullptr},
    {48, 58, &lexer_states[37], nullptr},
    {69, 70, &lexer_states[41], nullptr},
    {101, 102, &lexer_states[41], nullptr},
    {48, 58, &lexer_states[37], nullptr},
    {48, 58, &lexer_states[40], nullptr},
    {48, 58, &lexer_states[40], nullptr},
    {69, 70, &lexer_states[41], nullptr},
    {101, 102, &lexer_states[41], nullptr},
    {43, 44, &lexer_states[42], nullptr},
    {45, 46, &lexer_states[42], nullptr},
    {48, 58, &lexer_states[43], nullptr},
    {48, 58, &lexer_states[43], nullptr},
    {48, 58, &lexer_states[43], nullptr},
    {0, 2147483647, &lexer_states[44], &lexer_actions[0]},
    {-1, -1, nullptr, nullptr}
};

const LexerState lexer_states [] = 
{
    {0, 19, &lexer_transitions[0], nullptr},
    {1, 1, &lexer_transitions[19], &symbols[15]},
    {2, 1, &lexer_transitions[20], nullptr},
    {3, 1, &lexer_transitions[21], nullptr},
    {4, 1, &lexer_transitions[22], nullptr},
    {5, 0, &lexer_transitions[23], &symbols[2]},
    {6, 1, &lexer_transitions[23], nullptr},
    {7, 1, &lexer_transitions[24], nullptr},
    {8, 1, &lexer_transitions[25], nullptr},
    {9, 1, &lexer_transitions[26], nullptr},
    {10, 1, &lexer_transitions[27], nullptr},
    {11, 1, &lexer_transitions[28], nullptr},
    {12, 1, &lexer_transitions[29], nullptr},
    {13, 1, &lexer_transitions[30], nullptr},
    {14, 1, &lexer_transitions[31], nullptr},
    {15, 1, &lexer_transitions[32], nullptr},
    {16, 0, &lexer_transitions[33], &symbols[3]},
    {17, 0, &lexer_transitions[33], &symbols[5]},
    {18, 0, &lexer_transitions[33], &symbols[7]},
    {19, 0, &lexer_transitions[33], &symbols[8]},
    {20, 1, &lexer_transitions[33], &symbols[15]},
    {21, 1, &lexer_transitions[34], nullptr},
    {22, 0, &lexer_transitions[35], &symbols[12]},
    {23, 1, &lexer_transitions[35], &symbols[15]},
    {24, 1, &lexer_transitions[36], nullptr},
    {25, 1, &lexer_transitions[37], nullptr},
    {26, 1, &lexer_transitions[38], nullptr},
    {27, 0, &lexer_transitions[39], &symbols[13]},
    {28, 0, &lexer_transitions[39], &symbols[15]},
    {29, 1, &lexer_transitions[39], &symbols[15]},
    {30, 1, &lexer_transitions[40], &symbols[15]},
    {31, 1, &lexer_transitions[41], nullptr},
    {32, 1, &lexer_transitions[42], nullptr},
    {33, 1, &lexer_transitions[43], nullptr},
    {34, 1, &lexer_transitions[44], nullptr},
    {35, 1, &lexer_transitions[45], nullptr},
    {36, 0, &lexer_transitions[46], &symbols[16]},
    {37, 4, &lexer_transitions[46], &symbols[17]},
    {38, 1, &lexer_transitions[50], nullptr},
    {39, 1, &lexer_transitions[51], nullptr},
    {40, 3, &lexer_transitions[52], &symbols[18]},
    {41, 3, &lexer_transitions[55], nullptr},
    {42, 1, &lexer_transitions[58], nullptr},
    {43, 1, &lexer_transitions[59], &symbols[18]},
    {44, 0, &lexer_transitions[60], &symbols[19]},
    {45, 1, &lexer_transitions[60], nullptr},
    {-1, 0, nullptr, nullptr}
};

const LexerStateMachine lexer_state_machine = 
{
    1, // #actions
    61, // #transitions
    46, // #states
    lexer_actions, // actions
    lexer_transitions, // transitions
    lexer_states, // states
    &lexer_states[0] // start state
};

const LexerAction whitespace_lexer_actions [] = 
{
    {-1, nullptr}
};

const LexerTransition whitespace_lexer_transitions [] = 
{
    {9, 11, &whitespace_lexer_states[0], nullptr},
    {13, 14, &whitespace_lexer_states[0], nullptr},
    {32, 33, &whitespace_lexer_states[0], nullptr},
    {-1, -1, nullptr, nullptr}
};

const LexerState whitespace_lexer_states [] = 
{
    {0, 3, &whitespace_lexer_transitions[0], &symbols[3]},
    {-1, 0, nullptr, nullptr}
};

const LexerStateMachine whitespace_lexer_state_machine = 
{
    0, // #actions
    3, // #transitions
    1, // #states
    whitespace_lexer_actions, // actions
    whitespace_lexer_transitions, // transitions
    whitespace_lexer_states, // states
    &whitespace_lexer_states[0] // start state
};

const ParserStateMachine parser_state_machine = 
{
    "oscript",
    10, // #actions
    20, // #symbols
    49, // #transitions
    19, // #states
    actions,
    symbols,
    transitions,
    states,
    &symbols[0], // start symbol
    &symbols[1], // end symbol
    &symbols[2], // error symbol
    &symbols[3], // whitespace symbol
    &states[0], // start state
    &lexer_state_machine, // lexer state machine
    &whitespace_lexer_state_machine // whitespace lexer state machine
};

}

const lalr::ParserStateMachine* oscript_parser_state_machine = &parser_state_machine;
