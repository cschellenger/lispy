#include "lispy.h"

char* ltype_name(int t) {
  switch (t) {
  case LVAL_FUN:   return "Function";
  case LVAL_INT:   return "Integer";
  case LVAL_FLOAT: return "Float";
  case LVAL_BOOL:  return "Boolean";
  case LVAL_STR:   return "String";
  case LVAL_ERR:   return "Error";
  case LVAL_OK:    return "OK";
  case LVAL_SYM:   return "Symbol";
  case LVAL_SEXPR: return "S-Expression";
  case LVAL_QEXPR: return "Q-Expression";
  default: return "Unknown";
  }
}

int ltype_numeric(int t) {
  return t == LVAL_INT || t == LVAL_FLOAT;
}

int ltype_expr(int t) {
  return t == LVAL_SEXPR || t == LVAL_QEXPR;
}

int ltype_expr_or_str(int t) {
  return t == LVAL_STR || ltype_expr(t);
}

int lval_eq(lval* x, lval* y) {
  if (x->type != y->type) {
    return 0;
  }

  /* Should we allow comparing int and float? */
  switch(x->type) {
  case LVAL_BOOL:
  case LVAL_INT: return x->num == y->num;
  /* Should we compare with epsilon? */
  case LVAL_FLOAT: return x->fnum == y->fnum;
  case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
  case LVAL_STR: return (strcmp(x->str, y->str) == 0);
  case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
  case LVAL_FUN:
    if (x->builtin || y->builtin) {
      return x->builtin == y->builtin;
    } else {
      return lval_eq(x->formals, y->formals)
        && lval_eq(x->body, y->body);
    }
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    if (x->count != y->count) {
      return 0;
    }
    for (int i = 0; i < x->count; i++) {
      if (!lval_eq(x->cell[i], y->cell[i])) {
        return 0;
      }
    }
    /* nothing was unequal */
    return 1;
  default: break;
  }
  return 0;
}

lval* lval_int(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_INT;
  v->num = x;
  return v;
}

lval* lval_int_to_float(lval* x) {
  if (x->type == LVAL_FLOAT) {
    return x;
  }
  x->type = LVAL_FLOAT;
  x->fnum = (double) x->num;
  return x;
}

lval* lval_float(double x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FLOAT;
  v->fnum = x;
  return v;
}

lval* lval_float_to_int(lval* x) {
  if (x->type == LVAL_INT) {
    return x;
  }
  x->type = LVAL_INT;
  x->num = (long) x->fnum;
  return x;
}

lval* lval_bool(int b) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_BOOL;
  v->num = b;
  return v;
}

lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  va_list va;
  va_start(va, fmt);
  
  v->err = malloc(512);
  vsnprintf(v->err, 511, fmt, va);
  v->err = realloc(v->err, strlen(v->err) + 1);

  va_end(va);

  return v;
}

lval* lval_str(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_STR;
  v->str = malloc(strlen(s) + 1);
  strcpy(v->str, s);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_ok(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_OK;
  return v;
}

lval* lval_fun(lbuiltin func) {
  lval* v =  malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {

  case LVAL_BOOL:
  case LVAL_INT:
  case LVAL_FLOAT:
  case LVAL_OK:
    break;

  case LVAL_ERR: free(v->err);
    break;

  case LVAL_SYM: free(v->sym);
    break;

  case LVAL_STR: free(v->str);
    break;

  case LVAL_FUN:
    if (!v->builtin) {
      lenv_del(v->env);
      lval_del(v->formals);
      lval_del(v->body);
    }
    break;
    
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;

  default: printf("Unexpected type\n");
  }
  free(v);
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  if (f->builtin) {
    return f->builtin(e, a);
  }

  int given = a->count;
  int total = f->formals->count;

  while (a->count) {

    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. Got %i, Expected %i.",
                      given, total);
    }

    /* pop first symbol from the formals */
    lval* syms = lval_pop(f->formals, 0);

    if (strcmp(syms->sym, "&") == 0) {
      /* ensure & is followed by another symbol */
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. "
                        "Symbol '&' not followed by a single symbol.");
      }
      /* next formal symbol should by bound to remaining arguments */
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(syms);
      lval_del(nsym);
      break;
    }

    /* pop next argument from the list */
    lval* val = lval_pop(a, 0);

    /* bind a copy into the function's environment */
    lenv_put(f->env, syms, val);

    /* delete the symbol and value */
    lval_del(syms);
    lval_del(val);
  }

  /* argument list is now bound so clean up */
  lval_del(a);

  /* if '&' remains in formal list bind to empty list */
  if (f->formals->count > 0 &&
      strcmp(f->formals->cell[0]->sym, "&") == 0) {

    if (f->formals->count != 2) {
      return lval_err("Function format invalid. "
                      "Symbol '&' not followed by a single symbol.");
    }

    /* pop and delete '&' symbol */
    lval_del(lval_pop(f->formals, 0));

    /* pop next symbol and create empty list */
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();

    /* bind to environment and delete */
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  /* if all formals have been bound, evaluate */
  if (f->formals->count == 0) {

    /* environment parent = evaluation environment */
    f->env->par = e;

    /* evaluate and return */
    return builtin_eval(f->env, lval_add(lval_sexpr(),
                                         lval_copy(f->body)));
  } else {
    /* otherwise return partially evaluated function */
    return lval_copy(f);
  }
}

lval* lval_read_int(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_int(x) : lval_err("invalid integer");
}

lval* lval_read_float(mpc_ast_t* t) {
  errno = 0;
  double x = strtod(t->contents, NULL);
  return errno != ERANGE ? lval_float(x) : lval_err("invalid float");
}

