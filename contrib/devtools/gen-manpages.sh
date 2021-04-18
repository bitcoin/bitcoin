#!/usr/bin/env bash
# Copyright (c) 2016-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

WIDECOIND=${WIDECOIND:-$BINDIR/widecoind}
WIDECOINCLI=${WIDECOINCLI:-$BINDIR/widecoin-cli}
WIDECOINTX=${WIDECOINTX:-$BINDIR/widecoin-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/widecoin-wallet}
WIDECOINQT=${WIDECOINQT:-$BINDIR/qt/widecoin-qt}

[ ! -x $WIDECOIND ] && echo "$WIDECOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a WCNVER <<< "$($WIDECOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for widecoind if --version-string is not set,
# but has different outcomes for widecoin-qt and widecoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$WIDECOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $WIDECOIND $WIDECOINCLI $WIDECOINTX $WALLET_TOOL $WIDECOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${WCNVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${WCNVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
