# Function to get available subcommands
function __bitcoin_get_commands
    if command -q bitcoin
        bitcoin --help 2>/dev/null | sed -n '/^Commands:/,/^$/p' | grep -E '^\s+[a-z]' | awk '{print $1}' | string trim
    else
        # Fallback commands
        echo "gui"
        echo "node"
        echo "rpc"
        echo "wallet"
        echo "tx"
        echo "help"
        echo "bench"
        echo "chainstate"
        echo "test"
        echo "test-gui"
    end
end

# Function to get RPC methods
function __bitcoin_get_rpc_methods
    if command -q bitcoin
        bitcoin rpc help 2>/dev/null | grep -E '^[a-z]' | awk '{print $1}'
    else
        # Fallback RPC methods
        echo "getblockchaininfo"
        echo "getbestblockhash"
        echo "getblock"
        echo "getblockcount"
        echo "getblockhash"
        echo "gettxout"
        echo "getwalletinfo"
        echo "listwallets"
        echo "getnewaddress"
        echo "sendtoaddress"
        echo "getbalance"
        echo "listtransactions"
    end
end

# Function to check if we're completing for a specific subcommand
function __bitcoin_using_subcommand
    set -l cmd (commandline -opc)
    if test (count $cmd) -ge 2
        if contains -- $cmd[2] (__bitcoin_get_commands)
            return 0
        end
    end
    return 1
end

# Function to check which subcommand we're using
function __bitcoin_get_current_subcommand
    set -l cmd (commandline -opc)
    if test (count $cmd) -ge 2
        echo $cmd[2]
    end
end

# Main completions
complete -c bitcoin -f

# Global options (before subcommand)
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s h -l help -d "Show help information"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s v -l version -d "Show version information"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s m -l multiprocess -d "Run multiprocess binaries"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s M -l monolithic -d "Run monolithic binaries (default)"

# Subcommands
complete -c bitcoin -n "not __bitcoin_using_subcommand" -a "(__bitcoin_get_commands)"

# GUI subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l version -d "Show version information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l datadir -r -F -d "Data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l conf -r -F -d "Configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l regtest -d "Use regression test network"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l testnet -d "Use test network"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q gui" -l signet -d "Use signet network"

# Node subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l version -d "Show version information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l datadir -r -F -d "Data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l conf -r -F -d "Configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l pid -r -F -d "PID file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l daemon -d "Run as daemon"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l printtoconsole -d "Print to console"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l rpcuser -r -d "RPC username"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l rpcpassword -r -d "RPC password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l rpcport -r -d "RPC port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l rpcbind -r -d "RPC bind address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l rpcallowip -r -d "RPC allow IP"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l port -r -d "Listen port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l maxconnections -r -d "Maximum connections"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l blocksdir -r -F -d "Blocks directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q node" -l walletdir -r -F -d "Wallet directory"

# RPC subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l version -d "Show version information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l conf -r -F -d "Configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l datadir -r -F -d "Data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l rpcconnect -r -d "RPC connect host"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l rpcport -r -d "RPC port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l rpcuser -r -d "RPC username"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l rpcpassword -r -d "RPC password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l rpcwallet -r -F -d "RPC wallet"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l stdin -d "Read from stdin"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -l named -d "Use named arguments (default)"

# RPC methods (for positional arguments after rpc subcommand)
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q rpc" -a "(__bitcoin_get_rpc_methods)"

# TX subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -l create -d "Create new transaction"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -l json -d "Output in JSON format"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -l txid -d "Output transaction ID only"

# TX transaction commands
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -a "in=" -d "Add input"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -a "out=" -d "Add output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -a "sign=" -d "Sign transaction"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -a "load=" -d "Load transaction"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q tx" -a "set=" -d "Set transaction field"

# Help subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q help" -a "gui node rpc wallet tx bench chainstate test test-gui" -d "Get help for command"

# Bench subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q bench" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q bench" -l list -d "List available benchmarks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q bench" -l filter -r -d "Filter benchmarks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q bench" -l asymptote -r -d "Asymptote"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q bench" -l plot-plotlyurl -r -d "Plot URL"

# Chainstate subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q chainstate" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q chainstate" -l version -d "Show version information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q chainstate" -l datadir -r -F -d "Data directory"

# Test subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q test" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q test" -l list_content -d "List test content"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q test" -l run_test -r -d "Run specific test"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q test" -l log_level -r -d "Log level"

# Wallet subcommand completions
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -l help -d "Show help information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -l version -d "Show version information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -l datadir -r -F -d "Data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -l wallet -r -F -d "Wallet file"

# Wallet commands
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -a "create" -d "Create new wallet"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -a "info" -d "Show wallet information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -q wallet" -a "dump" -d "Dump wallet"