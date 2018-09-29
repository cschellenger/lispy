repl: repl.c repl.h lvals.c lenv.c
	cc -std=c99 -Wall repl.c mpc.c lvals.c lenv.c -ledit -lm -o repl
