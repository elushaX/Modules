
#pragma once

#include "SimpleParser.hpp"

const char* gGrammar = R"(
oscript {
	%whitespace "[ \t\r\n]+" ;
	%left '(' ')' ;
	%left '+' '-' ;
	%left '*' '/' ;
	%left '.' '=' ;
	%left '==' '>' '<' '>=' '<=' '!=' '!' ;
	%left ';' ;

	script : stmts | ;

	scope : '{' stmts '}' | '{' '}' ;

	stmts : stmts stmt | stmt ;

	stmt : stm_var_def_new_type ';'
		| stm_var_def_assign ';'
		| stm_log ';'
		| stm_assign ';'
		| stm_while
		| stm_return ';'
		| stm_break ';'
		| stm_if
		| stm_def_class
		| stm_def_method
		| stm_ignore ';' ;

	stm_def_method : 'method' id '(' id_list ')' scope ;
	stm_def_class : 'class' id scope ;

	stm_if : 'if' '(' expr ')' scope
		| 'if' '(' expr ')' scope 'else' scope ;

	stm_break : 'break' ;
	stm_return : 'return' expr | 'return' ;
	stm_while : 'while' '(' expr ')' scope ;
	stm_var_def_new_type : 'var' id ':' id ;
	stm_var_def_assign : 'var' id '=' expr ;
	stm_log : 'print' expr ;
	stm_assign : expr '=' expr ;
	stm_ignore : expr ;

	expr_list : expr_list ',' expr ',' | expr | ;

	id_list : id_list ',' id ',' | id | ;

	expr : id '(' expr_list ')'
		| expr '.' id '(' expr_list ')'
		| expr '.' id
		| expr '==' expr
		| '!' expr
		| expr '!=' expr
		| expr '>' expr
		| expr '<' expr
		| expr '>=' expr
		| expr '<=' expr
		| expr '+' expr
		| expr '-' expr
		| expr '*' expr
		| expr '/' expr
		| '(' expr ')'
		| boolean
		| integer
		| id ;

	id : "[a-zA-Z]+" ;
	boolean : "true|false" ;
	integer : "[0-9]+" ;
}
)";

const char* gSentence = R"(
method main ( ) {
	var i = 10 ;

	if ( i == 10 ) {
		while ( i > 0 ) {
			print i ;
			i = i - 1 ;
		}
	} else {
		print i ;
	}
}

main ( ) ;
)";
