#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "lispy.h"

int main(int argc, char** argv) {
  /* Create Some Parsers */
  Integer  = mpc_new("integer");
  Float    = mpc_new("float");
  Boolean  = mpc_new("bool");
  String   = mpc_new("string");
  Comment  = mpc_new("comment");
  Symbol   = mpc_new("symbol");
  Sexpr    = mpc_new("sexpr");
  Qexpr    = mpc_new("qexpr");
  List     = mpc_new("list");
  Expr     = mpc_new("expr");
  Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT, 
  "                                                      \
    float   : /-?[0-9]*\\.[0-9]+/ ;                      \
    integer : /-?[0-9]+/ ;                               \
    bool    : /(true|false)/ ;                           \
    string  : /\"(\\\\.|[^\"])*\"/ ;                     \
    comment : /;[^\\r\\n]*/ ;                            \
    symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%|]+/ ;       \
    sexpr   : '(' <expr>* ')' ;                          \
    qexpr   : '{' <expr>* '}' ;                          \
    list    : '[' <expr>* ']' ;                          \
    expr    : <float> | <integer> | <bool> | <string>    \
            | <comment> | <symbol> | <sexpr> | <qexpr>   \
            | <list> ;                                   \
    lispy   : /^/ <expr>* /$/ ;                          \
  ",
            Float, Integer, Boolean, String, Comment,
            Symbol, Sexpr, Qexpr, List, Expr, Lispy);

  lenv* e = lenv_new();
  lenv_add_builtins(e);
  char* home = getenv("LISPY_HOME");
  char stdlib_loc[1024];
  if (home) {
    printf("LISPY_HOME=%s\n", home);
    strcpy(stdlib_loc, home);
    strcat(stdlib_loc, "/stdlib.lsp");
  } else {
    printf("Unable to locate LISPY_HOME, defaulting to '.'\n");
    strcpy(stdlib_loc, "./stdlib.lsp");
  }
  lval* args = lval_add(lval_sexpr(), lval_str(stdlib_loc));
  lval* x = builtin_load(e, args);
  if (x->type == LVAL_ERR) {
    lval_println(x);
  }
  lval_del(x);
  if (argc >= 2) {
    for (int i = 1; i < argc; i++) {
      lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
      lval* x = builtin_load(e, args);

      if (x->type == LVAL_ERR) {
        lval_println(x);
      }
      /* args is already deleted by builtin_load */
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
  mpc_cleanup(11,
              Integer, Float, Boolean, String, Comment,
              Symbol, Sexpr, Qexpr, List, Expr, Lispy);
  return 0;
}
