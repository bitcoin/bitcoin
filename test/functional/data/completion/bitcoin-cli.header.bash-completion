# Copyright (c) 2012-2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# call $bitcoin-cli for RPC
_bitcoin_rpc() {
    # determine already specified args necessary for RPC
    local rpcargs=()
    for i in ${COMP_LINE}; do
        case "$i" in
            -conf=*|-datadir=*|-regtest|-rpc*|-testnet|-testnet4)
                rpcargs=( "${rpcargs[@]}" "$i" )
                ;;
        esac
    done
    $bitcoin_cli "${rpcargs[@]}" "$@"
}

_bitcoin_cli() {
    local cur prev words=() cword
    local bitcoin_cli

    # save and use original argument to invoke bitcoin-cli for -help, help and RPC
    # as bitcoin-cli might not be in $PATH
    bitcoin_cli="$1"

    COMPREPLY=()
    _get_comp_words_by_ref -n = cur prev words cword

