# bash programmable completion for tortoisecoind(1) and tortoisecoin-qt(1)
# Copyright (c) 2012-2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

_tortoisecoind() {
    local cur prev words=() cword
    local tortoisecoind

    # save and use original argument to invoke tortoisecoind for -help
    # it might not be in $PATH
    tortoisecoind="$1"

    COMPREPLY=()
    _get_comp_words_by_ref -n = cur prev words cword

    case "$cur" in
        -conf=*|-pid=*|-loadblock=*|-rpccookiefile=*|-wallet=*)
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

            # only parse -help if sensible
            if [[ -z "$cur" || "$cur" =~ ^- ]]; then
                local helpopts
                helpopts=$($tortoisecoind -help 2>&1 | awk '$1 ~ /^-/ { sub(/=.*/, "="); print $1 }' )
                COMPREPLY=( $( compgen -W "$helpopts" -- "$cur" ) )
            fi

            # Prevent space if an argument is desired
            if [[ $COMPREPLY == *= ]]; then
                compopt -o nospace
            fi
            return 0
            ;;
    esac
} &&
complete -F _tortoisecoind tortoisecoind tortoisecoin-qt

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
# ex: ts=4 sw=4 et filetype=sh
