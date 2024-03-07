oscript {
   %whitespace "[ \t\r\n]*";

   scope:
        '{' statements '}' [scope]
        | ;

   statements:
          statements ';' statement ';' [stm_scope_append]
        | statement [stm_scope_create]
        | ;

   statement:
          statementVar [tmp]
        | statementLog [tmp];

   statementVar: 'var' id [stm_defVar];
   statementLog: 'print' value [stm_log];

   value:
        boolean [expr_bool]
      | integer [expr_int]
      | real [expr_float]
      | string [expr_string]
      |
   ;

    id: "[a-z]";
    boolean: "true|false";
    integer: "(\+|\-)?[0-9]+";
    real: "(\+|\-)?[0-9]+(\.[0-9]+)?((e|E)(\+|\-)?[0-9]+)?";
    string: "[\"']:string:";
}
