#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"
#include "repl.h"

long min(long x, long y) {
  return x > y ? y : x;
}

long max(long x, long y) {
  return x > y ? x : y;
}

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
	    "Function '%s' cannot define non-symbol. Got %s, Expected %s",
	    ltype_name(syms->cell[i]->type),
	    ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
	  "Function '%s' passed too many arguments for symbols. Got %i, Expected %i",
	  func,
	  syms->count,
	  a->count - 1);

  for (int i = 0; i < syms->count; i++) {
    /* define 'def' globally */
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i + 1]);
    }
    if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }
  }
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
  return builtin_var(e, a, "def");
}

lval* builtin_fun(lenv* e, lval* a) {
  LASSERT_TYPE("fun", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("fun", a, 1, LVAL_QEXPR);
  LASSERT_NUM("fun", a, 2);
  lval* def = a->cell[0];
  lval* body = a->cell[1];
  for (int i = 0; i < def->count; i++) {
    LASSERT(a, (def->cell[i]->type == LVAL_SYM),
	    "Function 'fun' cannot define non-symbol. Got %s, Expected %s",
	    ltype_name(def->cell[i]->type),
	    ltype_name(LVAL_SYM));
  }
  lval* func_name = lval_pop(def, 0);
  lval* args = def;
  lenv_put(e, func_name, lval_lambda(args, body));
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_put(lenv* e, lval* a) {
  return builtin_var(e, a, "=");
}
  
lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_mod(lenv* e, lval* a) {
  return builtin_op(e, a, "%");
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments. Got %i, Expected %i.", a->count, 1);
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'head' passed incorrect type. Got %s, Expected %s",
	  ltype_name(a->cell[0]->type),
	  ltype_name(LVAL_QEXPR));
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}");
  /* take the first argument */
  lval* v = lval_take(a, 0);
  /* delete all the elements after the first */
  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments. Got %i, Expected %i.", a->count, 1);
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'tail' passed incorrect type. Got %s, Expected %s",
	  ltype_name(a->cell[0]->type),
	  ltype_name(LVAL_QEXPR));
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}");

  /* take the first argument */
  lval* v = lval_take(a, 0);

  /* delete the first element and return */
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_lambda(lenv* e, lval* a) {
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
	    "Cannot define a non-symbol. Got %s, Expected %s.",
	    ltype_name(a->cell[0]->cell[i]->type),
	    ltype_name(LVAL_SYM));
  }
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);
  return lval_lambda(formals, body);
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type.");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);
  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
  /* all arguments must be numbers */
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_NUM,
	    "Function %s passed incorrect type for argument %i",
	    op, i);
  }

  /* pop first element */
  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval* y = lval_pop(a, 0);
    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }
    if (strcmp(op, "-") == 0) {
      x->num -= y->num;
    }
    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero");
	break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero");
	break;
      }
      x->num %= y->num;
    }

    lval_del(y);
  }
  lval_del(a);
  return x;
}

int main(int argc, char** argv) {
  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Qexpr    = mpc_new("qexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number : /-?[0-9]+/ ;                               \
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%]+/ ;        \
    sexpr  : '(' <expr>* ')' ;                          \
    qexpr  : '{' <expr>* '}' ;                          \
    expr   : <number> | <symbol> | <sexpr> | <qexpr> ;  \
    lispy  : /^/ <expr>* /$/ ;                          \
  ",
  Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


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
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  return 0;
}