lval* lval_read_bool(mpc_ast_t* t) {
  if (strcmp(t->contents, "true") == 0) {
    return lval_bool(1);
  }
  if (strcmp(t->contents, "false") == 0) {
    return lval_bool(0);
  }
  return lval_err("Unable to parse boolean value: %s", t->contents);
}

lval* lval_read_str(mpc_ast_t* t) {
  /* replace the last quote with null terminator */
  t->contents[strlen(t->contents) - 1] = '\0';
  char* unescaped = malloc(strlen(t->contents + 1) + 1);
  /* copy to unescaped, cutting off first quote */
  strcpy(unescaped, t->contents + 1);
  unescaped = mpcf_unescape(unescaped);
  lval* str = lval_str(unescaped);
  free(unescaped);
  return str;
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  v->builtin = NULL;

  v->env = lenv_new();

  v->formals = formals;
  v->body = body;
  return v;
}

lval* lval_read(mpc_ast_t* t) {
  /* Handle symbols and numbers */
  if (strstr(t->tag, "integer")) {
    return lval_read_int(t);
  }
  if (strstr(t->tag, "float")) {
    return lval_read_float(t);
  }
  if (strstr(t->tag, "bool")) {
    return lval_read_bool(t);
  }
  if (strstr(t->tag, "string")) {
    return lval_read_str(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  /* if root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "qexpr")) {
    x = lval_qexpr();
  }
  if (strstr(t->tag, "list")) {
    x = lval_sexpr();
    x = lval_add(x, lval_fun(builtin_list));
  }

  /* fill in sexpr / qexpr */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "{") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "}") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "[") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "]") == 0) {
      continue;
    }
    if (strstr(t->children[i]->tag, "comment")) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}
 
void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i=0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != v->count-1) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print_str(lval* v) {
  char* escaped = malloc(strlen(v->str) + 1);
  strcpy(escaped, v->str);
  escaped = mpcf_escape(escaped);
  printf("\"%s\"", escaped);
  free(escaped);
}

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_INT: printf("%li", v->num);
    break;

  case LVAL_FLOAT: printf("%.3f", v->fnum);
    break;

  case LVAL_BOOL: printf("%s", v->num ? "true" : "false");
    break;

  case LVAL_ERR: printf("Error: %s", v->err);
    break;

  case LVAL_OK: printf("OK");
    break;

  case LVAL_SYM: printf("%s", v->sym);
    break;

  case LVAL_STR: lval_print_str(v);
    break;

  case LVAL_SEXPR: lval_expr_print(v, '(', ')');
    break;
  case LVAL_QEXPR: lval_expr_print(v, '{', '}');
    break;
  case LVAL_FUN:
    if (v->builtin) {
      printf("<function>");
    } else {
      printf("(\\ ");
      lval_print(v->formals);
      putchar(' ');
      lval_print(v->body);
      putchar(')');
    }
    break;
  }
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch(v->type) {

  case LVAL_FUN:
    if (v->builtin) {
      x->builtin = v->builtin;
    } else {
      x->builtin = NULL;
      x->env = lenv_copy(v->env);
      x->formals = lval_copy(v->formals);
      x->body = lval_copy(v->body);
    }
    break;

  case LVAL_BOOL:
  case LVAL_INT:
    x->num = v->num;
    break;

  case LVAL_FLOAT:
    x->fnum = v->fnum;
    break;
    
  case LVAL_ERR:
    x->err = malloc(strlen(v->err) + 1);
    strcpy(x->err, v->err);
    break;

  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym) + 1);
    strcpy(x->sym, v->sym);
    break;

  case LVAL_STR:
    x->str = malloc(strlen(v->str) + 1);
    strcpy(x->str, v->str);
    break;

  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval*) * x->count);
    for (int i = 0; i < x->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
    break;
  }
  return x;
}

lval* lval_pop(lval* v, int i) {
  /* find item at "i" */
  lval* x = v->cell[i];
  /* shift memory after item "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  /* decrement count of items in list */
  v->count--;
  /* reallocate memory used*/
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  /* evaluate each cell in v */
  for (int i = 0; i < v->count; i++) {
    int macro = 0;
    if (v->cell[i]->type == LVAL_SYM) {
      macro = (strcmp(v->cell[i]->sym, "defmacro") == 0);
    }
    v->cell[i] = lval_eval(e, v->cell[i]);
    if (macro) {
      break;
    }
  }
  
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }
  if (v->count == 0) {
    return v;
  }  
  if (v->count == 1) {
    return lval_eval(e, lval_take(v, 0));
  }
  
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval* err = lval_err("S-Expression starts with incorrect type. Got %s, Expected %s.",
                         ltype_name(f->type),
                         ltype_name(LVAL_FUN));
    lval_del(f);
    lval_del(v);
    return err;
  }
  lval* result = lval_call(e, f, v);
  return result;
}

lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
  }
  return v;
}

lval* lval_join(lval* x, lval* y) {
  /* for each cell in 'y' add it to 'x' */
  if (x->type == LVAL_STR && y->type == LVAL_STR) {
    int orig = strlen(x->str);
    x->str = realloc(x->str, sizeof(char) * (orig + strlen(y->str) + 1));
    char* ystart = x->str + (orig * sizeof(char));
    strcpy(ystart, y->str);
    x->str[strlen(x->str)] = '\0';
  } else {
    /* if both are not q-expressions, implicitly convert whichever is not */
    if (x->type == LVAL_STR) {
      x = lval_add(lval_qexpr(), x);
    }
    if (y->type == LVAL_STR) {
      y = lval_add(lval_qexpr(), y);
    }
    while (y->count) {
      x = lval_add(x, lval_pop(y, 0));
    }
  }
  lval_del(y);
  return x;
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}
