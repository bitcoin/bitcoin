#ifndef BITCOIN_INIT_SETTINGS_H
#define BITCOIN_INIT_SETTINGS_H

#include <addrman.h>
#include <banman.h>
#include <blockfilter.h>
#include <chainparamsbase.h>
#include <common/args.h>
#include <common/setting.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <init.h>
#include <kernel/blockmanager_opts.h>
#include <kernel/mempool_options.h>
#include <mapport.h>
#include <net.h>
#include <net_processing.h>
#include <node/chainstatemanager_args.h>
#include <node/kernel_notifications.h>
#include <node/mempool_persist_args.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/settings.h>
#include <rpc/util.h>
#include <script/sigcache.h>
#include <torcontrol.h>
#include <txdb.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <zmq/zmqabstractnotifier.h>

#include <string>
#include <vector>

using CheckaddrmanSetting = common::Setting<
    "-checkaddrman=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Run addrman consistency checks every <n> operations. Use 0 to disable. (default: %u)">
    ::HelpArgs<DEFAULT_ADDRMAN_CONSISTENCY_CHECKS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using VersionSetting = common::Setting<
    "-version", bool, common::SettingOptions{.legacy = true},
    "Print version and exit">;

using ConfSetting = common::Setting<
    "-conf=<file>", std::string, common::SettingOptions{.legacy = true},
    "Specify path to read-only configuration file. Relative paths will be prefixed by datadir location (only useable from command line, not configuration file) (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, BITCOIN_CONF_FILENAME); }>;

using ConfSettingPath = common::Setting<
    "-conf=<file>", fs::path, common::SettingOptions{.legacy = true}>
    ::DefaultFn<[] { return BITCOIN_CONF_FILENAME; }>;

using DatadirSetting = common::Setting<
    "-datadir=<dir>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Specify data directory">;

using DatadirSettingPath = common::Setting<
    "-datadir=<dir>", fs::path, common::SettingOptions{.legacy = true, .disallow_negation = true}>;

using RpccookiefileSetting = common::Setting<
    "-rpccookiefile=<loc>", fs::path, common::SettingOptions{.legacy = true},
    "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)">
    ::Category<OptionsCategory::RPC>;

using RpcpasswordSetting = common::Setting<
    "-rpcpassword=<pw>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Password for JSON-RPC connections">
    ::Category<OptionsCategory::RPC>;

using RpcportSetting = common::Setting<
    "-rpcport=<port>", std::optional<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Listen for JSON-RPC connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u)">
    ::HelpFn<[](const auto& fmt, const auto& defaultBaseParams, const auto& testnetBaseParams, const auto& testnet4BaseParams, const auto& signetBaseParams, const auto& regtestBaseParams) { return strprintf(fmt, defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), testnet4BaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()); }>
    ::Category<OptionsCategory::RPC>;

using RpcportSettingInt = common::Setting<
    "-rpcport=<port>", int64_t, common::SettingOptions{.legacy = true, .network_only = true}>
    ::DefaultFn<[] { return BaseParams().RPCPort(); }>
    ::HelpFn<[](const auto& fmt, const auto& defaultBaseParams, const auto& testnetBaseParams, const auto& testnet4BaseParams, const auto& signetBaseParams, const auto& regtestBaseParams) { return strprintf(fmt, defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), testnet4BaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()); }>;

using RpcuserSetting = common::Setting<
    "-rpcuser=<user>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Username for JSON-RPC connections">
    ::Category<OptionsCategory::RPC>;

using DaemonSetting = common::Setting<
    "-daemon", bool, common::SettingOptions{.legacy = true},
    "Run in the background as a daemon and accept commands (default: %d)">
    ::HelpArgs<DEFAULT_DAEMON>;

using DaemonwaitSetting = common::Setting<
    "-daemonwait", bool, common::SettingOptions{.legacy = true},
    "Wait for initialization to be finished before exiting. This implies -daemon (default: %d)">
    ::Default<DEFAULT_DAEMONWAIT>;

using FastpruneSetting = common::Setting<
    "-fastprune", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Use smaller block files and lower minimum prune height for testing purposes">
    ::Category<OptionsCategory::DEBUG_TEST>;

using BlocksdirSetting = common::Setting<
    "-blocksdir=<dir>", std::string, common::SettingOptions{.legacy = true},
    "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)">;

using BlocksdirSettingPath = common::Setting<
    "-blocksdir=<dir>", fs::path, common::SettingOptions{.legacy = true}>;

using SettingsSetting = common::Setting<
    "-settings=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify path to dynamic settings data file. Can be disabled with -nosettings. File is written at runtime and not meant to be edited by users (use %s instead for custom settings). Relative paths will be prefixed by datadir location. (default: %s)">
    ::DefaultFn<[] { return BITCOIN_SETTINGS_FILENAME; }>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, BITCOIN_CONF_FILENAME, BITCOIN_SETTINGS_FILENAME); }>;

