#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "lispy.h"

long min(long x, long y) {
  return x > y ? y : x;
}

long max(long x, long y) {
  return x > y ? x : y;
}


int main(int argc, char** argv) {
  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Boolean  = mpc_new("bool");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Qexpr    = mpc_new("qexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number : /-?[0-9]+/ ;                               \
    bool   : /(true|false)/ ;                           \
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%|]+/ ;       \
    sexpr  : '(' <expr>* ')' ;                          \
    qexpr  : '{' <expr>* '}' ;                          \
    expr   : <number> | <bool> | <symbol>               \
           | <sexpr> | <qexpr> ;                        \
    lispy  : /^/ <expr>* /$/ ;                          \
  ",
            Number, Boolean, Symbol, Sexpr, Qexpr, Expr, Lispy);


  puts("Lisp Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  while (1) {

    char* input = readline("lisp> ");

    if (input && *input) {
      add_history(input);
      /* Attempt to Parse the user Input */
      mpc_result_t r;
      if (mpc_parse("<stdin>", input, Lispy, &r)) {
	/* On Success Print the AST
           mpc_ast_print(r.output);
	*/

	lval* result = lval_eval(e, lval_read(r.output));
	lval_println(result);
	lval_del(result);
	mpc_ast_delete(r.output);
	
      } else {
	/* Otherwise Print the Error */
	mpc_err_print(r.error);
	mpc_err_delete(r.error);
      }
      free(input);
    }
  }
  lenv_del(e);
  /* Undefine and Delete our Parsers */
  mpc_cleanup(7, Number, Boolean, Symbol, Sexpr, Qexpr, Expr, Lispy);
  return 0;
}
