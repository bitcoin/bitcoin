#!/bin/sh

# the autodetected version git tag at the end 
# can screw up manpage output a little bit
BTCVER=$(bitcoin-cli --version | cut -d"-" -f1 | cut -d"v" -f3)

# Create a footer file with copyright content.
# This gets autodetected fine for bitcoind if
# --version-string is not set, but has different
# outcomes for bitcoin-qt and bitcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
bitcoind --version | sed -n '1!p' >> footer.h2m

for cmd in bitcoind bitcoin-qt bitcoin-cli; do
  help2man -N --version-string=${BTCVER} --include=footer.h2m -o ../../doc/man/${cmd}.1 ${cmd}
done

rm -f footer.h2m
