#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "lispy.h"

int main(int argc, char** argv) {
  /* Create Some Parsers */
  Number   = mpc_new("number");
  Boolean  = mpc_new("bool");
  String   = mpc_new("string");
  Comment  = mpc_new("comment");
  Symbol   = mpc_new("symbol");
  Sexpr    = mpc_new("sexpr");
  Qexpr    = mpc_new("qexpr");
  Expr     = mpc_new("expr");
  Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT, 
  "                                                      \
    number  : /-?[0-9]+/ ;                               \
    bool    : /(true|false)/ ;                           \
    string  : /\"(\\\\.|[^\"])*\"/ ;                     \
    comment : /;[^\\r\\n]*/ ;                            \
    symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%|]+/ ;       \
    sexpr   : '(' <expr>* ')' ;                          \
    qexpr   : '{' <expr>* '}' ;                          \
    expr    : <number> | <bool> | <string> | <comment>   \
            | <symbol> | <sexpr> | <qexpr> ;             \
    lispy   : /^/ <expr>* /$/ ;                          \
  ",
            Number, Boolean, String, Comment,
            Symbol, Sexpr, Qexpr, Expr, Lispy);

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  if (argc >= 2) {
    for (int i = 1; i < argc; i++) {
      lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
      lval* x = builtin_load(e, args);

      if (x->type == LVAL_ERR) {
        lval_println(x);
      }

      lval_del(args);
      lval_del(x);
    }
  } else {
    puts("Lisp Version 0.0.1");
    puts("Press Ctrl+c to Exit\n");

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
  }
  lenv_del(e);
  /* Undefine and Delete our Parsers */
  mpc_cleanup(9,
              Number, Boolean, String, Comment,
              Symbol, Sexpr, Qexpr, Expr, Lispy);
  return 0;
}
