#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

# -- sphinx-include-start --
gcc -o hello_bson hello_bson.c $(pkg-config --libs --cflags libbson-1.0)
