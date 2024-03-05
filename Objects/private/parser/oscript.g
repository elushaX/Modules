oscript {
	%whitespace "[ \t\r\n]*";
	%none error;
	%none integer;
	%left '(' ')';
	%left '+' '-';
	%left '*' '/';
	%left '.' '=';
	%left '==' '>' '<' '>=' '<=' '!=' '!';

	scope:
		'{' stmts '}' [scope]
		| ;

	stmts:
			stmts ';' stmt ';' [stm_list_append]
		| stmt [stm_list_create]
		| ;

	stmt:
			stm_var_def_new_type [tmp]
		| stm_var_def_assign [tmp]
		| stm_log [tmp]
		| stm_assign [tmp]
	;

	stm_var_def_new_type: 'var' id ':' id [stm_var_def_new_type];
	stm_var_def_assign: 'var' id '=' expr [stm_var_def_assign];
	stm_log: 'print' expr [stm_log];
	stm_assign: expr '=' expr [stm_assign];

	expr_list:
			expr_list ',' expr ',' [expr_list_append]
		| expr [expr_list_create]
		| ;

	expr:
			id '(' expr_list ')' [expr_function]
		|	expr '.' id '(' expr_list ')' [expr_method]
		|	expr '.' id [expr_child]
		| expr '==' expr [expr_bool_eq]
		| '!' expr [expr_bool_negate]
		| expr '!=' expr [expr_bool_neq]
		| expr '>' expr [expr_bool_grater]
		| expr '<' expr [expr_bool_lesser]
		| expr '>=' expr [expr_bool_grater_eq]
		| expr '<=' expr [expr_bool_lesser_eq]
		| expr '+' expr [expr_add]
		| expr '-' expr [expr_subtract]
		| expr '*' expr [expr_multiply]
		| expr '/' expr [expr_divide]
		| '(' expr ')' [expr_compound]
		| boolean [expr_bool]
		| integer [expr_int]
		| real [expr_float]
		| string [expr_string]
		| id [expr_id]
	;

	id: "[a-z]";
	boolean: "true|false";
	integer: "(\+|\-)?[0-9]+";
	real: "(\+|\-)?[0-9]+(\.[0-9]+)?((e|E)(\+|\-)?[0-9]+)?";
	string: "[\"']:string:";
}
