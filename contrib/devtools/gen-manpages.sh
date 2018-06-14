#!/bin/bash

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

DOBBSCOIND=${DOBBSCOIND:-$SRCDIR/dobbscoind}
DOBBSCOINCLI=${DOBBSCOINCLI:-$SRCDIR/dobbscoin-cli}
DOBBSCOINTX=${DOBBSCOINTX:-$SRCDIR/dobbscoin-tx}
DOBBSCOINQT=${DOBBSCOINQT:-$SRCDIR/qt/dobbscoin-qt}

[ ! -x $DOBBSCOIND ] && echo "$DOBBSCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
SLACKVER=($($DOBBSCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for bitcoind if --version-string is not set,
# but has different outcomes for bitcoin-qt and bitcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$DOBBSCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $DOBBSCOIND $DOBBSCOINCLI $DOBBSCOINTX $DOBBSCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${SLACKVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${SLACKVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