using HelpDebugSetting = common::Setting<
    "-help-debug", bool, common::SettingOptions{.legacy = true},
    "Print help message with debugging options and exit">
    ::Category<OptionsCategory::DEBUG_TEST>;

using TestSetting = common::Setting<
    "-test=<option>", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Pass a test-only option. Options include : %s.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, util::Join(TEST_OPTIONS_DOC, ", ")); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using AllowignoredconfSetting = common::Setting<
    "-allowignoredconf", bool, common::SettingOptions{.legacy = true},
    "For backwards compatibility, treat an unused %s file in the datadir as a warning, not an error.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, BITCOIN_CONF_FILENAME); }>;

using RpccookiepermsSetting = common::Setting<
    "-rpccookieperms=<readable-by>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Set permissions on the RPC auth cookie file so that it is readable by [owner|group|all] (default: owner [via umask 0077])">
    ::Category<OptionsCategory::RPC>;

using RpcauthSetting = common::Setting<
    "-rpcauth=<userpw>", std::vector<std::string>, common::SettingOptions{.legacy = true, .sensitive = true},
    "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcauth. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times">
    ::Category<OptionsCategory::RPC>;

using RpcwhitelistdefaultSetting = common::Setting<
    "-rpcwhitelistdefault", bool, common::SettingOptions{.legacy = true},
    "Sets default behavior for rpc whitelisting. Unless rpcwhitelistdefault is set to 0, if any -rpcwhitelist is set, the rpc server acts as if all rpc users are subject to empty-unless-otherwise-specified whitelists. If rpcwhitelistdefault is set to 1 and no -rpcwhitelist is set, rpc server acts as if all rpc users are subject to empty whitelists.">
    ::Category<OptionsCategory::RPC>;

using RpcwhitelistSetting = common::Setting<
    "-rpcwhitelist=<whitelist>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Set a whitelist to filter incoming RPC calls for a specific user. The field <whitelist> comes in the format: <USERNAME>:<rpc 1>,<rpc 2>,...,<rpc n>. If multiple whitelists are set for a given user, they are set-intersected. See -rpcwhitelistdefault documentation for information on default whitelist behavior.">
    ::Category<OptionsCategory::RPC>;

using RpcallowipSetting = common::Setting<
    "-rpcallowip=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Allow JSON-RPC connections from specified source. Valid values for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0), a network/CIDR (e.g. 1.2.3.4/24), all ipv4 (0.0.0.0/0), or all ipv6 (::/0). This option can be specified multiple times">
    ::Category<OptionsCategory::RPC>;

using RpcbindSetting = common::Setting<
    "-rpcbind=<addr>[:port]", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost)">
    ::Category<OptionsCategory::RPC>;

using RpcservertimeoutSetting = common::Setting<
    "-rpcservertimeout=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Timeout during HTTP requests (default: %d)">
    ::Default<DEFAULT_HTTP_SERVER_TIMEOUT>
    ::Category<OptionsCategory::RPC>;

using RpcworkqueueSetting = common::Setting<
    "-rpcworkqueue=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Set the depth of the work queue to service RPC calls (default: %d)">
    ::Default<DEFAULT_HTTP_WORKQUEUE>
    ::Category<OptionsCategory::RPC>;

using RpcthreadsSetting = common::Setting<
    "-rpcthreads=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set the number of threads to service RPC calls (default: %d)">
    ::Default<DEFAULT_HTTP_THREADS>
    ::Category<OptionsCategory::RPC>;

using AlertnotifySetting = common::Setting<
    "-alertnotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command when an alert is raised (%s in cmd is replaced by message)">;

using AssumevalidSetting = common::Setting<
    "-assumevalid=<hex>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet3: %s, testnet4: %s, signet: %s)">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams) { return strprintf(fmt, defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnet4ChainParams->GetConsensus().defaultAssumeValid.GetHex(), signetChainParams->GetConsensus().defaultAssumeValid.GetHex()); }>;

using BlocksxorSetting = common::Setting<
    "-blocksxor", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Whether an XOR-key applies to blocksdir *.dat files. "
                             "The created XOR-key will be zeros for an existing blocksdir or when `-blocksxor=0` is "
                             "set, and random for a freshly initialized blocksdir. "
                             "(default: %u)">
    ::HelpArgs<kernel::DEFAULT_XOR_BLOCKSDIR>;

using BlocknotifySetting = common::Setting<
    "-blocknotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command when the best block changes (%s in cmd is replaced by block hash)">;

using BlockreconstructionextratxnSetting = common::Setting<
    "-blockreconstructionextratxn=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Extra transactions to keep in memory for compact block reconstructions (default: %u)">
    ::HelpArgs<DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN>;

using BlocksonlySetting = common::Setting<
    "-blocksonly", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Whether to reject transactions from network peers. Disables automatic broadcast and rebroadcast of transactions, unless the source peer has the 'forcerelay' permission. RPC transactions are not affected. (default: %u)">
    ::HelpArgs<DEFAULT_BLOCKSONLY>;

