#!/usr/bin/env bash
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

args="-signet"
eoc=

command -v bitcoin-cli > /dev/null \
&& bcli="bitcoin-cli" \
|| bcli="$(dirname $0)/../../src/bitcoin-cli"

if [ "$VARCHECKS" = "" ]; then
    VARCHECKS='if [ "$varname" = "cmd" ]; then bcli=$value;'
fi

# compatibility; previously the bitcoin-cli path was given as first argument
if [[ "$1" != "" && "${1:$((${#1}-11))}" = "bitcoin-cli" ]]; then
    echo "using $1 as bcli"
    bcli=$1
    shift
fi

for i in "$@"; do
    if [ $eoc ]; then
        args="$args $i"
    elif [ "$i" = "--" ]; then
        # end of commands; rest into args for bitcoin-cli
        eoc=1
        continue
    elif [ "${i:0:2}" = "--" ]; then
        # command
        j=${i:2}
        if [ "$j" = "help" ]; then
            >&2 echo -e $HELPSTRING
            exit 1
        fi
        export varname=${j%=*}
        export value=${j#*=}
        eval $VARCHECKS '
        else
            >&2 echo "unknown parameter $varname (from \"$i\"); for help, type: $0 --help"
            exit 1
        fi'
    else
        # arg
        args="$args $i"
    fi
done

if ! [ -e "$bcli" ]; then
    command -v "$bcli" >/dev/null 2>&1 || { echo >&2 "error: unable to find bitcoin-cli binary: $bcli"; exit 1; }
fi
