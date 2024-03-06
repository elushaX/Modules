
#include "Private.hpp"

#include <parser/parser.h>

using namespace obj::BCgen;

typedef lalr::ParserNode<SymbolType> Node;

static UserData scope(const UserData* start, const Node*, size_t) { return start[1]; }

static UserData scope_empty(const UserData*, const Node*, size_t) { return new StatementScope({}, true); }

static UserData stm_list_append(const UserData* start, const Node*, size_t) {
	auto scope = (StatementScope*) start[0];
	auto stm = (Statement*) start[1];
	scope->mStatements.append(stm);
	return scope;
}

static UserData stm_list_create(const UserData* start, const Node*, size_t) {
	return new StatementScope({ (Statement*) start[0] }, true);
}

static UserData stm_log(const UserData* start, const Node*, size_t) {
	auto expr = (Expression*) (start[1]);
	return new StatementPrint(expr);
}

static UserData stm_assign(const UserData* start, const Node*, size_t) {
	return new StatementCopy((Expression*) start[0], (Expression*) start[2]);
}

static UserData stm_var_def_new_type(const UserData*, const Node* nodes, size_t) {
	return new StatementLocalDef((char*) nodes[1].lexeme().c_str(), new ExpressionNew((char*) nodes[3].lexeme().c_str()));
}

static UserData stm_var_def_assign(const UserData* start, const Node* nodes, size_t) {
	return new StatementLocalDef((char*) nodes[1].lexeme().c_str(), (Expression*) start[3]);
}

static UserData stm_return(const UserData* start, const Node*, size_t length) {
	if (length == 2) {
		return new StatementReturn((Expression*) start[1]);
	}
	return new StatementReturn();
}

static UserData stm_ignore(const UserData* start, const Node*, size_t) {
	return new StatementIgnore((Expression*) start[0]);
}

static UserData stm_def_class(const UserData* start, const Node* nodes, size_t) {
	auto newClass = new StatementClassDef((char*) nodes[1].lexeme().c_str(), (StatementScope*) start[2]);
	return newClass;
}

static UserData stm_def_method(const UserData* start, const Node* nodes, size_t) {
	auto method = new StatementFuncDef((char*) nodes[1].lexeme().c_str());
	method->mStatements = (StatementScope*) start[5];
	auto args = (tp::Buffer<tp::String>*) start[3];
	method->mArgs = *args;
	delete args;
	return method;
}

static UserData stm_if(const UserData* start, const Node*, size_t length) {
	if (length == 5) return new StatementIf((Expression*) start[2], (StatementScope*) start[4], nullptr);
	return new StatementIf((Expression*) start[2], (StatementScope*) start[4], (StatementScope*) start[6]);
}

static UserData stm_while_loop(const UserData* start, const Node*, size_t) {
	return new StatementWhile((Expression*) start[2], (StatementScope*) start[4]);
}

static UserData stm_break(const UserData*, const Node*, size_t) {
	ASSERT(0);
	// todo : implement break
	return nullptr;
}

static UserData id_list_append(const UserData* start, const Node* nodes, size_t) {
	auto list = (tp::Buffer<tp::String>*) start[0];
	list->append((char*) nodes[1].lexeme().c_str());
	return list;
}

static UserData id_list_create(const UserData*, const Node* nodes, size_t) {
	return new tp::Buffer<tp::String>({ (char*) nodes[0].lexeme().c_str() });
}

static UserData expr_function(const UserData* start, const Node*, size_t) {
	auto expr = (Expression*) (start[0]);
	auto args = (ExpressionList*) (start[2]);
	return expr->ExprCall(args);
}

static UserData expr_method(const UserData* start, const Node* nodes, size_t) {
	auto expr = (Expression*) (start[0]);
	auto args = (ExpressionList*) (start[4]);
	auto childId = nodes[1].lexeme();
	auto method = new ExpressionChild(expr, (char*) childId.c_str());
	return method->ExprCall(args);
}

static UserData expr_list_append(const UserData* start, const Node*, size_t) {
	auto list = (ExpressionList*) start[0];
	auto expr = (Expression*) start[2];
	list->mItems.append(expr);
	return list;
}

