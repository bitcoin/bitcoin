#!/usr/bin/env bash
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

#
# Issue blocks using a local node at a given interval.
#

export HELPSTRING="syntax: $0 <idle time> [--help] [--cmd=<bitcoin-cli path>] [--] [<bitcoin-cli args>]"

if [ $# -lt 1 ]; then
    echo $HELPSTRING
    exit 1
fi

function log()
{
    echo "- $(date +%H:%M:%S): $*"
}

idletime=$1
shift

bcli=
args=

# shellcheck source=contrib/signet/args.sh
source $(dirname $0)/args.sh "$@"

MKBLOCK=$(dirname $0)/mkblock.sh

if [ ! -e "$MKBLOCK" ]; then
    >&2 echo "error: cannot locate mkblock.sh (expected to find in $MKBLOCK"
    exit 1
fi

echo "- checking node status"
conns=$($bcli $args getconnectioncount) || { echo >&2 "node error"; exit 1; }

if [ $conns -lt 1 ]; then
    echo "warning: node is not connected to any other node"
fi

log "node OK with $conns connection(s)"
log "mining at maximum capacity with $idletime second delay between each block"
log "hit ^C to stop"

while true; do
    if [ -e "reorg-list.txt" ]; then
        ./forker.sh $bcli $args
    else
        log "generating next block"
        blockhash=$("$MKBLOCK" "$bcli" $args) || { echo "node error; aborting" ; exit 1; }
        log "mined block $($bcli $args getblockcount) $blockhash to $($bcli $args getconnectioncount) peer(s); idling for $idletime seconds"
    fi
    sleep $idletime
done
