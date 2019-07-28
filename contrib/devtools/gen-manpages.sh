#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

BITGREEND=${BITGREEND:-$BINDIR/bitgreend}
BITGREENCLI=${BITGREENCLI:-$BINDIR/bitgreen-cli}
BITGREENTX=${BITGREENTX:-$BINDIR/bitgreen-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/bitgreen-wallet}
BITGREENQT=${BITGREENQT:-$BINDIR/qt/bitgreen-qt}

[ ! -x $BITGREEND ] && echo "$BITGREEND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a BTCVER <<< "$($BITGREENCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for bitgreend if --version-string is not set,
# but has different outcomes for bitgreen-qt and bitgreen-cli.
echo "[COPYRIGHT]" > footer.h2m
$BITGREEND --version | sed -n '1!p' >> footer.h2m

for cmd in $BITGREEND $BITGREENCLI $BITGREENTX $WALLET_TOOL $BITGREENQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${BTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${BTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
