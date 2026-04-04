# get available subcommands with descriptions
function __bitcoin_get_commands
    if not set -q __bitcoin_commands_cache
        if command -q bitcoin
            set -g __bitcoin_commands_cache (bitcoin --help 2>/dev/null | grep -E '^\s+[a-z][-a-z]*\s' | string trim | string replace -r '\s*\[ARGS\]\s*' '\t' | string replace -r '\s{2,}' '\t' | head -20)
        end
        if not set -q __bitcoin_commands_cache; or test (count $__bitcoin_commands_cache) -eq 0
            set -g __bitcoin_commands_cache \
                "gui\tStart GUI" \
                "node\tStart node" \
                "rpc\tCall RPC method" \
                "wallet\tCall wallet command" \
                "tx\tManipulate transactions" \
                "help\tShow help message" \
                "bench\tRun benchmarks" \
                "chainstate\tRun chainstate util" \
                "test\tRun unit tests" \
                "test-gui\tRun GUI unit tests"
        end
    end
    for entry in $__bitcoin_commands_cache
        echo $entry
    end
end

# get just the subcommand names
function __bitcoin_get_command_names
    __bitcoin_get_commands | string replace -r '\t.*' ''
end

# get RPC methods
function __bitcoin_get_rpc_methods
    if not set -q __bitcoin_rpc_methods_cache
        if command -q bitcoin
            set -g __bitcoin_rpc_methods_cache (bitcoin rpc help 2>/dev/null | grep -E '^[a-z]' | awk '{print $1}')
        end
        if not set -q __bitcoin_rpc_methods_cache; or test (count $__bitcoin_rpc_methods_cache) -eq 0
            # full fallback list when bitcoin isnt installed or node is not running.
            set -g __bitcoin_rpc_methods_cache \
                dumptxoutset getbestblockhash getblock getblockchaininfo \
                getblockcount getblockfilter getblockfrompeer getblockhash \
                getblockheader getblockstats getchainstates getchaintips \
                getchaintxstats getdeploymentinfo getdescriptoractivity \
                getdifficulty getmempoolancestors getmempooldescendants \
                getmempoolentry getmempoolinfo getrawmempool gettxout \
                gettxoutproof gettxoutsetinfo gettxspendingprevout \
                importmempool loadtxoutset preciousblock pruneblockchain \
                savemempool scanblocks scantxoutset verifychain \
                verifytxoutproof waitforblock waitforblockheight waitfornewblock \
                getmemoryinfo getrpcinfo help logging stop uptime \
                getblocktemplate getmininginfo getnetworkhashps \
                getprioritisedtransactions prioritisetransaction submitblock \
                submitheader \
                addnode clearbanned disconnectnode getaddednodeinfo \
                getaddrmaninfo getconnectioncount getnettotals getnetworkinfo \
                getnodeaddresses getpeerinfo listbanned ping setban \
                setnetworkactive \
                analyzepsbt combinepsbt combinerawtransaction converttopsbt \
                createpsbt createrawtransaction decodepsbt decoderawtransaction \
                decodescript descriptorprocesspsbt finalizepsbt \
                fundrawtransaction getrawtransaction joinpsbts \
                sendrawtransaction signrawtransactionwithkey submitpackage \
                testmempoolaccept utxoupdatepsbt \
                enumeratesigners \
                createmultisig deriveaddresses estimatesmartfee \
                getdescriptorinfo getindexinfo signmessagewithprivkey \
                validateaddress verifymessage \
                abandontransaction abortrescan backupwallet bumpfee \
                createwallet createwalletdescriptor encryptwallet \
                getaddressesbylabel getaddressinfo getbalance getbalances \
                gethdkeys getnewaddress getrawchangeaddress \
                getreceivedbyaddress getreceivedbylabel gettransaction \
                getwalletinfo importdescriptors importprunedfunds \
                keypoolrefill listaddressgroupings listdescriptors listlabels \
                listlockunspent listreceivedbyaddress listreceivedbylabel \
                listsinceblock listtransactions listunspent listwalletdir \
                listwallets loadwallet lockunspent migratewallet psbtbumpfee \
                removeprunedfunds rescanblockchain restorewallet send sendall \
                sendmany sendtoaddress setlabel settxfee setwalletflag \
                signmessage signrawtransactionwithwallet simulaterawtransaction \
                unloadwallet walletcreatefundedpsbt walletdisplayaddress \
                walletlock walletpassphrase walletpassphrasechange \
                walletprocesspsbt
        end
    end
    for method in $__bitcoin_rpc_methods_cache
        echo $method
    end
