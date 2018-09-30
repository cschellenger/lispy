#include "mpc.h"

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM,
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef lval* (*lbuiltin)(lenv*, lval*);

struct lval {
  int type;
  
  long num;
  char* err;
  char* sym;
  
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  
  int count;
  struct lval** cell;
};

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};

lval* builtin_add(lenv* e, lval* a);
lval* builtin_and(lenv* e, lval* a);
lval* builtin_cmp(lenv* e, lval* a, char* op);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_fun(lenv* e, lval* a);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_gte(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_if(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_lambda(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_lte(lenv* e, lval* a);
lval* builtin_mod(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
lval* builtin_not(lenv* e, lval* a);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_or(lenv* e, lval* a);
lval* builtin_ord(lenv* e, lval* a, char* op);
lval* builtin_put(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_var(lenv* e, lval* a, char* func);

void  lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void  lenv_add_builtins(lenv* e);
lenv* lenv_copy(lenv* e);
void  lenv_def(lenv* e, lval* k, lval* v);
void  lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
lenv* lenv_new(void);
void  lenv_put(lenv* e, lval* k, lval* v);

char* ltype_name(int t);

lval* lval_add(lval* v, lval* x);
lval* lval_call(lenv* e, lval* f, lval* a);
lval* lval_copy(lval* v);
void  lval_del(lval* v);
int   lval_eq(lval* x, lval* y);
lval* lval_err(char* fmt, ...);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
void  lval_expr_print(lval* v, char open, char close);
lval* lval_fun(lbuiltin func);
lval* lval_join(lval* x, lval* y);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_num(long x);
lval* lval_pop(lval* v, int i);
void  lval_print(lval* v);
void  lval_println(lval* v);
lval* lval_qexpr(void);
lval* lval_read(mpc_ast_t* t);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_sexpr(void);
lval* lval_sym(char* s);
lval* lval_take(lval* v, int i);

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. " \
    "Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. " \
    "Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);
