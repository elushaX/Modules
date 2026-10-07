
#include "parser/Parser.hpp"

#include "Private.hpp"

#include "interpreter/OperatoinCodes.hpp"

using namespace tp;
using namespace obj;

#include "oscript.g.hpp"

namespace {

	class AstBuilder {
	public:
		AstBuilder(OScriptParser& parser, const char* source) :
			mParser(parser),
			mSource(source) {}

		Statement* buildScript(OScriptNode node) {
			if (node->leafs.size() == 0) {
				return new StatementScope({}, true);
			}
			return buildStatementScope(node->leafs[0]);
		}

	private:
		OScriptParser& mParser;
		const char* mSource;

		String action(OScriptNode node) { return mParser.actionOf(node); }

		std::string lexeme(OScriptNode node) {
			String s = mParser.lexemeOf(mSource, node);
			return std::string(s.read());
		}

		static bool is(const String& action, const char* name) {
			const char* a = action.read();
			const char* b = name;
			while (*a && *b) {
				if (*a != *b) return false;
				a++;
				b++;
			}
			return *a == *b;
		}

		StatementScope* buildStatementScope(OScriptNode stmts) {
			String act = action(stmts);
			if (is(act, "stm_list_create")) {
				return new StatementScope({ buildStatement(stmts->leafs[0]) }, true);
			}
			auto scope = buildStatementScope(stmts->leafs[0]);
			scope->mStatements.append(buildStatement(stmts->leafs[1]));
			return scope;
		}

		StatementScope* buildScope(OScriptNode node) {
			String act = action(node);
			if (is(act, "scope_empty")) {
				return new StatementScope({}, true);
			}
			return buildStatementScope(node->leafs[1]);
		}

		Statement* buildStatement(OScriptNode stmt) {
			OScriptNode inner = stmt->leafs[0];
			String act = action(inner);

			if (is(act, "stm_def_method")) return buildDefMethod(inner);
			if (is(act, "stm_def_class")) return buildDefClass(inner);
			if (is(act, "stm_if")) return buildIf(inner);
			if (is(act, "stm_while_loop")) return buildWhile(inner);
			if (is(act, "stm_return")) return buildReturn(inner);
			if (is(act, "stm_var_def_new_type")) return buildVarDefNewType(inner);
			if (is(act, "stm_var_def_assign")) return buildVarDefAssign(inner);
			if (is(act, "stm_log")) return buildLog(inner);
			if (is(act, "stm_assign")) return buildAssign(inner);
			if (is(act, "stm_ignore")) return buildIgnore(inner);
			if (is(act, "stm_break")) return nullptr;

			return nullptr;
		}

		Statement* buildDefMethod(OScriptNode node) {
			auto method = new StatementFuncDef(lexeme(node->leafs[1]));
			method->mStatements = buildScope(node->leafs[5]);
			Buffer<std::string> args;
			buildIdList(node->leafs[3], args);
			method->mArgs = args;
			return method;
		}

		Statement* buildDefClass(OScriptNode node) {
			return new StatementClassDef(lexeme(node->leafs[1]), buildScope(node->leafs[2]));
		}

		Statement* buildIf(OScriptNode node) {
			Expression* cond = buildExpr(node->leafs[2]);
			StatementScope* onTrue = buildScope(node->leafs[4]);
			if (node->leafs.size() == 5) {
				return new StatementIf(cond, onTrue, nullptr);
			}
			return new StatementIf(cond, onTrue, buildScope(node->leafs[6]));
		}

		Statement* buildWhile(OScriptNode node) {
			return new StatementWhile(buildExpr(node->leafs[2]), buildScope(node->leafs[4]));
		}

		Statement* buildReturn(OScriptNode node) {
			if (node->leafs.size() == 2) {
				return new StatementReturn(buildExpr(node->leafs[1]));
			}
			return new StatementReturn();
		}

		Statement* buildVarDefNewType(OScriptNode node) {
			return new StatementLocalDef(lexeme(node->leafs[1]), new ExpressionNew(lexeme(node->leafs[3])));
		}

		Statement* buildVarDefAssign(OScriptNode node) {
			return new StatementLocalDef(lexeme(node->leafs[1]), buildExpr(node->leafs[3]));
		}

		Statement* buildLog(OScriptNode node) {
			return new StatementPrint(buildExpr(node->leafs[1]));
		}

		Statement* buildAssign(OScriptNode node) {
			return new StatementCopy(buildExpr(node->leafs[0]), buildExpr(node->leafs[2]));
		}

		Statement* buildIgnore(OScriptNode node) {
			return new StatementIgnore(buildExpr(node->leafs[0]));
		}

		void buildIdList(OScriptNode node, Buffer<std::string>& out) {
			if (node->leafs.size() == 0) return;
			String act = action(node);
			if (is(act, "id_list_create")) {
				out.append(lexeme(node->leafs[0]));
				return;
			}
			buildIdList(node->leafs[0], out);
			out.append(lexeme(node->leafs[2]));
		}