static UserData expr_list_create(const UserData* start, const Node*, size_t) {
	auto exprList = new ExpressionList();
	exprList->mItems.append((Expression*) start[0]);
	return exprList;
}

static UserData expr_child(const UserData* start, const Node* nodes, size_t) {
	return new ExpressionChild((Expression*) start[0], (char*) nodes[2].lexeme().c_str());
}

static UserData expr_bool_eq(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean((Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::EQUAL);
}

static UserData expr_bool_negate(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean((Expression*) start[1]);
}

static UserData expr_bool_neq(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean((Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::NOT_EQUAL);
}

static UserData expr_bool_grater(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean((Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::MORE);
}

static UserData expr_bool_lesser(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean((Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::LESS);
}

static UserData expr_bool_grater_eq(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean(
		(Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::EQUAL_OR_MORE
	);
}

static UserData expr_bool_lesser_eq(const UserData* start, const Node*, size_t) {
	return new ExpressionBoolean(
		(Expression*) start[0], (Expression*) start[2], ExpressionBoolean::BoolType::EQUAL_OR_LESS
	);
}

static UserData expr_add(const UserData* start, const Node*, size_t) {
	return new ExpressionArithmetics((Expression*) start[0], (Expression*) start[2], obj::OpCode::OBJ_ADD);
}

static UserData expr_subtract(const UserData* start, const Node*, size_t) {
	return new ExpressionArithmetics((Expression*) start[0], (Expression*) start[2], obj::OpCode::OBJ_SUB);
}

static UserData expr_multiply(const UserData* start, const Node*, size_t) {
	return new ExpressionArithmetics((Expression*) start[0], (Expression*) start[2], obj::OpCode::OBJ_MUL);
}

static UserData expr_divide(const UserData* start, const Node*, size_t) {
	return new ExpressionArithmetics((Expression*) start[0], (Expression*) start[2], obj::OpCode::OBJ_DIV);
}

static UserData expr_compound(const UserData* start, const Node*, size_t) { return start[1]; }

static UserData expr_bool(const UserData*, const Node* nodes, size_t) {
	return new ExpressionConst(nodes[0].lexeme() == "true");
}

static UserData expr_int(const UserData*, const Node* nodes, size_t) {
	return new ExpressionConst(stoi(nodes[0].lexeme()));
}

static UserData expr_float(const UserData*, const Node* nodes, size_t) {
	return new ExpressionConst(stof(nodes[0].lexeme()));
}

static UserData expr_string(const UserData*, const Node* nodes, size_t) {
	return new ExpressionConst((char*) nodes[0].lexeme().c_str());
}

static UserData expr_id(const UserData*, const Node* nodes, size_t) {
	return new ExpressionLocal((char*) nodes[0].lexeme().c_str());
}

static UserData tmp(const UserData* start, const Node*, size_t) {
	// pass through
	return *start;
}

#define BIND(name) parser.set_action_handler(#name, &(name))

void bind(LalrParser& parser) {
	BIND(scope);

	BIND(stm_list_append);
	BIND(stm_list_create);

	BIND(expr_list_append);
	BIND(expr_list_create);

	BIND(stm_var_def_new_type);
	BIND(stm_var_def_assign);
	BIND(stm_log);
	BIND(stm_assign);
	BIND(stm_ignore);

	BIND(stm_while_loop);
	BIND(stm_return);
	BIND(stm_break);
	BIND(scope_empty);
	BIND(stm_def_method);
	BIND(stm_def_class);
	BIND(stm_if);
	BIND(id_list_append);
	BIND(id_list_create);

	BIND(expr_function);
	BIND(expr_method);
	BIND(expr_child);
	BIND(expr_bool_eq);
	BIND(expr_bool_negate);
	BIND(expr_bool_neq);
	BIND(expr_bool_grater);
	BIND(expr_bool_lesser);
	BIND(expr_bool_grater_eq);
	BIND(expr_bool_lesser_eq);
	BIND(expr_add);
	BIND(expr_subtract);
	BIND(expr_multiply);
	BIND(expr_divide);
	BIND(expr_compound);
	BIND(expr_bool);
	BIND(expr_int);
	BIND(expr_float);
	BIND(expr_string);
	BIND(expr_id);

	BIND(tmp);

	parser.set_lexer_action_handler("string", &lalr::string_literal<IteratorType>);
}