end

# check if we're completing for a specific subcommand
function __bitcoin_using_subcommand
    set -l cmd (commandline -opc)
    set -l known (__bitcoin_get_command_names)
    for arg in $cmd[2..]
        switch $arg
            case '-*'
                continue
            case '*'
                if contains -- $arg $known
                    return 0
                end
        end
    end
    return 1
end

# check which subcommand we're using
function __bitcoin_get_current_subcommand
    set -l cmd (commandline -opc)
    set -l known (__bitcoin_get_command_names)
    for arg in $cmd[2..]
        switch $arg
            case '-*'
                continue
            case '*'
                if contains -- $arg $known
                    echo $arg
                    return
                end
        end
    end
end

# Main completions
complete -c bitcoin -f

# Global options
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s h -l help -d "Show help information"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s v -l version -d "Show version information"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s m -l multiprocess -d "Run multiprocess binaries"
complete -c bitcoin -n "not __bitcoin_using_subcommand" -s M -l monolithic -d "Run monolithic binaries (default)"

# Subcommands
complete -c bitcoin -n "not __bitcoin_using_subcommand" -a "(__bitcoin_get_commands)"

# === GUI subcommand (bitcoin-qt, same options as node) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o datadir -r -F -d "Specify data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o conf -r -F -d "Specify configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o blocksdir -r -F -d "Specify blocks directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o settings -r -F -d "Specify settings file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o walletdir -r -F -d "Specify wallet directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o daemon -d "Run in background as daemon"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o daemonwait -d "Wait for initialization then background"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o printtoconsole -d "Send trace/debug info to console"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o server -d "Accept command line and JSON-RPC commands"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o chain -r -d "Use specified chain (main, test, testnet4, signet, regtest)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o signet -d "Use signet chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o testnet -d "Use testnet3 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o testnet4 -d "Use testnet4 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o regtest -d "Use regression test network"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o txindex -d "Maintain full transaction index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o dbcache -r -d "Maximum database cache size in MiB"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o prune -r -d "Reduce storage by pruning old blocks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o reindex -d "Rebuild chain state and block index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o rpcuser -r -d "RPC username"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o rpcpassword -r -d "RPC password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o rpcport -r -d "RPC port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o rpcbind -r -d "RPC bind address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o rpcallowip -r -d "Allow JSON-RPC from specified source"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o port -r -d "Listen for connections on port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o maxconnections -r -d "Maximum number of connections"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx gui" -o disablewallet -d "Disable wallet"

