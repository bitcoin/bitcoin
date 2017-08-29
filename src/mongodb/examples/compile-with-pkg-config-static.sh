#!/bin/sh

# -- sphinx-include-start --
gcc -o hello_mongoc hello_mongoc.c $(pkg-config --libs --cflags libmongoc-static-1.0)