		Expression* buildExpr(OScriptNode node) {
			String act = action(node);

			if (is(act, "expr_function")) {
				auto fn = new ExpressionLocal(lexeme(node->leafs[0]).c_str());
				auto args = (ExpressionList*) buildExprList(node->leafs[2]);
				return fn->ExprCall(args);
			}
			if (is(act, "expr_method")) {
				auto parent = buildExpr(node->leafs[0]);
				auto args = (ExpressionList*) buildExprList(node->leafs[4]);
				auto method = new ExpressionChild(parent, lexeme(node->leafs[2]).c_str());
				return method->ExprCall(args);
			}
			if (is(act, "expr_child")) {
				return new ExpressionChild(buildExpr(node->leafs[0]), lexeme(node->leafs[2]).c_str());
			}
			if (is(act, "expr_bool_eq")) return boolExpr(node, ExpressionBoolean::BoolType::EQUAL);
			if (is(act, "expr_bool_neq")) return boolExpr(node, ExpressionBoolean::BoolType::NOT_EQUAL);
			if (is(act, "expr_bool_grater")) return boolExpr(node, ExpressionBoolean::BoolType::MORE);
			if (is(act, "expr_bool_lesser")) return boolExpr(node, ExpressionBoolean::BoolType::LESS);
			if (is(act, "expr_bool_grater_eq")) return boolExpr(node, ExpressionBoolean::BoolType::EQUAL_OR_MORE);
			if (is(act, "expr_bool_lesser_eq")) return boolExpr(node, ExpressionBoolean::BoolType::EQUAL_OR_LESS);
			if (is(act, "expr_bool_negate")) return new ExpressionBoolean(buildExpr(node->leafs[1]));
			if (is(act, "expr_add")) return arithExpr(node, obj::OpCode::OBJ_ADD);
			if (is(act, "expr_subtract")) return arithExpr(node, obj::OpCode::OBJ_SUB);
			if (is(act, "expr_multiply")) return arithExpr(node, obj::OpCode::OBJ_MUL);
			if (is(act, "expr_divide")) return arithExpr(node, obj::OpCode::OBJ_DIV);
			if (is(act, "expr_compound")) return buildExpr(node->leafs[1]);
			if (is(act, "expr_bool")) return new ExpressionConst(lexeme(node->leafs[0]) == "true");
			if (is(act, "expr_int")) return new ExpressionConst((alni) stoll(lexeme(node->leafs[0])));
			if (is(act, "expr_float")) return new ExpressionConst((flt4) stof(lexeme(node->leafs[0])));
			if (is(act, "expr_string")) {
				std::string raw = lexeme(node->leafs[0]);
				std::string inner = raw.size() >= 2 ? raw.substr(1, raw.size() - 2) : std::string();
				return new ExpressionConst(inner.c_str());
			}
			if (is(act, "expr_id")) return new ExpressionLocal(lexeme(node->leafs[0]).c_str());

			return nullptr;
		}

		Expression* buildExprList(OScriptNode node) {
			if (node->leafs.size() == 0) {
				return new ExpressionList();
			}
			String act = action(node);
			if (is(act, "expr_list_create")) {
				auto list = new ExpressionList();
				list->mItems.append(buildExpr(node->leafs[0]));
				return list;
			}
			auto list = (ExpressionList*) buildExprList(node->leafs[0]);
			list->mItems.append(buildExpr(node->leafs[2]));
			return list;
		}

		Expression* boolExpr(OScriptNode node, ExpressionBoolean::BoolType type) {
			return new ExpressionBoolean(buildExpr(node->leafs[0]), buildExpr(node->leafs[2]), type);
		}

		Expression* arithExpr(OScriptNode node, obj::OpCode op) {
			return new ExpressionArithmetics(buildExpr(node->leafs[0]), buildExpr(node->leafs[2]), op);
		}
	};

	struct GrammarHolder {
		OScriptParser parser;
		bool compiled = false;
		GrammarHolder() {
			compiled = parser.compileTables(oscript_grammar, String::Logic::calcLength(oscript_grammar));
		}
	};

	OScriptParser& grammarParser() {
		static GrammarHolder holder;
		return holder.parser;
	}
}

obj::Parser::Result obj::Parser::parse(const std::string& stream) {
	std::string src = stream;
	src += "\n";

	OScriptParser& parser = grammarParser();

	auto res = parser.parse(src.c_str(), (ualni) src.size());

	if (!res.accepted || !res.ast) {
		return { nullptr, true };
	}

	AstBuilder builder(parser, src.c_str());
	auto* scope = (StatementScope*) builder.buildScript(res.ast);

	return { scope, false };
}
