# bash programmable completion for bitcoin-cli(1)
# Copyright (c) 2012-2022 The Bitcoin Core developers
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

    if ((cword > 5)); then
        case ${words[cword-5]} in
            sendtoaddress)
                COMPREPLY=( $( compgen -W "true false" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    if ((cword > 4)); then
        case ${words[cword-4]} in
            importaddress|listtransactions|setban)
                COMPREPLY=( $( compgen -W "true false" -- "$cur" ) )
                return 0
                ;;
            signrawtransactionwithkey|signrawtransactionwithwallet)
                COMPREPLY=( $( compgen -W "ALL NONE SINGLE ALL|ANYONECANPAY NONE|ANYONECANPAY SINGLE|ANYONECANPAY" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    if ((cword > 3)); then
        case ${words[cword-3]} in
            addmultisigaddress)
                return 0
                ;;
            getbalance|gettxout|importaddress|importpubkey|importprivkey|listreceivedbyaddress|listsinceblock)
                COMPREPLY=( $( compgen -W "true false" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    if ((cword > 2)); then
        case ${words[cword-2]} in
            addnode)
                COMPREPLY=( $( compgen -W "add remove onetry" -- "$cur" ) )
                return 0
                ;;
            setban)
                COMPREPLY=( $( compgen -W "add remove" -- "$cur" ) )
                return 0
                ;;
            fundrawtransaction|getblock|getblockheader|getmempoolancestors|getmempooldescendants|getrawtransaction|gettransaction|listreceivedbyaddress|sendrawtransaction)
                COMPREPLY=( $( compgen -W "true false" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    case "$prev" in
        backupwallet|dumpwallet|importwallet)
            _filedir
            return 0
            ;;
        getaddednodeinfo|getrawmempool|lockunspent)
            COMPREPLY=( $( compgen -W "true false" -- "$cur" ) )
            return 0
            ;;
        getbalance|getnewaddress|listtransactions|sendmany)
            return 0
            ;;
    esac

    case "$cur" in
        -conf=*)
            cur="${cur#*=}"
            _filedir
            return 0
            ;;
        -datadir=*)
            cur="${cur#*=}"
            _filedir -d
            return 0
            ;;
        -*=*)	# prevent nonsense completions
            return 0
            ;;
        *)
            local helpopts commands

            # only parse -help if senseful
            if [[ -z "$cur" || "$cur" =~ ^- ]]; then
                helpopts=$($bitcoin_cli -help 2>&1 | awk '$1 ~ /^-/ { sub(/=.*/, "="); print $1 }' )
            fi

            # only parse help if senseful
            if [[ -z "$cur" || "$cur" =~ ^[a-z] ]]; then
                commands=$(_bitcoin_rpc help 2>/dev/null | awk '$1 ~ /^[a-z]/ { print $1; }')
            fi

            COMPREPLY=( $( compgen -W "$helpopts $commands" -- "$cur" ) )

            # Prevent space if an argument is desired
            if [[ $COMPREPLY == *= ]]; then
                compopt -o nospace
            fi
            return 0
            ;;
    esac
} &&
complete -F _bitcoin_cli bitcoin-cli

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
# ex: ts=4 sw=4 et filetype=sh
