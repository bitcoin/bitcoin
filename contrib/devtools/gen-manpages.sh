#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

BITCOINTALKCOIND=${BITCOINTALKCOIND:-$BINDIR/bitcointalkcoind}
BITCOINTALKCOINCLI=${BITCOINTALKCOINCLI:-$BINDIR/bitcointalkcoin-cli}
BITCOINTALKCOINTX=${BITCOINTALKCOINTX:-$BINDIR/bitcointalkcoin-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/bitcointalkcoin-wallet}
BITCOINTALKCOINQT=${BITCOINTALKCOINQT:-$BINDIR/qt/bitcointalkcoin-qt}

[ ! -x $BITCOINTALKCOIND ] && echo "$BITCOINTALKCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
TALKVER=($($BITCOINTALKCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for bitcointalkcoind if --version-string is not set,
# but has different outcomes for bitcointalkcoin-qt and bitcointalkcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$BITCOINTALKCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $BITCOINTALKCOIND $BITCOINTALKCOINCLI $BITCOINTALKCOINTX $WALLET_TOOL $BITCOINTALKCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${TALKVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${TALKVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