# === Node subcommand (bitcoind) ===
# General options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o datadir -r -F -d "Specify data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o conf -r -F -d "Specify configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o includeconf -r -F -d "Specify additional configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o settings -r -F -d "Specify settings file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o pid -r -F -d "Specify pid file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o daemon -d "Run in background as daemon"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o daemonwait -d "Wait for initialization then background"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o printtoconsole -d "Send trace/debug info to console"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o allowignoredconf -d "Treat unused conf file as warning"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blocksdir -r -F -d "Specify blocks directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blocksonly -d "Reject transactions from network peers"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blocksxor -d "Apply XOR-key to blocksdir dat files"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o dbcache -r -d "Maximum database cache size in MiB"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o debuglogfile -r -F -d "Debug log file location"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o loadblock -r -F -d "Import blocks from external file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxmempool -r -d "Max transaction memory pool in MB"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o mempoolexpiry -r -d "Max hours to keep transactions in mempool"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o par -r -d "Script verification threads (0=auto)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o persistmempool -d "Save/load mempool on shutdown/restart"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o persistmempoolv1 -d "Write mempool.dat in legacy format"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o prune -r -d "Enable pruning of old blocks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o reindex -d "Rebuild chain state and block index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o reindex-chainstate -d "Rebuild chain state from blk*.dat"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o txindex -d "Maintain full transaction index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blockfilterindex -r -d "Maintain compact filter index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o coinstatsindex -d "Maintain coinstats index"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o assumevalid -r -d "Skip script verification for this block"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o alertnotify -r -d "Execute command on alert"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blocknotify -r -d "Execute command on best block change"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o shutdownnotify -r -d "Execute command before shutdown"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o startupnotify -r -d "Execute command on startup"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blockreconstructionextratxn -r -d "Extra txns for compact block reconstruction"
# Connection options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o addnode -r -d "Add node to connect to"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o asmap -r -F -d "ASN mapping file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o bantime -r -d "Default ban duration in seconds"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o bind -r -d "Bind to address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o cjdnsreachable -d "Host is configured for CJDNS"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o connect -r -d "Connect only to specified node"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o discover -d "Discover own IP addresses"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o dns -d "Allow DNS lookups"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o dnsseed -d "Query DNS for peer addresses"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o externalip -r -d "Specify public address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o fixedseeds -d "Allow fixed seeds"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o forcednsseed -d "Always query DNS for peers"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o i2pacceptincoming -d "Accept inbound I2P connections"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o i2psam -r -d "I2P SAM proxy address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o listen -d "Accept connections from outside"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o listenonion -d "Create Tor onion service"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxconnections -r -d "Max automatic peer connections"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxreceivebuffer -r -d "Max per-connection receive buffer"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxsendbuffer -r -d "Max per-connection send buffer"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxuploadtarget -r -d "Max outbound traffic per 24h"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o natpmp -d "Use PCP or NAT-PMP for port mapping"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o networkactive -d "Enable all P2P network activity"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o onion -r -d "SOCKS5 proxy for Tor"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o onlynet -r -d "Automatic outbound connections only to network"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o peerblockfilters -d "Serve compact block filters (BIP 157)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o peerbloomfilters -d "Support bloom filter blocks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o port -r -d "Listen port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o proxy -r -d "SOCKS5 proxy"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o proxyrandomize -d "Randomize proxy credentials"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o seednode -r -d "Connect to node for peer addresses"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o timeout -r -d "Connection timeout in ms"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o torcontrol -r -d "Tor control host:port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o torpassword -r -d "Tor control port password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o v2transport -d "Support v2 transport"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o whitebind -r -d "Bind address with permissions"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o whitelist -r -d "Add permission flags to peers"
# Wallet options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o addresstype -r -d "Address type (legacy, p2sh-segwit, bech32, bech32m)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o avoidpartialspends -d "Group outputs by address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o changetype -r -d "Change address type"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o consolidatefeerate -r -d "Max feerate for UTXO consolidation"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o disablewallet -d "Disable wallet"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o discardfee -r -d "Fee rate to discard change"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o fallbackfee -r -d "Fallback fee rate"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o keypool -r -d "Key pool size"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxapsfee -r -d "Max additional fee for partial spend avoidance"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o mintxfee -r -d "Min fee rate for tx creation"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o paytxfee -r -d "Fee rate to add to transactions"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o signer -r -d "External signing tool"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o spendzeroconfchange -d "Spend unconfirmed change"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o txconfirmtarget -r -d "Target blocks for confirmation"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o wallet -r -F -d "Wallet path to load"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o walletbroadcast -d "Broadcast wallet transactions"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o walletdir -r -F -d "Wallet directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o walletnotify -r -d "Execute command on wallet tx change"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o walletrbf -d "Send transactions with full-RBF"
# Debugging options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o debug -r -d "Output debug logging for category"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o debugexclude -r -d "Exclude debug logging category"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o help-debug -d "Print help with debugging options"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o logips -d "Include IP addresses in debug output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o loglevelalways -d "Always prepend category and level"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o logsourcelocations -d "Prepend source location to debug output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o logthreadnames -d "Prepend thread name to debug output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o logtimestamps -d "Prepend timestamp to debug output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o maxtxfee -r -d "Max total fees per wallet transaction"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o shrinkdebugfile -d "Shrink debug.log on startup"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o uacomment -r -d "Append comment to user agent string"
# Chain selection
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o chain -r -d "Use specified chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o signet -d "Use signet chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o signetchallenge -r -d "Signet challenge script"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o signetseednode -r -d "Signet seed node"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o testnet -d "Use testnet3 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o testnet4 -d "Use testnet4 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o regtest -d "Use regression test network"
# Relay options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o bytespersigop -r -d "Equivalent bytes per sigop"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o datacarrier -d "Relay data carrier transactions"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o datacarriersize -r -d "Max data carrier script size"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o minrelaytxfee -r -d "Min relay fee rate"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o permitbaremultisig -d "Relay non-P2SH multisig outputs"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o whitelistforcerelay -d "Force relay for whitelisted peers"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o whitelistrelay -d "Relay for whitelisted peers"
# Block creation options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blockmintxfee -r -d "Min fee rate for block inclusion"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o blockreservedweight -r -d "Reserved block weight"
# RPC server options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rest -d "Accept public REST requests"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcallowip -r -d "Allow JSON-RPC from source"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcauth -r -d "RPC username:HMAC-SHA-256 password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcbind -r -d "RPC bind address"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpccookiefile -r -F -d "Auth cookie location"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpccookieperms -r -d "Auth cookie file permissions"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcpassword -r -d "RPC password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcport -r -d "RPC port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcthreads -r -d "RPC threads"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcuser -r -d "RPC username"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcwhitelist -r -d "RPC whitelist for user"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o rpcwhitelistdefault -d "Default RPC whitelist behavior"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx node" -o server -d "Accept command line and JSON-RPC commands"

