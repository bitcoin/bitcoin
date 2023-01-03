#!/usr/bin/env bash
# Copyright (c) 2016-2019 The Buttcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

ButtcoinD=${ButtcoinD:-$BINDIR/Buttcoind}
ButtcoinCLI=${ButtcoinCLI:-$BINDIR/Buttcoin-cli}
ButtcoinTX=${ButtcoinTX:-$BINDIR/Buttcoin-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/Buttcoin-wallet}
ButtcoinQT=${ButtcoinQT:-$BINDIR/qt/Buttcoin-qt}

[ ! -x $ButtcoinD ] && echo "$ButtcoinD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a BTCVER <<< "$($ButtcoinCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for Buttcoind if --version-string is not set,
# but has different outcomes for Buttcoin-qt and Buttcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$ButtcoinD --version | sed -n '1!p' >> footer.h2m

for cmd in $ButtcoinD $ButtcoinCLI $ButtcoinTX $WALLET_TOOL $ButtcoinQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${BTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${BTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
