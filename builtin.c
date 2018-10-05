#include "lispy.h"

lval* builtin_load(lenv* e, lval* a) {
  LASSERT_NUM("load", a, 1);
  LASSERT_TYPE("load", a, 0, LVAL_STR);

  /* Parse File given by string name */
  mpc_result_t r;
  if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {
    /* Read contents */
    lval* expr = lval_read(r.output);
    mpc_ast_delete(r.output);
    /* Evaluate each Expression */
    while (expr->count) {
      lval* x = lval_eval(e, lval_pop(expr, 0));
      /* If Evaluation leads to error print it */
      if (x->type == LVAL_ERR) {
        lval_println(x);
      }
      lval_del(x);
    }
    /* Delete expressions and arguments */
    lval_del(expr);
    lval_del(a);
    /* Return empty list */
    return lval_sexpr();
  } else {
    /* Get Parse Error as String */
    char* err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);
    /* Create new error message using it */
    lval* err = lval_err("Could not load Library %s", err_msg);
    free(err_msg);
    lval_del(a);
    /* Cleanup and return error */
    return err;
  }
}

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot redefine non-symbol. Got %s, Expected %s",
            func,
            ltype_name(syms->cell[i]->type),
            ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
          "Function '%s' passed wrong number of arguments for symbols. Got %i, Expected %i",
          func,
          a->count - 1,
          syms->count);

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
  LASSERT_TYPE("if", a, 0, LVAL_BOOL);
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
    LASSERT_TYPE("&&", a, i, LVAL_BOOL);
  }
  int r = 1;
  for (int i = 0; i < a->count; i++) {
    r &= a->cell[i]->num;
    if (!r) {
      break;
    }
  }
  lval_del(a);
  return lval_bool(r);
}

lval* builtin_or(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("||", a, i, LVAL_BOOL);
  }
  int r = 0;
  for (int i = 0; i < a->count; i++) {
    r |= a->cell[i]->num;
    if (r) {
      break;
    }
  }
  lval_del(a);
  return lval_bool(r);
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
  LASSERT_NUM("head", a, 1);
  lval* arg1 = a->cell[0];
  LASSERT(a, (arg1->type == LVAL_QEXPR || arg1->type == LVAL_STR),
	  "Function 'head' passed incorrect type. Got %s, Expected (%s OR %s)",
	  ltype_name(arg1->type),
	  ltype_name(LVAL_QEXPR),
          ltype_name(LVAL_STR));
  if (arg1->type == LVAL_QEXPR) {
    LASSERT(a, arg1->count != 0, "Function 'head' passed {}");
    /* take the first argument */
    lval* v = lval_take(a, 0);
    /* delete all the elements after the first */
    while (v->count > 1) {
      lval_del(lval_pop(v, 1));
    }
    return v;
  } else if (arg1->type == LVAL_STR) {
    LASSERT (a, (strlen(arg1->str) != 0),
             "Function 'head' passed empty string");
    char* first = malloc(sizeof(char) + 1);
    first[0] = arg1->str[0];
    first[1] = '\0';
    lval_del(a);
    return lval_str(first);
  } else {
    return lval_err("Function 'head' type not handled: %s", ltype_name(arg1->type));
  }
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT_NUM("tail", a, 1);
  lval* arg1 = a->cell[0];
  LASSERT(a, (arg1->type == LVAL_QEXPR || arg1->type == LVAL_STR),
	  "Function 'tail' passed incorrect type. Got %s, Expected (%s OR %s)",
	  ltype_name(a->cell[0]->type),
	  ltype_name(LVAL_QEXPR),
          ltype_name(LVAL_STR));
  if (arg1->type == LVAL_QEXPR) {
    LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}");
    /* take the first argument */
    lval* v = lval_take(a, 0);
    /* delete the first element and return */
    lval_del(lval_pop(v, 0));
    return v;
  } else if (arg1->type == LVAL_STR) {
    LASSERT (a, (strlen(arg1->str) != 0),
             "Function 'head' passed empty string");
    char* rest = malloc(sizeof(char) * strlen(arg1->str));
    char* rp = arg1->str + (sizeof(char));
    strcpy(rest, rp);
    rest[strlen(rest)] = '\0';
    lval_del(a);
    return lval_str(rest);
  } else {
    return lval_err("Function 'tail' type not handled: %s", ltype_name(arg1->type));
  }
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
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, (a->cell[i]->type == LVAL_QEXPR || a->cell[i]->type == LVAL_STR),
            "Function 'join' cannot operate on type: %s",
            ltype_name(a->cell[i]->type));
  }

  lval* x = lval_pop(a, 0);
  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}