# === RPC subcommand (bitcoin-cli) ===
# Options
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o color -r -d "Color setting (always, auto, never)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o conf -r -F -d "Specify configuration file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o datadir -r -F -d "Specify data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o named -d "Pass named instead of positional arguments"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcclienttimeout -r -d "HTTP request timeout in seconds"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcconnect -r -d "Send commands to node on host"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpccookiefile -r -F -d "Auth cookie location"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcpassword -r -d "RPC password"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcport -r -d "RPC port"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcuser -r -d "RPC username"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcwait -d "Wait for RPC server to start"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcwaittimeout -r -d "Timeout to wait for RPC server"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o rpcwallet -r -d "RPC wallet name"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o stdin -d "Read extra arguments from stdin"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o stdinrpcpass -d "Read RPC password from stdin"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o stdinwalletpassphrase -d "Read wallet passphrase from stdin"
# Chain selection
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o chain -r -d "Use specified chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o signet -d "Use signet chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o signetchallenge -r -d "Signet challenge script"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o signetseednode -r -d "Signet seed node"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o testnet -d "Use testnet3 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o testnet4 -d "Use testnet4 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o regtest -d "Use regression test network"
# CLI commands
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o addrinfo -d "Get address count per network"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o generate -d "Generate blocks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o getinfo -d "Get general server information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -o netinfo -d "Get network peer connection info"
# RPC methods
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx rpc" -a "(__bitcoin_get_rpc_methods)"

# === Wallet subcommand (bitcoin-wallet) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o datadir -r -F -d "Specify data directory"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o wallet -r -d "Specify wallet name"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o dumpfile -r -F -d "File for dump/createfromdump"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o debug -r -d "Output debugging information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o printtoconsole -d "Send trace/debug info to console"
# Chain selection
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o chain -r -d "Use specified chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o signet -d "Use signet chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o signetchallenge -r -d "Signet challenge script"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o signetseednode -r -d "Signet seed node"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o testnet -d "Use testnet3 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o testnet4 -d "Use testnet4 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -o regtest -d "Use regression test network"
# Wallet commands
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -a "create" -d "Create new descriptor wallet"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -a "createfromdump" -d "Create wallet from dumped records"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -a "dump" -d "Print all wallet key-value records"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx wallet" -a "info" -d "Get wallet info"

# === TX subcommand (bitcoin-tx) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o create -d "Create new empty transaction"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o json -d "Select JSON output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o txid -d "Output only the transaction id"
# Chain selection
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o chain -r -d "Use specified chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o signet -d "Use signet chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o signetchallenge -r -d "Signet challenge script"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o signetseednode -r -d "Signet seed node"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o testnet -d "Use testnet3 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o testnet4 -d "Use testnet4 chain"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -o regtest -d "Use regression test network"
# TX commands
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "delin=" -d "Delete input N from TX"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "delout=" -d "Delete output N from TX"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "in=" -d "Add input (TXID:VOUT[:SEQ])"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "locktime=" -d "Set TX lock time"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "nversion=" -d "Set TX version"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "outaddr=" -d "Add address-based output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "outdata=" -d "Add data-based output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "outmultisig=" -d "Add multisig output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "outpubkey=" -d "Add pay-to-pubkey output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "outscript=" -d "Add raw script output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "replaceable" -d "Set RBF opt-in sequence number"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "sign=" -d "Sign transaction (SIGHASH-FLAGS)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "load=" -d "Load JSON file into register"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx tx" -a "set=" -d "Set register to JSON string"

