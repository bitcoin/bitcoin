# bash programmable completion for bitcoin(1) wrapper
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# wrapper to delegate completion to the real function
_bitcoin_wrap() {
    local delegate="$1" shift_count="$2"
    local func="_${delegate//-/_}"
    local words cword dir file

    # load completion script if function not yet defined
    if ! declare -F $func >/dev/null; then
        dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
        file="$(grep -l -E "complete[[:space:]]+-F[[:space:]]+${func}" "$dir"/* 2>/dev/null | head -n1)"
        [ -z "$file" ] && file="$(grep -l -E "^[[:space:]]*${func}[[:space:]]*\(\)" "$dir"/* 2>/dev/null | head -n1)"
        [ -n "$file" ] && source "$file" || return 0
    fi

    _get_comp_words_by_ref -n = words cword

    COMP_WORDS=( "$delegate" "${words[@]:$shift_count}" )
    COMP_CWORD=$(( cword - (shift_count - 1) ))
    COMP_LINE="${COMP_WORDS[@]}"
    COMP_POINT=${#COMP_LINE}
    $func "$delegate"
}

_bitcoin() {
    local cur prev words cword
    local bitcoin subcmd offset

    # save and use original argument to invoke bitcoin for help
    # it might not be in $PATH
    bitcoin="$1"

    COMPREPLY=()
    _get_comp_words_by_ref -n = cur prev words cword

    case "${words[1]}" in
        -m|-M|--multiprocess|--monolithic)
            subcmd="${words[2]}"
            offset=3
            ;;
        *)
            subcmd="${words[1]}"
            offset=2
            ;;
    esac

    case "$subcmd" in
        gui|node)
            _bitcoin_wrap bitcoind "$offset"
            return 0
            ;;
        rpc)
            _bitcoin_wrap bitcoin-cli "$offset"
            return 0
            ;;
        tx)
            _bitcoin_wrap bitcoin-tx "$offset"
            return 0
            ;;
    esac

    case "$cur" in
        -*=*)   # prevent nonsense completions
            return 0
            ;;
        *)
            local options commands

            # only parse help if sensible
            if [[ -z "$cur" || "$cur" =~ ^- ]]; then
                options=$($bitcoin help 2>&1 | awk '{ for(i=1;i<=NF;i++) if ($i~/^--/) { sub(/=.*/, "=",$i); print $i } }' )
            fi
            if [[ -z "$cur" || "$cur" =~ ^[a-z] ]]; then
                commands=$($bitcoin help 2>/dev/null | awk '$1 ~ /^[a-z]/ { print $1; }')
            fi

            COMPREPLY=( $( compgen -W "$options $commands" -- "$cur" ) )

            # Prevent space if an argument is desired
            if [[ $COMPREPLY == *= ]]; then
                compopt -o nospace
            fi
            return 0
            ;;
    esac

} &&
complete -F _bitcoin bitcoin

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
# ex: ts=4 sw=4 et filetype=sh