/* performs the provided operation against cells in the lval a */
lval* builtin_op(lenv* e, lval* a, char* op) {
  /* all arguments must be numbers */
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, ltype_numeric(a->cell[i]->type),
	    "Function %s passed incorrect type for argument %i",
	    op, i);
  }

  /* pop first element */
  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    if (x->type == LVAL_INT) {
      x->num = -x->num;
    } else if (x->type == LVAL_FLOAT) {
      x->fnum = -x->fnum;
    }
  }

  while (a->count > 0) {
    lval* y = lval_pop(a, 0);
    if (x->type != y->type) {
      /* We must convert to the same type. Since one is a float, the end result
       * must be a float as well.
       */
      x = lval_int_to_float(x);
      y = lval_int_to_float(y);
    }
    if (strcmp(op, "+") == 0) {
      if (x->type == LVAL_INT) {
        x->num += y->num;
      } else if (x->type == LVAL_FLOAT) {
        x->fnum += y->fnum;
      }
    }
    if (strcmp(op, "-") == 0) {
      if (x->type == LVAL_INT) {
        x->num -= y->num;
      } else if (x->type == LVAL_FLOAT) {
        x->fnum -= y->fnum;
      }
    }
    if (strcmp(op, "*") == 0) {
      if (x->type == LVAL_INT) {
        x->num *= y->num;
      } else if (x->type == LVAL_FLOAT) {
        x->fnum *= y->fnum;
      }
    }
    if (strcmp(op, "/") == 0) {
      if ((y->type == LVAL_INT && y->num == 0)
          || (y->type == LVAL_INT && y->fnum == (double) 0.0)) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero");
	break;
      }
      if (x->type == LVAL_INT) {
        x->num /= y->num;
      } else if (y->type == LVAL_FLOAT) {
        x->fnum /= y->fnum;
      }
    }
    if (strcmp(op, "%") == 0) {
      if ((y->type == LVAL_INT && y->num == 0)
          || (y->type == LVAL_FLOAT && y->fnum == (double) 0.0)) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero");
	break;
      }
      if (x->type == LVAL_INT) {
        x->num %= y->num;
      } else if (x->type == LVAL_FLOAT) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Cannot perform floating point modulus");
      }
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
    LASSERT(a, ltype_numeric(a->cell[i]->type),
	    "Function %s passed incorrect type for argument %i",
	    op, i);
  }
  int r;
  lval* x = lval_pop(a, 0);
  lval* y = lval_pop(a, 0);
  if (x->type != y->type) {
    /* type conversion required */
    x = lval_int_to_float(x);
    y = lval_int_to_float(y);
  }
  if (strcmp(op, ">") == 0) {
    if (x->type == LVAL_INT) {
      r = x->num > y->num;
    } else {
      r = x->fnum > y->fnum;
    }
  }
  if (strcmp(op, "<") == 0) {
    if (x->type == LVAL_INT) {
      r = x->num < y->num;
    } else {
      r = x->fnum < y->fnum;
    }
  }
  if (strcmp(op, "==") == 0) {
    if (x->type == LVAL_INT) {
      r = x->num == y->num;
    } else {
      r = x->fnum == y->fnum;
    }
  }
  if (strcmp(op, ">=") == 0) {
    if (x->type == LVAL_INT) {
      r = x->num >= y->num;
    } else {
      r = x->fnum >= y->fnum;
    }
  }
  if (strcmp(op, "<=") == 0) {
    if (x->type == LVAL_INT) {
      r = x->num <= y->num;
    } else {
      r = x->fnum <= y->fnum;
    }
  }

  lval_del(x);
  lval_del(y);
  lval_del(a);
  return lval_bool(r);
}

lval* builtin_not(lenv* e, lval* a) {
  LASSERT_NUM("!", a, 1);
  LASSERT_TYPE("!", a, 1, LVAL_BOOL);
  int r = !a->cell[0]->num;
  lval_del(a);
  return lval_bool(r);
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
  return lval_bool(r);
}     
