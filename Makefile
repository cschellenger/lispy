repl: lispy.h repl.c lvals.c lenv.c builtin.c
	cc -g -std=c99 -Wall repl.c mpc.c lvals.c lenv.c builtin.c -ledit -lm -o repl
