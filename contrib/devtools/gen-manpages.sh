#!/usr/bin/env bash
# Copyright (c) 2016-2019 The XBit Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

XBITD=${XBITD:-$BINDIR/xbitd}
XBITCLI=${XBITCLI:-$BINDIR/xbit-cli}
XBITTX=${XBITTX:-$BINDIR/xbit-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/xbit-wallet}
XBITQT=${XBITQT:-$BINDIR/qt/xbit-qt}

[ ! -x $XBITD ] && echo "$XBITD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a XBTVER <<< "$($XBITCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for xbitd if --version-string is not set,
# but has different outcomes for xbit-qt and xbit-cli.
echo "[COPYRIGHT]" > footer.h2m
$XBITD --version | sed -n '1!p' >> footer.h2m

for cmd in $XBITD $XBITCLI $XBITTX $WALLET_TOOL $XBITQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${XBTVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${XBTVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
