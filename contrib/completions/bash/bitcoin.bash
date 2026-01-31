#!/usr/bin/env bash

_bitcoin_complete() {
    local cur prev words cword split
    _init_completion -s || return

    # get the main command (node, rpc, tx, etc.)
    local main_cmd=""
    local i=1
    while [[ $i -lt $cword ]]; do
        case "${words[$i]}" in
            -*)
                ;;
            *)
                main_cmd="${words[$i]}"
                break
                ;;
        esac
        ((i++))
    done

    # handle global options before subcommand
    if [[ -z "$main_cmd" ]]; then
        case "$cur" in
            -*)
                COMPREPLY=($(compgen -W "-h --help -v --version -m --multiprocess -M --monolithic" -- "$cur"))
                return
                ;;
            *)
                # get available subcommands from bitcoin --help
                local subcommands
                if command -v bitcoin >/dev/null 2>&1; then
                    subcommands=$(bitcoin --help 2>/dev/null | sed -n '/^Commands:/,/^$/p' | grep -E "^\s+[a-z]" | awk '{print $1}' | tr '\n' ' ')
                fi
                # fallback to common subcommands if bitcoin not available
                if [[ -z "$subcommands" ]]; then
                    subcommands="gui node rpc wallet tx help bench chainstate test test-gui"
                fi
                COMPREPLY=($(compgen -W "$subcommands" -- "$cur"))
                return
                ;;
        esac
    fi

    # delegate to specific completion functions based on subcommand
    case "$main_cmd" in
        gui)
            _bitcoin_gui_complete
            ;;
        node)
            _bitcoin_node_complete
            ;;
        rpc)
            _bitcoin_rpc_complete
            ;;
        wallet)
            _bitcoin_wallet_complete
            ;;
        tx)
            _bitcoin_tx_complete
            ;;
        help)
            # For help command, suggest available commands
            local subcommands="gui node rpc wallet tx bench chainstate test test-gui"
            COMPREPLY=($(compgen -W "$subcommands" -- "$cur"))
            ;;
        bench)
            _bitcoin_bench_complete
            ;;
        chainstate)
            _bitcoin_chainstate_complete
            ;;
        test|test-gui)
            _bitcoin_test_complete
            ;;
        *)
            # For unknown subcommands, try to get help dynamically
            if command -v bitcoin >/dev/null 2>&1; then
                local options
                options=$(bitcoin help "$main_cmd" 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
                COMPREPLY=($(compgen -W "$options" -- "$cur"))
            fi
            ;;
    esac
}

# GUI subcommand completion (similar to bitcoin-qt)
_bitcoin_gui_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -conf|-settings)
            _filedir
            return
            ;;
        -datadir|-blocksdir|-walletdir)
            _filedir -d
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help gui 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -version -datadir -conf -regtest -testnet -signet"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
    esac
}

# Node subcommand completion (similar to bitcoind)
_bitcoin_node_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -conf|-includeconf|-pid|-settings)
            _filedir
            return
            ;;
        -datadir|-blocksdir|-walletdir)
            _filedir -d
            return
            ;;
        -rpcbind|-rpcallowip|-externalip|-seednode|-addnode|-connect)
            return
            ;;
        -rpcport|-port|-maxconnections|-rpcthreads|-rpcworkqueue|-keypool)
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help node 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -version -datadir -conf -pid -daemon -printtoconsole -rpcuser -rpcpassword -rpcport -rpcbind -rpcallowip"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
    esac
}

# RPC subcommand completion (similar to bitcoin-cli)
_bitcoin_rpc_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -conf|-rpcwallet)
            _filedir
            return
            ;;
        -datadir)
            _filedir -d
            return
            ;;
        -rpcconnect|-rpcuser|-rpcpassword|-rpcport|-rpcclienttimeout)
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help rpc 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -version -conf -datadir -rpcconnect -rpcport -rpcuser -rpcpassword -rpcwallet -stdin -timeout -named"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
        *)
            # RPC method completion - bitcoin rpc uses -named by default
            local rpc_methods
            if command -v bitcoin >/dev/null 2>&1; then
                rpc_methods=$(bitcoin rpc help 2>/dev/null | grep -E "^[a-z]" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$rpc_methods" ]]; then
                rpc_methods="getblockchaininfo getbestblockhash getblock getblockcount getblockhash gettxout getwalletinfo listwallets"
            fi
            COMPREPLY=($(compgen -W "$rpc_methods" -- "$cur"))
            ;;
    esac
}

# TX subcommand completion (similar to bitcoin-tx)
_bitcoin_tx_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -create|-json|-txid)
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help tx 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -create -json -txid"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
        *=*)
            # Handle key=value pairs for transaction creation
            return
            ;;
        *)
            # Transaction commands
            local tx_commands="in= out= sign= load= set="
            COMPREPLY=($(compgen -W "$tx_commands" -- "$cur"))
            ;;
    esac
}

# Bench subcommand completion
_bitcoin_bench_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help bench 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -list -filter -asymptote -plot-plotlyurl"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
    esac
}

# Chainstate subcommand completion
_bitcoin_chainstate_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -datadir)
            _filedir -d
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help chainstate 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -version -datadir"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
    esac
}

# Test subcommand completion
_bitcoin_test_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help test 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -list_content -run_test -log_level"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
    esac
}

# Wallet subcommand completion
_bitcoin_wallet_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "$prev" in
        -wallet|-datadir)
            _filedir
            return
            ;;
    esac

    case "$cur" in
        -*)
            local options
            if command -v bitcoin >/dev/null 2>&1; then
                options=$(bitcoin help wallet 2>/dev/null | grep -E "^\s+-" | awk '{print $1}' | tr '\n' ' ')
            fi
            if [[ -z "$options" ]]; then
                options="-help -version -datadir -wallet"
            fi
            COMPREPLY=($(compgen -W "$options" -- "$cur"))
            ;;
        *)
            # Wallet commands
            local wallet_commands="create info dump"
            COMPREPLY=($(compgen -W "$wallet_commands" -- "$cur"))
            ;;
    esac
}

complete -F _bitcoin_complete bitcoin