using CoinstatsindexSetting = common::Setting<
    "-coinstatsindex", bool, common::SettingOptions{.legacy = true},
    "Maintain coinstats index used by the gettxoutsetinfo RPC (default: %u)">
    ::Default<DEFAULT_COINSTATSINDEX>;

using DbbatchsizeSetting = common::Setting<
    "-dbbatchsize", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Maximum database write batch size in bytes (default: %u)">
    ::HelpArgs<nDefaultDbBatchSize>;

using DbcacheSetting = common::Setting<
    "-dbcache=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maximum database cache size <n> MiB (minimum %d, default: %d). Make sure you have enough RAM. In addition, unused memory allocated to the mempool is shared with this cache (see -maxmempool).">
    ::Default<nDefaultDbCache>
    ::HelpArgs<nMinDbCache, nDefaultDbCache>;

using IncludeconfSetting = common::Setting<
    "-includeconf=<file>", common::Unset, common::SettingOptions{.legacy = true},
    "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)">;

using LoadblockSetting = common::Setting<
    "-loadblock=<file>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Imports blocks from external file on startup">;

using MaxmempoolSetting = common::Setting<
    "-maxmempool=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Keep the transaction memory pool below <n> megabytes (default: %u)">
    ::HelpArgs<DEFAULT_MAX_MEMPOOL_SIZE_MB>;

using MaxorphantxSetting = common::Setting<
    "-maxorphantx=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Keep at most <n> unconnectable transactions in memory (default: %u)">
    ::HelpArgs<DEFAULT_MAX_ORPHAN_TRANSACTIONS>;

using MempoolexpirySetting = common::Setting<
    "-mempoolexpiry=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Do not keep transactions in the mempool longer than <n> hours (default: %u)">
    ::HelpArgs<DEFAULT_MEMPOOL_EXPIRY_HOURS>;

using MinimumchainworkSetting = common::Setting<
    "-minimumchainwork=<hex>", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Minimum work assumed to exist on a valid chain in hex (default: %s, testnet3: %s, testnet4: %s, signet: %s)">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams) { return strprintf(fmt, defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnet4ChainParams->GetConsensus().nMinimumChainWork.GetHex(), signetChainParams->GetConsensus().nMinimumChainWork.GetHex()); }>;

using ParSetting = common::Setting<
    "-par=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set the number of script verification threads (0 = auto, up to %d, <0 = leave that many cores free, default: %d)">
    ::Default<DEFAULT_SCRIPTCHECK_THREADS>
    ::HelpArgs<MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS>;

using PersistmempoolSetting = common::Setting<
    "-persistmempool", bool, common::SettingOptions{.legacy = true},
    "Whether to save the mempool on shutdown and load on restart (default: %u)">
    ::Default<node::DEFAULT_PERSIST_MEMPOOL>;

using Persistmempoolv1Setting = common::Setting<
    "-persistmempoolv1", bool, common::SettingOptions{.legacy = true},
    "Whether a mempool.dat file created by -persistmempool or the savemempool RPC will be written in the legacy format "
                             "(version 1) or the current format (version 2). This temporary option will be removed in the future. (default: %u)">
    ::HelpArgs<DEFAULT_PERSIST_V1_DAT>;

using PidSetting = common::Setting<
    "-pid=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)">
    ::DefaultFn<[] { return BITCOIN_PID_FILENAME; }>;

using PruneSetting = common::Setting<
    "-prune=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >=%u = automatically prune block files to stay under the specified target size in MiB)">
    ::HelpArgs<MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024>;

using ReindexSetting = common::Setting<
    "-reindex", bool, common::SettingOptions{.legacy = true},
    "If enabled, wipe chain state and block index, and rebuild them from blk*.dat files on disk. Also wipe and rebuild other optional indexes that are active. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.">;

using ReindexChainstateSetting = common::Setting<
    "-reindex-chainstate", bool, common::SettingOptions{.legacy = true},
    "If enabled, wipe chain state, and rebuild it from blk*.dat files on disk. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.">;

using StartupnotifySetting = common::Setting<
    "-startupnotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command on startup.">;

using ShutdownnotifySetting = common::Setting<
    "-shutdownnotify=<cmd>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Execute command immediately before beginning shutdown. The need for shutdown may be urgent, so be careful not to delay it long (if the command doesn't require interaction with the server, consider having it fork into the background).">;

using TxindexSetting = common::Setting<
    "-txindex", bool, common::SettingOptions{.legacy = true},
    "Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)">
    ::Default<DEFAULT_TXINDEX>;

using BlockfilterindexSetting = common::Setting<
    "-blockfilterindex=<type>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Maintain an index of compact filters by block (default: %s, values: %s).">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()); }>;

using BlockfilterindexSettingStr = common::Setting<
    "-blockfilterindex=<type>", std::string, common::SettingOptions{.legacy = true}>
    ::DefaultFn<[] { return DEFAULT_BLOCKFILTERINDEX; }>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()); }>;

