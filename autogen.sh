#!/bin/sh
set -e
srcdir="$(dirname $0)"
cd "$srcdir"
autoreconf --install --force
