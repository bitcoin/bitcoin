#!/bin/sh
cmake -DCHECK=on -DSEED= -DALLOC=DYNAMIC -DDEBUG=on -DSHLIB=off -DTESTS=1 -DBENCH=1 -DSIMUL="/usr/bin/valgrind" -DSIMAR="--tool=memcheck --track-origins=yes --leak-check=full" $1