using AddnodeSetting = common::Setting<
    "-addnode=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Add a node to connect to and attempt to keep the connection open (see the addnode RPC help for more info). This option can be specified multiple times to add multiple nodes; connections are limited to %u at a time and are counted separately from the -maxconnections limit.">
    ::HelpArgs<MAX_ADDNODE_CONNECTIONS>
    ::Category<OptionsCategory::CONNECTION>;

using AsmapSetting = common::Setting<
    "-asmap=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify asn mapping used for bucketing of the peers (default: %s). Relative paths will be prefixed by the net-specific datadir location.">
    ::DefaultFn<[] { return DEFAULT_ASMAP_FILENAME; }>
    ::Category<OptionsCategory::CONNECTION>;

using BantimeSetting = common::Setting<
    "-bantime=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Default duration (in seconds) of manually configured bans (default: %u)">
    ::Default<DEFAULT_MISBEHAVING_BANTIME>
    ::Category<OptionsCategory::CONNECTION>;

using BindSetting = common::Setting<
    "-bind=<addr>[:<port>][=onion]", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Bind to given address and always listen on it (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections (default: 127.0.0.1:%u=onion, testnet3: 127.0.0.1:%u=onion, testnet4: 127.0.0.1:%u=onion, signet: 127.0.0.1:%u=onion, regtest: 127.0.0.1:%u=onion)">
    ::HelpFn<[](const auto& fmt, const auto& defaultBaseParams, const auto& testnetBaseParams, const auto& testnet4BaseParams, const auto& signetBaseParams, const auto& regtestBaseParams) { return strprintf(fmt, defaultBaseParams->OnionServiceTargetPort(), testnetBaseParams->OnionServiceTargetPort(), testnet4BaseParams->OnionServiceTargetPort(), signetBaseParams->OnionServiceTargetPort(), regtestBaseParams->OnionServiceTargetPort()); }>
    ::Category<OptionsCategory::CONNECTION>;

using CjdnsreachableSetting = common::Setting<
    "-cjdnsreachable", common::Unset, common::SettingOptions{.legacy = true},
    "If set, then this host is configured for CJDNS (connecting to fc00::/8 addresses would lead us to the CJDNS network, see doc/cjdns.md) (default: 0)">
    ::Category<OptionsCategory::CONNECTION>;

using ConnectSetting = common::Setting<
    "-connect=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.">
    ::Category<OptionsCategory::CONNECTION>;

using DiscoverSetting = common::Setting<
    "-discover", bool, common::SettingOptions{.legacy = true},
    "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)">
    ::Default<true>
    ::HelpArgs<>
    ::Category<OptionsCategory::CONNECTION>;

using DnsSetting = common::Setting<
    "-dns", bool, common::SettingOptions{.legacy = true},
    "Allow DNS lookups for -addnode, -seednode and -connect (default: %u)">
    ::Default<DEFAULT_NAME_LOOKUP>
    ::Category<OptionsCategory::CONNECTION>;

using DnsseedSetting = common::Setting<
    "-dnsseed", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Query for peer addresses via DNS lookup, if low on addresses (default: %u unless -connect used or -maxconnections=0)">
    ::HelpArgs<DEFAULT_DNSSEED>
    ::Category<OptionsCategory::CONNECTION>;

using ExternalipSetting = common::Setting<
    "-externalip=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Specify your own public address">
    ::Category<OptionsCategory::CONNECTION>;

using FixedseedsSetting = common::Setting<
    "-fixedseeds", bool, common::SettingOptions{.legacy = true},
    "Allow fixed seeds if DNS seeds don't provide peers (default: %u)">
    ::Default<DEFAULT_FIXEDSEEDS>
    ::Category<OptionsCategory::CONNECTION>;

using ForcednsseedSetting = common::Setting<
    "-forcednsseed", bool, common::SettingOptions{.legacy = true},
    "Always query for peer addresses via DNS lookup (default: %u)">
    ::Default<DEFAULT_FORCEDNSSEED>
    ::Category<OptionsCategory::CONNECTION>;

using ListenSetting = common::Setting<
    "-listen", bool, common::SettingOptions{.legacy = true},
    "Accept connections from outside (default: %u if no -proxy, -connect or -maxconnections=0)">
    ::HelpArgs<DEFAULT_LISTEN>
    ::Category<OptionsCategory::CONNECTION>;

using ListenonionSetting = common::Setting<
    "-listenonion", bool, common::SettingOptions{.legacy = true},
    "Automatically create Tor onion service (default: %d)">
    ::HelpArgs<DEFAULT_LISTEN_ONION>
    ::Category<OptionsCategory::CONNECTION>;

using MaxconnectionsSetting = common::Setting<
    "-maxconnections=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maintain at most <n> automatic connections to peers (default: %u). This limit does not apply to connections manually added via -addnode or the addnode RPC, which have a separate limit of %u.">
    ::Default<DEFAULT_MAX_PEER_CONNECTIONS>
    ::HelpArgs<DEFAULT_MAX_PEER_CONNECTIONS, MAX_ADDNODE_CONNECTIONS>
    ::Category<OptionsCategory::CONNECTION>;

using MaxreceivebufferSetting = common::Setting<
    "-maxreceivebuffer=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)">
    ::Default<DEFAULT_MAXRECEIVEBUFFER>
    ::Category<OptionsCategory::CONNECTION>;

using MaxsendbufferSetting = common::Setting<
    "-maxsendbuffer=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maximum per-connection memory usage for the send buffer, <n>*1000 bytes (default: %u)">
    ::Default<DEFAULT_MAXSENDBUFFER>
    ::Category<OptionsCategory::CONNECTION>;

using MaxuploadtargetSetting = common::Setting<
    "-maxuploadtarget=<n>", std::string, common::SettingOptions{.legacy = true},
    "Tries to keep outbound traffic under the given target per 24h. Limit does not apply to peers with 'download' permission or blocks created within past week. 0 = no limit (default: %s). Optional suffix units [k|K|m|M|g|G|t|T] (default: M). Lowercase is 1000 base while uppercase is 1024 base">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DEFAULT_MAX_UPLOAD_TARGET); }>
    ::Category<OptionsCategory::CONNECTION>;

