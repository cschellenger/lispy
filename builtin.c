#include "lispy.h"

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

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_NUM("if", a, 3);
  /* first arg must be condition */
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  /* true branch */
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  /* false branch */
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);
  lval* x;
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  if (a->cell[0]->num) {
    x = lval_eval(e, lval_pop(a, 1));
  } else {
    x = lval_eval(e, lval_pop(a, 2));
  }
  lval_del(a);
  return x;
}

lval* builtin_and(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("&&", a, i, LVAL_NUM);
  }
  int r = 1;
  for (int i = 0; i < a->count; i++) {
    r &= a->cell[i]->num;
    if (!r) {
      break;
    }
  }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_or(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("||", a, i, LVAL_NUM);
  }
  int r = 0;
  for (int i = 0; i < a->count; i++) {
    r |= a->cell[i]->num;
    if (r) {
      break;
    }
  }
  lval_del(a);
  return lval_num(r);
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

lval* builtin_eq(lenv* e, lval* a) {
  return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a) {
  return builtin_cmp(e, a, "!=");
}

lval* builtin_gt(lenv* e, lval* a) {
  return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a) {
  return builtin_ord(e, a, "<");
}

lval* builtin_gte(lenv* e, lval* a) {
  return builtin_ord(e, a, ">=");
}

lval* builtin_lte(lenv* e, lval* a) {
  return builtin_ord(e, a, "<=");
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

lval* builtin_ord(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  /* all arguments must be numbers */
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_NUM,
	    "Function %s passed incorrect type for argument %i",
	    op, i);
  }
  int r;
  lval* x = lval_pop(a, 0);
  lval* y = lval_pop(a, 0);
  if (strcmp(op, ">") == 0) {
    r = x->num > y->num;
  }
  if (strcmp(op, "<") == 0) {
    r = x->num < y->num;
  }
  if (strcmp(op, "==") == 0) {
    r = x->num == y->num;
  }
  if (strcmp(op, ">=") == 0) {
    r = x->num >= y->num;
  }
  if (strcmp(op, "<=") == 0) {
    r = x->num <= y->num;
  }

  lval_del(x);
  lval_del(y);
  lval_del(a);
  return lval_num(r);
}

lval* builtin_not(lenv* e, lval* a) {
  LASSERT_NUM("!", a, 1);
  LASSERT_TYPE("!", a, 1, LVAL_NUM);
  int r = !a->cell[0]->num;
  lval_del(a);
  return lval_num(r);
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  int r;
  if (strcmp(op, "==") == 0) {
    r = lval_eq(a->cell[0], a->cell[1]);
  }
  if (strcmp(op, "!=") == 0) {
    r = !lval_eq(a->cell[0], a->cell[1]);
  }
  lval_del(a);
  return lval_num(r);
}     
