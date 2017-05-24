#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

FLOWD=${FLOWD:-$SRCDIR/flowd}
FLOWCLI=${FLOWCLI:-$SRCDIR/flow-cli}
FLOWTX=${FLOWTX:-$SRCDIR/flow-tx}
FLOWQT=${FLOWQT:-$SRCDIR/qt/flow-qt}

[ ! -x $FLOWD ] && echo "$FLOWD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
FLWVER=($($FLOWCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for flowd if --version-string is not set,
# but has different outcomes for flow-qt and flow-cli.
echo "[COPYRIGHT]" > footer.h2m
$FLOWD --version | sed -n '1!p' >> footer.h2m

for cmd in $FLOWD $FLOWCLI $FLOWTX $FLOWQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${FLWVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${FLWVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