using OnionSetting = common::Setting<
    "-onion=<ip:port|path>", std::string, common::SettingOptions{.legacy = true},
    "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy). May be a local file path prefixed with 'unix:'.">
    ::Category<OptionsCategory::CONNECTION>;

using OnionSetting2 = common::Setting<
    "-onion=<ip:port>", std::string, common::SettingOptions{.legacy = true},
    "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy)">
    ::Category<OptionsCategory::CONNECTION>;

using I2psamSetting = common::Setting<
    "-i2psam=<ip:port>", std::string, common::SettingOptions{.legacy = true},
    "I2P SAM proxy to reach I2P peers and accept I2P connections (default: none)">
    ::Category<OptionsCategory::CONNECTION>;

using I2pacceptincomingSetting = common::Setting<
    "-i2pacceptincoming", bool, common::SettingOptions{.legacy = true},
    "Whether to accept inbound I2P connections (default: %i). Ignored if -i2psam is not set. Listening for inbound I2P connections is done through the SAM proxy, not by binding to a local address and port.">
    ::Default<DEFAULT_I2P_ACCEPT_INCOMING>
    ::Category<OptionsCategory::CONNECTION>;

using OnlynetSetting = common::Setting<
    "-onlynet=<net>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Make automatic outbound connections only to network <net> (%s). Inbound and manual connections are not affected by this option. It can be specified multiple times to allow multiple networks.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, util::Join(GetNetworkNames(), ", ")); }>
    ::Category<OptionsCategory::CONNECTION>;

using V2transportSetting = common::Setting<
    "-v2transport", bool, common::SettingOptions{.legacy = true},
    "Support v2 transport (default: %u)">
    ::Default<DEFAULT_V2_TRANSPORT>
    ::Category<OptionsCategory::CONNECTION>;

using PeerbloomfiltersSetting = common::Setting<
    "-peerbloomfilters", bool, common::SettingOptions{.legacy = true},
    "Support filtering of blocks and transaction with bloom filters (default: %u)">
    ::Default<DEFAULT_PEERBLOOMFILTERS>
    ::Category<OptionsCategory::CONNECTION>;

using PeerblockfiltersSetting = common::Setting<
    "-peerblockfilters", bool, common::SettingOptions{.legacy = true},
    "Serve compact block filters to peers per BIP 157 (default: %u)">
    ::Default<DEFAULT_PEERBLOCKFILTERS>
    ::Category<OptionsCategory::CONNECTION>;

using TxreconciliationSetting = common::Setting<
    "-txreconciliation", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Enable transaction reconciliations per BIP 330 (default: %d)">
    ::HelpArgs<DEFAULT_TXRECONCILIATION_ENABLE>
    ::Category<OptionsCategory::CONNECTION>;

using PortSetting = common::Setting<
    "-port=<port>", int64_t, common::SettingOptions{.legacy = true, .network_only = true},
    "Listen for connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u). Not relevant for I2P (see doc/i2p.md).">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams, const auto& regtestChainParams) { return strprintf(fmt, defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), testnet4ChainParams->GetDefaultPort(), signetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()); }>
    ::Category<OptionsCategory::CONNECTION>;

using ProxySetting = common::Setting<
    "-proxy=<ip:port|path>", std::string, common::SettingOptions{.legacy = true, .disallow_elision = true},
    "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled). May be a local file path prefixed with 'unix:' if the proxy supports it.">
    ::Category<OptionsCategory::CONNECTION>;

using ProxySetting2 = common::Setting<
    "-proxy=<ip:port>", std::string, common::SettingOptions{.legacy = true, .disallow_elision = true},
    "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled)">
    ::Category<OptionsCategory::CONNECTION>;