# === Help subcommand ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx help" -a "(__bitcoin_get_command_names)" -d "Get help for command"

# === Bench subcommand (bench_bitcoin) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx bench" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx bench" -o list -d "List available benchmarks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx bench" -o filter -r -d "Filter benchmarks by regex"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx bench" -o asymptote -r -d "Asymptote values"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx bench" -o plot-plotlyurl -r -d "Plot Plotly URL"

# === Chainstate subcommand (bitcoin-chainstate) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx chainstate" -o help -d "Print help message and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx chainstate" -o version -d "Print version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx chainstate" -o datadir -r -F -d "Specify data directory"

# === Test subcommand (test_bitcoin, Boost.Test) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l help -d "Help for framework parameters"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l version -d "Print Boost.Test version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l run_test -r -d "Filter which tests to execute"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l list_content -d "List test suites and test cases"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l list_labels -d "List all available labels"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l log_level -r -d "Logging level (all|success|test_suite|message|warning|error|nothing)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l log_format -r -d "Log format (HRF|CLF|XML|JUNIT)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l log_sink -r -d "Log sink (stderr|stdout|file)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l logger -r -d "Log format,level,sink specification"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l output_format -r -d "Output format (HRF|CLF|XML)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l report_level -r -d "Report level (confirm|short|detailed|no)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l report_format -r -d "Report format (HRF|CLF|XML)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l report_sink -r -d "Report sink (stderr|stdout|file)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l report_memory_leaks_to -r -F -d "File to report memory leaks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l random -r -d "Random order execution (optional seed)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l color_output -d "Enable color output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l catch_system_errors -d "Catch system errors/signals"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l detect_fp_exceptions -d "Enable floating point exception traps"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l detect_memory_leaks -d "Turn on memory leak detection"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l build_info -d "Display library build information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l auto_start_dbg -d "Attach debugger on system failure"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l break_exec_path -r -d "Break at specific execution path"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l result_code -d "Enable/disable result code generation"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l save_pattern -d "Save or match test pattern file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l show_progress -d "Turn on progress display"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l use_alt_stack -d "Use alternative stack for signal handling"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test" -l wait_for_debugger -d "Wait for debugger before test run"

# === Test-gui subcommand (test_bitcoin-qt, same Boost.Test options) ===
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l help -d "Help for framework parameters"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l version -d "Print Boost.Test version and exit"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l run_test -r -d "Filter which tests to execute"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l list_content -d "List test suites and test cases"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l list_labels -d "List all available labels"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l log_level -r -d "Logging level (all|success|test_suite|message|warning|error|nothing)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l log_format -r -d "Log format (HRF|CLF|XML|JUNIT)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l log_sink -r -d "Log sink (stderr|stdout|file)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l logger -r -d "Log format,level,sink specification"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l output_format -r -d "Output format (HRF|CLF|XML)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l report_level -r -d "Report level (confirm|short|detailed|no)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l report_format -r -d "Report format (HRF|CLF|XML)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l report_sink -r -d "Report sink (stderr|stdout|file)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l report_memory_leaks_to -r -F -d "File to report memory leaks"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l random -r -d "Random order execution (optional seed)"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l color_output -d "Enable color output"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l catch_system_errors -d "Catch system errors/signals"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l detect_fp_exceptions -d "Enable floating point exception traps"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l detect_memory_leaks -d "Turn on memory leak detection"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l build_info -d "Display library build information"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l auto_start_dbg -d "Attach debugger on system failure"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l break_exec_path -r -d "Break at specific execution path"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l result_code -d "Enable/disable result code generation"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l save_pattern -d "Save or match test pattern file"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l show_progress -d "Turn on progress display"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l use_alt_stack -d "Use alternative stack for signal handling"
complete -c bitcoin -n "__bitcoin_get_current_subcommand | grep -qx test-gui" -l wait_for_debugger -d "Wait for debugger before test run"
