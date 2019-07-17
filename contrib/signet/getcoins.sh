#!/usr/bin/env bash
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

#
# Get coins from Signet Faucet
#

export VARCHECKS='
        if [ "$varname" = "cmd" ]; then
            bcli=$value;
        elif [ "$varname" = "faucet" ]; then
            faucet=$value;
        elif [ "$varname" = "addr" ]; then
            addr=$value;
        elif [ "$varname" = "password" ]; then
            password=$value;
    '
export HELPSTRING="syntax: $0 [--help] [--cmd=<bitcoin-cli path>] [--faucet=<faucet URL>] [--addr=<signet bech32 address>] [--password=<faucet password>] [--] [<bitcoin-cli args>]"

bcli=
args=
password=
addr=
faucet="https://signet.bc-2.jp/claim"

# shellcheck source=contrib/signet/args.sh
source $(dirname $0)/args.sh "$@"

if [ "$addr" = "" ]; then
    # get address for receiving coins
    addr=$($bcli $args getnewaddress faucet bech32) || { echo >&2 "for help, type: $0 --help"; exit 1; }
fi

# shellcheck disable=SC2015
command -v "curl" > /dev/null \
&& curl -X POST -d "address=$addr&password=$password" $faucet \
|| wget -qO - --post-data "address=$addr&password=$password" $faucet

echo