using ProxyrandomizeSetting = common::Setting<
    "-proxyrandomize", bool, common::SettingOptions{.legacy = true},
    "Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)">
    ::Default<DEFAULT_PROXYRANDOMIZE>
    ::Category<OptionsCategory::CONNECTION>;

using SeednodeSetting = common::Setting<
    "-seednode=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes. During startup, seednodes will be tried before dnsseeds.">
    ::Category<OptionsCategory::CONNECTION>;

using NetworkactiveSetting = common::Setting<
    "-networkactive", bool, common::SettingOptions{.legacy = true},
    "Enable all P2P network activity (default: 1). Can be changed by the setnetworkactive RPC command">
    ::Default<true>
    ::HelpArgs<>
    ::Category<OptionsCategory::CONNECTION>;

using TimeoutSetting = common::Setting<
    "-timeout=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Specify socket connection timeout in milliseconds. If an initial attempt to connect is unsuccessful after this amount of time, drop it (minimum: 1, default: %d)">
    ::Default<DEFAULT_CONNECT_TIMEOUT>
    ::Category<OptionsCategory::CONNECTION>;

using PeertimeoutSetting = common::Setting<
    "-peertimeout=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Specify a p2p connection timeout delay in seconds. After connecting to a peer, wait this amount of time before considering disconnection based on inactivity (minimum: 1, default: %d)">
    ::Default<DEFAULT_PEER_CONNECT_TIMEOUT>
    ::Category<OptionsCategory::CONNECTION>;

using TorcontrolSetting = common::Setting<
    "-torcontrol=<ip>:<port>", std::string, common::SettingOptions{.legacy = true},
    "Tor control host and port to use if onion listening enabled (default: %s). If no port is specified, the default port of %i will be used.">
    ::DefaultFn<[] { return DEFAULT_TOR_CONTROL; }>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DEFAULT_TOR_CONTROL, DEFAULT_TOR_CONTROL_PORT); }>
    ::Category<OptionsCategory::CONNECTION>;

using TorpasswordSetting = common::Setting<
    "-torpassword=<pass>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Tor control port password (default: empty)">
    ::Category<OptionsCategory::CONNECTION>;

using UpnpSetting = common::Setting<
    "-upnp", common::Unset, common::SettingOptions{.legacy = true},
    "">
    ::Category<OptionsCategory::HIDDEN>;

using NatpmpSetting = common::Setting<
    "-natpmp", bool, common::SettingOptions{.legacy = true},
    "Use PCP or NAT-PMP to map the listening port (default: %u)">
    ::Default<DEFAULT_NATPMP>
    ::Category<OptionsCategory::CONNECTION>;

using WhitebindSetting = common::Setting<
    "-whitebind=<[permissions@]addr>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Bind to the given address and add permission flags to the peers connecting to it. "
        "Use [host]:port notation for IPv6. Allowed permissions: %s. "
        "Specify multiple permissions separated by commas (default: download,noban,mempool,relay). Can be specified multiple times.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, util::Join(NET_PERMISSIONS_DOC, ", ")); }>
    ::Category<OptionsCategory::CONNECTION>;

using WhitelistSetting = common::Setting<
    "-whitelist=<[permissions@]IP address or network>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Add permission flags to the peers using the given IP address (e.g. 1.2.3.4) or "
        "CIDR-notated network (e.g. 1.2.3.0/24). Uses the same permissions as "
        "-whitebind. "
        "Additional flags \"in\" and \"out\" control whether permissions apply to incoming connections and/or manual (default: incoming only). "
        "Can be specified multiple times.">
    ::Category<OptionsCategory::CONNECTION>;

using ZmqpubhashblockSetting = common::Setting<
    "-zmqpubhashblock=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash block in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubhashtxSetting = common::Setting<
    "-zmqpubhashtx=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash transaction in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubrawblockSetting = common::Setting<
    "-zmqpubrawblock=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish raw block in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubrawtxSetting = common::Setting<
    "-zmqpubrawtx=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish raw transaction in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubsequenceSetting = common::Setting<
    "-zmqpubsequence=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash block and tx sequence in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubhashblockhwmSetting = common::Setting<
    "-zmqpubhashblockhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash block outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubhashtxhwmSetting = common::Setting<
    "-zmqpubhashtxhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash transaction outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubrawblockhwmSetting = common::Setting<
    "-zmqpubrawblockhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish raw block outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubrawtxhwmSetting = common::Setting<
    "-zmqpubrawtxhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish raw transaction outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqpubsequencehwmSetting = common::Setting<
    "-zmqpubsequencehwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash sequence message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using CheckblocksSetting = common::Setting<
    "-checkblocks=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "How many blocks to check at startup (default: %u, 0 = all)">
    ::Default<DEFAULT_CHECKBLOCKS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using ChecklevelSetting = common::Setting<
    "-checklevel=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "How thorough the block verification of -checkblocks is: %s (0-4, default: %u)">
    ::Default<DEFAULT_CHECKLEVEL>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, util::Join(CHECKLEVEL_DOC, ", "), DEFAULT_CHECKLEVEL); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckblockindexSetting = common::Setting<
    "-checkblockindex", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do a consistency check for the block tree, chainstate, and other validation data structures every <n> operations. Use 0 to disable. (default: %u, regtest: %u)">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams, const auto& regtestChainParams) { return strprintf(fmt, defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckblockindexSettingInt = common::Setting<
    "-checkblockindex", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true}>;

