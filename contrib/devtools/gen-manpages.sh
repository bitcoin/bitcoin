#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

TALKCOIND=${TALKCOIND:-$BINDIR/talkcoind}
TALKCOINCLI=${TALKCOINCLI:-$BINDIR/talkcoin-cli}
TALKCOINTX=${TALKCOINTX:-$BINDIR/talkcoin-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/talkcoin-wallet}
TALKCOINQT=${TALKCOINQT:-$BINDIR/qt/talkcoin-qt}

[ ! -x $TALKCOIND ] && echo "$TALKCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
TALKVER=($($TALKCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for talkcoind if --version-string is not set,
# but has different outcomes for talkcoin-qt and talkcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$TALKCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $TALKCOIND $TALKCOINCLI $TALKCOINTX $WALLET_TOOL $TALKCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${TALKVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${TALKVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