using CheckmempoolSetting = common::Setting<
    "-checkmempool=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Run mempool consistency checks every <n> transactions. Use 0 to disable. (default: %u, regtest: %u)">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams, const auto& regtestChainParams) { return strprintf(fmt, defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckpointsSetting = common::Setting<
    "-checkpoints", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Enable rejection of any forks from the known historical chain until block %s (default: %u)">
    ::HelpFn<[](const auto& fmt, const auto& defaultChainParams) { return strprintf(fmt, defaultChainParams->Checkpoints().GetHeight(), DEFAULT_CHECKPOINTS_ENABLED); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using DeprecatedrpcSetting = common::Setting<
    "-deprecatedrpc=<method>", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Allows deprecated RPC method(s) to be used">
    ::Category<OptionsCategory::DEBUG_TEST>;

using StopafterblockimportSetting = common::Setting<
    "-stopafterblockimport", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Stop running after importing blocks from disk (default: %u)">
    ::Default<DEFAULT_STOPAFTERBLOCKIMPORT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using StopatheightSetting = common::Setting<
    "-stopatheight", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Stop running after reaching the given height in the main chain (default: %u)">
    ::HelpArgs<node::DEFAULT_STOPATHEIGHT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitancestorcountSetting = common::Setting<
    "-limitancestorcount=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)">
    ::HelpArgs<DEFAULT_ANCESTOR_LIMIT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitancestorsizeSetting = common::Setting<
    "-limitancestorsize=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions whose size with all in-mempool ancestors exceeds <n> kilobytes (default: %u)">
    ::HelpArgs<DEFAULT_ANCESTOR_SIZE_LIMIT_KVB>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitdescendantcountSetting = common::Setting<
    "-limitdescendantcount=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u)">
    ::HelpArgs<DEFAULT_DESCENDANT_LIMIT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitdescendantsizeSetting = common::Setting<
    "-limitdescendantsize=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool descendants (default: %u).">
    ::HelpArgs<DEFAULT_DESCENDANT_SIZE_LIMIT_KVB>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CapturemessagesSetting = common::Setting<
    "-capturemessages", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Capture all P2P messages to disk">
    ::Category<OptionsCategory::DEBUG_TEST>;

using MocktimeSetting = common::Setting<
    "-mocktime=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Replace actual time with %s (default: 0)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, UNIX_EPOCH_TIME); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using MaxsigcachesizeSetting = common::Setting<
    "-maxsigcachesize=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)">
    ::HelpArgs<(DEFAULT_VALIDATION_CACHE_BYTES >> 20)>
    ::Category<OptionsCategory::DEBUG_TEST>;

using MaxtipageSetting = common::Setting<
    "-maxtipage=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Maximum tip age in seconds to consider node in initial block download (default: %u)">
    ::HelpArgs<Ticks<std::chrono::seconds>(DEFAULT_MAX_TIP_AGE)>
    ::Category<OptionsCategory::DEBUG_TEST>;

using PrintprioritySetting = common::Setting<
    "-printpriority", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Log transaction fee rate in %s/kvB when mining blocks (default: %u)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, node::DEFAULT_PRINT_MODIFIED_FEE); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using UacommentSetting = common::Setting<
    "-uacomment=<cmt>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Append comment to the user agent string">
    ::Category<OptionsCategory::DEBUG_TEST>;

using AcceptnonstdtxnSetting = common::Setting<
    "-acceptnonstdtxn", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Relay and mine \"non-standard\" transactions (test networks only; default: %u)">
    ::Default<DEFAULT_ACCEPT_NON_STD_TXN>
    ::Category<OptionsCategory::NODE_RELAY>;

using IncrementalrelayfeeSetting = common::Setting<
    "-incrementalrelayfee=<amt>", std::string, common::SettingOptions{.legacy = true, .debug_only = true},
    "Fee rate (in %s/kvB) used to define cost of relay, used for mempool limiting and replacement policy. (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using DustrelayfeeSetting = common::Setting<
    "-dustrelayfee=<amt>", std::string, common::SettingOptions{.legacy = true, .debug_only = true},
    "Fee rate (in %s/kvB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using AcceptstalefeeestimatesSetting = common::Setting<
    "-acceptstalefeeestimates", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Read fee estimates even if they are stale (%sdefault: %u) fee estimates are considered stale if they are %s hours old">
    ::Default<DEFAULT_ACCEPT_STALE_FEE_ESTIMATES>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, "regtest only; ", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES, Ticks<std::chrono::hours>(MAX_FILE_AGE)); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using BytespersigopSetting = common::Setting<
    "-bytespersigop", int64_t, common::SettingOptions{.legacy = true},
    "Equivalent bytes per sigop in transactions for relay and mining (default: %u)">
    ::DefaultFn<[] { return nBytesPerSigOp; }>
    ::HelpArgs<DEFAULT_BYTES_PER_SIGOP>
    ::Category<OptionsCategory::NODE_RELAY>;

using DatacarrierSetting = common::Setting<
    "-datacarrier", bool, common::SettingOptions{.legacy = true},
    "Relay and mine data carrier transactions (default: %u)">
    ::Default<DEFAULT_ACCEPT_DATACARRIER>
    ::Category<OptionsCategory::NODE_RELAY>;

using DatacarriersizeSetting = common::Setting<
    "-datacarriersize", int64_t, common::SettingOptions{.legacy = true},
    "Relay and mine transactions whose data-carrying raw scriptPubKey "
                             "is of this size or less (default: %u)">
    ::Default<MAX_OP_RETURN_RELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using PermitbaremultisigSetting = common::Setting<
    "-permitbaremultisig", bool, common::SettingOptions{.legacy = true},
    "Relay transactions creating non-P2SH multisig outputs (default: %u)">
    ::Default<DEFAULT_PERMIT_BAREMULTISIG>
    ::Category<OptionsCategory::NODE_RELAY>;

using MinrelaytxfeeSetting = common::Setting<
    "-minrelaytxfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "Fees (in %s/kvB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using WhitelistforcerelaySetting = common::Setting<
    "-whitelistforcerelay", bool, common::SettingOptions{.legacy = true},
    "Add 'forcerelay' permission to whitelisted peers with default permissions. This will relay transactions even if the transactions were already in the mempool. (default: %d)">
    ::Default<DEFAULT_WHITELISTFORCERELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using WhitelistrelaySetting = common::Setting<
    "-whitelistrelay", bool, common::SettingOptions{.legacy = true},
    "Add 'relay' permission to whitelisted peers with default permissions. This will accept relayed transactions even when not relaying transactions (default: %d)">
    ::Default<DEFAULT_WHITELISTRELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using BlockmaxweightSetting = common::Setting<
    "-blockmaxweight=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set maximum BIP141 block weight (default: %d)">
    ::HelpArgs<DEFAULT_BLOCK_MAX_WEIGHT>
    ::Category<OptionsCategory::BLOCK_CREATION>;

using BlockmintxfeeSetting = common::Setting<
    "-blockmintxfee=<amt>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Set lowest fee rate (in %s/kvB) for transactions to be included in block creation. (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)); }>
    ::Category<OptionsCategory::BLOCK_CREATION>;

using BlockversionSetting = common::Setting<
    "-blockversion=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Override block version to test forking scenarios">
    ::Category<OptionsCategory::BLOCK_CREATION>;

using RestSetting = common::Setting<
    "-rest", bool, common::SettingOptions{.legacy = true},
    "Accept public REST requests (default: %u)">
    ::Default<DEFAULT_REST_ENABLE>
    ::Category<OptionsCategory::RPC>;

using RpcdoccheckSetting = common::Setting<
    "-rpcdoccheck", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Throw a non-fatal error at runtime if the documentation for an RPC is incorrect (default: %u)">
    ::Default<DEFAULT_RPC_DOC_CHECK>
    ::Category<OptionsCategory::RPC>;

using ServerSetting = common::Setting<
    "-server", bool, common::SettingOptions{.legacy = true},
    "Accept command line and JSON-RPC commands">
    ::Category<OptionsCategory::RPC>;

using IpcbindSetting = common::Setting<
    "-ipcbind=<address>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Bind to Unix socket address and listen for incoming connections. Valid address values are \"unix\" to listen on the default path, <datadir>/node.sock, or \"unix:/custom/path\" to specify a custom path. Can be specified multiple times to listen on multiple paths. Default behavior is not to listen on any path. If relative paths are specified, they are interpreted relative to the network data directory. If paths include any parent directory components and the parent directories do not exist, they will be created.">
    ::Category<OptionsCategory::IPC>;

using DbcrashratioSettingHidden = common::Setting<
    "-dbcrashratio", std::optional<int64_t>, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ForcecompactdbSettingHidden = common::Setting<
    "-forcecompactdb", std::optional<bool>, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ChoosedatadirSettingHidden = common::Setting<
    "-choosedatadir", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using LangSettingHidden = common::Setting<
    "-lang=<lang>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MinSettingHidden = common::Setting<
    "-min", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ResetguisettingsSettingHidden = common::Setting<
    "-resetguisettings", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SplashSettingHidden = common::Setting<
    "-splash", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using UiplatformSettingHidden = common::Setting<
    "-uiplatform", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

#endif // BITCOIN_INIT_SETTINGS_H
