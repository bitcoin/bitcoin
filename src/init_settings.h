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
#include <node/caches.h>
#include <node/chainstatemanager_args.h>
#include <node/kernel_notifications.h>
#include <node/mempool_persist_args.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/settings.h>
#include <rpc/util.h>
#include <script/sigcache.h>
#include <torcontrol.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <zmq/zmqabstractnotifier.h>

#include <string>
#include <vector>

using CheckAddrManSetting = common::Setting<
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
    ::HelpFn<[](const auto& help) { return help.Format(BITCOIN_CONF_FILENAME); }>;

using ConfSettingPath = common::Setting<
    "-conf=<file>", fs::path, common::SettingOptions{.legacy = true}>
    ::DefaultFn<[] { return BITCOIN_CONF_FILENAME; }>;

using DataDirSetting = common::Setting<
    "-datadir=<dir>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Specify data directory">;

using DataDirSettingPath = common::Setting<
    "-datadir=<dir>", fs::path, common::SettingOptions{.legacy = true, .disallow_negation = true}>;

using RpcCookieFileSetting = common::Setting<
    "-rpccookiefile=<loc>", fs::path, common::SettingOptions{.legacy = true},
    "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)">
    ::Category<OptionsCategory::RPC>;

using RpcPasswordSetting = common::Setting<
    "-rpcpassword=<pw>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Password for JSON-RPC connections">
    ::Category<OptionsCategory::RPC>;

using RpcPortSetting = common::Setting<
    "-rpcport=<port>", int64_t, common::SettingOptions{.legacy = true, .network_only = true},
    "Listen for JSON-RPC connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u)">
    ::DefaultFn<[] { return BaseParams().RPCPort(); }>
    ::HelpFn<[](const auto& help, const auto& defaultBaseParams, const auto& testnetBaseParams, const auto& testnet4BaseParams, const auto& signetBaseParams, const auto& regtestBaseParams) { return help.Format(defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), testnet4BaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()); }>
    ::Category<OptionsCategory::RPC>;

using RpcUserSetting = common::Setting<
    "-rpcuser=<user>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Username for JSON-RPC connections">
    ::Category<OptionsCategory::RPC>;

using IpcBindSetting = common::Setting<
    "-ipcbind=<address>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Bind to Unix socket address and listen for incoming connections. Valid address values are \"unix\" to listen on the default path, <datadir>/node.sock, or \"unix:/custom/path\" to specify a custom path. Can be specified multiple times to listen on multiple paths. Default behavior is not to listen on any path. If relative paths are specified, they are interpreted relative to the network data directory. If paths include any parent directory components and the parent directories do not exist, they will be created.">
    ::Category<OptionsCategory::IPC>;

using DaemonSetting = common::Setting<
    "-daemon", bool, common::SettingOptions{.legacy = true},
    "Run in the background as a daemon and accept commands (default: %d)">
    ::HelpArgs<DEFAULT_DAEMON>;

using DaemonWaitSetting = common::Setting<
    "-daemonwait", bool, common::SettingOptions{.legacy = true},
    "Wait for initialization to be finished before exiting. This implies -daemon (default: %d)">
    ::Default<DEFAULT_DAEMONWAIT>;

using FastPruneSetting = common::Setting<
    "-fastprune", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Use smaller block files and lower minimum prune height for testing purposes">
    ::Category<OptionsCategory::DEBUG_TEST>;

using BlocksDirSetting = common::Setting<
    "-blocksdir=<dir>", std::string, common::SettingOptions{.legacy = true},
    "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)">;

using BlocksDirSettingPath = common::Setting<
    "-blocksdir=<dir>", fs::path, common::SettingOptions{.legacy = true}>;

using SettingsSetting = common::Setting<
    "-settings=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify path to dynamic settings data file. Can be disabled with -nosettings. File is written at runtime and not meant to be edited by users (use %s instead for custom settings). Relative paths will be prefixed by datadir location. (default: %s)">
    ::DefaultFn<[] { return BITCOIN_SETTINGS_FILENAME; }>
    ::HelpFn<[](const auto& help) { return help.Format(BITCOIN_CONF_FILENAME, BITCOIN_SETTINGS_FILENAME); }>;

using HelpDebugSetting = common::Setting<
    "-help-debug", bool, common::SettingOptions{.legacy = true},
    "Print help message with debugging options and exit">
    ::Category<OptionsCategory::DEBUG_TEST>;

using TestSetting = common::Setting<
    "-test=<option>", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Pass a test-only option. Options include : %s.">
    ::HelpFn<[](const auto& help) { return help.Format(util::Join(TEST_OPTIONS_DOC, ", ")); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using AllowIgnoredConfSetting = common::Setting<
    "-allowignoredconf", bool, common::SettingOptions{.legacy = true},
    "For backwards compatibility, treat an unused %s file in the datadir as a warning, not an error.">
    ::HelpFn<[](const auto& help) { return help.Format(BITCOIN_CONF_FILENAME); }>;

using RpcCookiePermsSetting = common::Setting<
    "-rpccookieperms=<readable-by>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Set permissions on the RPC auth cookie file so that it is readable by [owner|group|all] (default: owner [via umask 0077])">
    ::Category<OptionsCategory::RPC>;

using RpcAuthSetting = common::Setting<
    "-rpcauth=<userpw>", std::vector<std::string>, common::SettingOptions{.legacy = true, .sensitive = true},
    "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcauth. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times">
    ::Category<OptionsCategory::RPC>;

using RpcWhitelistDefaultSetting = common::Setting<
    "-rpcwhitelistdefault", bool, common::SettingOptions{.legacy = true},
    "Sets default behavior for rpc whitelisting. Unless rpcwhitelistdefault is set to 0, if any -rpcwhitelist is set, the rpc server acts as if all rpc users are subject to empty-unless-otherwise-specified whitelists. If rpcwhitelistdefault is set to 1 and no -rpcwhitelist is set, rpc server acts as if all rpc users are subject to empty whitelists.">
    ::Category<OptionsCategory::RPC>;

using RpcWhitelistSetting = common::Setting<
    "-rpcwhitelist=<whitelist>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Set a whitelist to filter incoming RPC calls for a specific user. The field <whitelist> comes in the format: <USERNAME>:<rpc 1>,<rpc 2>,...,<rpc n>. If multiple whitelists are set for a given user, they are set-intersected. See -rpcwhitelistdefault documentation for information on default whitelist behavior.">
    ::Category<OptionsCategory::RPC>;

using RpcAllowIpSetting = common::Setting<
    "-rpcallowip=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Allow JSON-RPC connections from specified source. Valid values for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0), a network/CIDR (e.g. 1.2.3.4/24), all ipv4 (0.0.0.0/0), or all ipv6 (::/0). RFC4193 is allowed only if -cjdnsreachable=0. This option can be specified multiple times">
    ::Category<OptionsCategory::RPC>;

using RpcBindSetting = common::Setting<
    "-rpcbind=<addr>[:port]", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost)">
    ::Category<OptionsCategory::RPC>;

using RpcServerTimeoutSetting = common::Setting<
    "-rpcservertimeout=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Timeout during HTTP requests (default: %d)">
    ::Default<DEFAULT_HTTP_SERVER_TIMEOUT>
    ::Category<OptionsCategory::RPC>;

using RpcWorkQueueSetting = common::Setting<
    "-rpcworkqueue=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Set the maximum depth of the work queue to service RPC calls (default: %d)">
    ::Default<DEFAULT_HTTP_WORKQUEUE>
    ::Category<OptionsCategory::RPC>;

using RpcThreadsSetting = common::Setting<
    "-rpcthreads=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set the number of threads to service RPC calls (default: %d)">
    ::Default<DEFAULT_HTTP_THREADS>
    ::Category<OptionsCategory::RPC>;

using AlertNotifySetting = common::Setting<
    "-alertnotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command when an alert is raised (%s in cmd is replaced by message)">;

using AssumeValidSetting = common::Setting<
    "-assumevalid=<hex>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet3: %s, testnet4: %s, signet: %s)">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams) { return help.Format(defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnet4ChainParams->GetConsensus().defaultAssumeValid.GetHex(), signetChainParams->GetConsensus().defaultAssumeValid.GetHex()); }>;

using BlocksXorSetting = common::Setting<
    "-blocksxor", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Whether an XOR-key applies to blocksdir *.dat files. "
                             "The created XOR-key will be zeros for an existing blocksdir or when `-blocksxor=0` is "
                             "set, and random for a freshly initialized blocksdir. "
                             "(default: %u)">
    ::HelpArgs<kernel::DEFAULT_XOR_BLOCKSDIR>;

using BlockNotifySetting = common::Setting<
    "-blocknotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command when the best block changes (%s in cmd is replaced by block hash)">;

using BlockReconstructionExtraTxnSetting = common::Setting<
    "-blockreconstructionextratxn=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Extra transactions to keep in memory for compact block reconstructions (default: %u)">
    ::HelpArgs<DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN>;

using BlocksOnlySetting = common::Setting<
    "-blocksonly", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Whether to reject transactions from network peers. Disables automatic broadcast and rebroadcast of transactions, unless the source peer has the 'forcerelay' permission. RPC transactions are not affected. (default: %u)">
    ::HelpArgs<DEFAULT_BLOCKSONLY>;

using CoinStatsIndexSetting = common::Setting<
    "-coinstatsindex", bool, common::SettingOptions{.legacy = true},
    "Maintain coinstats index used by the gettxoutsetinfo RPC (default: %u)">
    ::Default<DEFAULT_COINSTATSINDEX>;

using DbBatchSizeSetting = common::Setting<
    "-dbbatchsize", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Maximum database write batch size in bytes (default: %u)">
    ::HelpArgs<DEFAULT_DB_CACHE_BATCH>;

using DbCacheSetting = common::Setting<
    "-dbcache=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Maximum database cache size <n> MiB (minimum %d, default: %d). Make sure you have enough RAM. In addition, unused memory allocated to the mempool is shared with this cache (see -maxmempool).">
    ::HelpArgs<(MIN_DB_CACHE >> 20), (DEFAULT_DB_CACHE >> 20)>;

using IncludeConfSetting = common::Setting<
    "-includeconf=<file>", common::Unset, common::SettingOptions{.legacy = true},
    "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)">;

using LoadBlockSetting = common::Setting<
    "-loadblock=<file>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Imports blocks from external file on startup">;

using MaxMemPoolSetting = common::Setting<
    "-maxmempool=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Keep the transaction memory pool below <n> megabytes (default: %u)">
    ::HelpArgs<DEFAULT_MAX_MEMPOOL_SIZE_MB>;

using MempoolExpirySetting = common::Setting<
    "-mempoolexpiry=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Do not keep transactions in the mempool longer than <n> hours (default: %u)">
    ::HelpArgs<DEFAULT_MEMPOOL_EXPIRY_HOURS>;

using MinimumChainWorkSetting = common::Setting<
    "-minimumchainwork=<hex>", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Minimum work assumed to exist on a valid chain in hex (default: %s, testnet3: %s, testnet4: %s, signet: %s)">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams) { return help.Format(defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnet4ChainParams->GetConsensus().nMinimumChainWork.GetHex(), signetChainParams->GetConsensus().nMinimumChainWork.GetHex()); }>;

using ParSetting = common::Setting<
    "-par=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set the number of script verification threads (0 = auto, up to %d, <0 = leave that many cores free, default: %d)">
    ::Default<DEFAULT_SCRIPTCHECK_THREADS>
    ::HelpArgs<MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS>;

using PersistMempoolSetting = common::Setting<
    "-persistmempool", bool, common::SettingOptions{.legacy = true},
    "Whether to save the mempool on shutdown and load on restart (default: %u)">
    ::Default<node::DEFAULT_PERSIST_MEMPOOL>;

using PersistMempoolV1Setting = common::Setting<
    "-persistmempoolv1", bool, common::SettingOptions{.legacy = true},
    "Whether a mempool.dat file created by -persistmempool or the savemempool RPC will be written in the legacy format "
                             "(version 1) or the current format (version 2). This temporary option will be removed in the future. (default: %u)">
    ::HelpArgs<DEFAULT_PERSIST_V1_DAT>;

using PidSetting = common::Setting<
    "-pid=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)">
    ::DefaultFn<[] { return BITCOIN_PID_FILENAME; }>;

using PruneSetting = common::Setting<
    "-prune=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true},
    "Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >=%u = automatically prune block files to stay under the specified target size in MiB)">
    ::HelpArgs<MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024>;

using ReIndexSetting = common::Setting<
    "-reindex", bool, common::SettingOptions{.legacy = true},
    "If enabled, wipe chain state and block index, and rebuild them from blk*.dat files on disk. Also wipe and rebuild other optional indexes that are active. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.">;

using ReIndexChainstateSetting = common::Setting<
    "-reindex-chainstate", bool, common::SettingOptions{.legacy = true},
    "If enabled, wipe chain state, and rebuild it from blk*.dat files on disk. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.">;

using StartupNotifySetting = common::Setting<
    "-startupnotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command on startup.">;

using ShutdownNotifySetting = common::Setting<
    "-shutdownnotify=<cmd>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Execute command immediately before beginning shutdown. The need for shutdown may be urgent, so be careful not to delay it long (if the command doesn't require interaction with the server, consider having it fork into the background).">;

using TxIndexSetting = common::Setting<
    "-txindex", bool, common::SettingOptions{.legacy = true},
    "Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)">
    ::Default<DEFAULT_TXINDEX>;

using BlockFilterIndexSetting = common::Setting<
    "-blockfilterindex=<type>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Maintain an index of compact filters by block (default: %s, values: %s).">
    ::HelpFn<[](const auto& help) { return help.Format(DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()); }>;

using BlockFilterIndexSettingStr = common::Setting<
    "-blockfilterindex=<type>", std::string, common::SettingOptions{.legacy = true}>
    ::DefaultFn<[] { return DEFAULT_BLOCKFILTERINDEX; }>
    ::HelpFn<[](const auto& help) { return help.Format(DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()); }>;

using AddNodeSetting = common::Setting<
    "-addnode=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Add a node to connect to and attempt to keep the connection open (see the addnode RPC help for more info). This option can be specified multiple times to add multiple nodes; connections are limited to %u at a time and are counted separately from the -maxconnections limit.">
    ::HelpArgs<MAX_ADDNODE_CONNECTIONS>
    ::Category<OptionsCategory::CONNECTION>;

using AsMapSetting = common::Setting<
    "-asmap=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify asn mapping used for bucketing of the peers. Relative paths will be prefixed by the net-specific datadir location.">
    ::Category<OptionsCategory::CONNECTION>;

using BanTimeSetting = common::Setting<
    "-bantime=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Default duration (in seconds) of manually configured bans (default: %u)">
    ::Default<DEFAULT_MISBEHAVING_BANTIME>
    ::Category<OptionsCategory::CONNECTION>;

using BindSetting = common::Setting<
    "-bind=<addr>[:<port>][=onion]", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Bind to given address and always listen on it (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections (default: 127.0.0.1:%u=onion, testnet3: 127.0.0.1:%u=onion, testnet4: 127.0.0.1:%u=onion, signet: 127.0.0.1:%u=onion, regtest: 127.0.0.1:%u=onion)">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams, const auto& regtestChainParams) { return help.Format(defaultChainParams->GetDefaultPort() + 1, testnetChainParams->GetDefaultPort() + 1, testnet4ChainParams->GetDefaultPort() + 1, signetChainParams->GetDefaultPort() + 1, regtestChainParams->GetDefaultPort() + 1); }>
    ::Category<OptionsCategory::CONNECTION>;

using CJdnsReachableSetting = common::Setting<
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

using DnsSeedSetting = common::Setting<
    "-dnsseed", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Query for peer addresses via DNS lookup, if low on addresses (default: %u unless -connect used or -maxconnections=0)">
    ::HelpArgs<DEFAULT_DNSSEED>
    ::Category<OptionsCategory::CONNECTION>;

using ExternalIpSetting = common::Setting<
    "-externalip=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Specify your own public address">
    ::Category<OptionsCategory::CONNECTION>;

using FixedSeedsSetting = common::Setting<
    "-fixedseeds", bool, common::SettingOptions{.legacy = true},
    "Allow fixed seeds if DNS seeds don't provide peers (default: %u)">
    ::Default<DEFAULT_FIXEDSEEDS>
    ::Category<OptionsCategory::CONNECTION>;

using ForceDnsSeedSetting = common::Setting<
    "-forcednsseed", bool, common::SettingOptions{.legacy = true},
    "Always query for peer addresses via DNS lookup (default: %u)">
    ::Default<DEFAULT_FORCEDNSSEED>
    ::Category<OptionsCategory::CONNECTION>;

using ListenSetting = common::Setting<
    "-listen", bool, common::SettingOptions{.legacy = true},
    "Accept connections from outside (default: %u if no -proxy, -connect or -maxconnections=0)">
    ::Default<DEFAULT_LISTEN>
    ::Category<OptionsCategory::CONNECTION>;

using ListenOnionSetting = common::Setting<
    "-listenonion", bool, common::SettingOptions{.legacy = true},
    "Automatically create Tor onion service (default: %d)">
    ::Default<DEFAULT_LISTEN_ONION>
    ::Category<OptionsCategory::CONNECTION>;

using MaxConnectionsSetting = common::Setting<
    "-maxconnections=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maintain at most <n> automatic connections to peers (default: %u). This limit does not apply to connections manually added via -addnode or the addnode RPC, which have a separate limit of %u. It does not apply to short-lived private broadcast connections either, which have a separate limit of %u.">
    ::Default<DEFAULT_MAX_PEER_CONNECTIONS>
    ::HelpArgs<DEFAULT_MAX_PEER_CONNECTIONS, MAX_ADDNODE_CONNECTIONS, MAX_PRIVATE_BROADCAST_CONNECTIONS>
    ::Category<OptionsCategory::CONNECTION>;

using MaxReceiveBufferSetting = common::Setting<
    "-maxreceivebuffer=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)">
    ::Default<DEFAULT_MAXRECEIVEBUFFER>
    ::Category<OptionsCategory::CONNECTION>;

using MaxSendBufferSetting = common::Setting<
    "-maxsendbuffer=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Maximum per-connection memory usage for the send buffer, <n>*1000 bytes (default: %u)">
    ::Default<DEFAULT_MAXSENDBUFFER>
    ::Category<OptionsCategory::CONNECTION>;

using MaxUploadTargetSetting = common::Setting<
    "-maxuploadtarget=<n>", std::string, common::SettingOptions{.legacy = true},
    "Tries to keep outbound traffic under the given target per 24h. Limit does not apply to peers with 'download' permission or blocks created within past week. 0 = no limit (default: %s). Optional suffix units [k|K|m|M|g|G|t|T] (default: M). Lowercase is 1000 base while uppercase is 1024 base">
    ::HelpFn<[](const auto& help) { return help.Format(DEFAULT_MAX_UPLOAD_TARGET); }>
    ::Category<OptionsCategory::CONNECTION>;

using OnionSetting = common::Setting<
    "-onion=<ip:port|path>", std::string, common::SettingOptions{.legacy = true},
    "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy). May be a local file path prefixed with 'unix:'.">
    ::Category<OptionsCategory::CONNECTION>;

using OnionSetting2 = common::Setting<
    "-onion=<ip:port>", std::string, common::SettingOptions{.legacy = true},
    "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy)">
    ::Category<OptionsCategory::CONNECTION>;

using I2pSamSetting = common::Setting<
    "-i2psam=<ip:port>", std::string, common::SettingOptions{.legacy = true},
    "I2P SAM proxy to reach I2P peers and accept I2P connections">
    ::Category<OptionsCategory::CONNECTION>;

using I2pAcceptIncomingSetting = common::Setting<
    "-i2pacceptincoming", bool, common::SettingOptions{.legacy = true},
    "Whether to accept inbound I2P connections (default: %i). Ignored if -i2psam is not set. Listening for inbound I2P connections is done through the SAM proxy, not by binding to a local address and port.">
    ::Default<DEFAULT_I2P_ACCEPT_INCOMING>
    ::Category<OptionsCategory::CONNECTION>;

using OnlyNetSetting = common::Setting<
    "-onlynet=<net>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Make automatic outbound connections only to network <net> (%s). Inbound and manual connections are not affected by this option. It can be specified multiple times to allow multiple networks.">
    ::HelpFn<[](const auto& help) { return help.Format(util::Join(GetNetworkNames(), ", ")); }>
    ::Category<OptionsCategory::CONNECTION>;

using V2TransportSetting = common::Setting<
    "-v2transport", bool, common::SettingOptions{.legacy = true},
    "Support v2 transport (default: %u)">
    ::Default<DEFAULT_V2_TRANSPORT>
    ::Category<OptionsCategory::CONNECTION>;

using PeerBloomFiltersSetting = common::Setting<
    "-peerbloomfilters", bool, common::SettingOptions{.legacy = true},
    "Support filtering of blocks and transaction with bloom filters (default: %u)">
    ::Default<DEFAULT_PEERBLOOMFILTERS>
    ::Category<OptionsCategory::CONNECTION>;

using PeerBlockFiltersSetting = common::Setting<
    "-peerblockfilters", bool, common::SettingOptions{.legacy = true},
    "Serve compact block filters to peers per BIP 157 (default: %u)">
    ::Default<DEFAULT_PEERBLOCKFILTERS>
    ::Category<OptionsCategory::CONNECTION>;

using TxReconciliationSetting = common::Setting<
    "-txreconciliation", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Enable transaction reconciliations per BIP 330 (default: %d)">
    ::HelpArgs<DEFAULT_TXRECONCILIATION_ENABLE>
    ::Category<OptionsCategory::CONNECTION>;

using PortSetting = common::Setting<
    "-port=<port>", int64_t, common::SettingOptions{.legacy = true, .network_only = true},
    "Listen for connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u). Not relevant for I2P (see doc/i2p.md). If set to a value x, the default onion listening port will be set to x+1.">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& testnetChainParams, const auto& testnet4ChainParams, const auto& signetChainParams, const auto& regtestChainParams) { return help.Format(defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), testnet4ChainParams->GetDefaultPort(), signetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()); }>
    ::Category<OptionsCategory::CONNECTION>;

using ProxySetting = common::Setting<
    "-proxy=<ip>[:<port>]|unix:<path>[=<network>]", std::vector<std::string>, common::SettingOptions{.legacy = true, .disallow_elision = true},
    "%s">
    ::HelpFn<[](const auto& help, const auto& proxy_doc) { return help.Format(proxy_doc); }>
    ::Category<OptionsCategory::CONNECTION>;

using ProxySettingStr = common::Setting<
    "-proxy=<ip>[:<port>]|unix:<path>[=<network>]", std::string, common::SettingOptions{.legacy = true, .disallow_elision = true}>;

using ProxySetting2 = common::Setting<
    "-proxy=<ip>[:<port>][=<network>]", std::vector<std::string>, common::SettingOptions{.legacy = true, .disallow_elision = true},
    "%s">
    ::HelpFn<[](const auto& help, const auto& proxy_doc) { return help.Format(proxy_doc); }>
    ::Category<OptionsCategory::CONNECTION>;

using ProxySetting2Str = common::Setting<
    "-proxy=<ip>[:<port>][=<network>]", std::string, common::SettingOptions{.legacy = true, .disallow_elision = true}>;

using ProxyRandomizeSetting = common::Setting<
    "-proxyrandomize", bool, common::SettingOptions{.legacy = true},
    "Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)">
    ::Default<DEFAULT_PROXYRANDOMIZE>
    ::Category<OptionsCategory::CONNECTION>;

using SeedNodeSetting = common::Setting<
    "-seednode=<ip>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes. During startup, seednodes will be tried before dnsseeds.">
    ::Category<OptionsCategory::CONNECTION>;

using NetworkActiveSetting = common::Setting<
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

using PeerTimeoutSetting = common::Setting<
    "-peertimeout=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Specify a p2p connection timeout delay in seconds. After connecting to a peer, wait this amount of time before considering disconnection based on inactivity (minimum: 1, default: %d)">
    ::Default<DEFAULT_PEER_CONNECT_TIMEOUT>
    ::Category<OptionsCategory::CONNECTION>;

using TorControlSetting = common::Setting<
    "-torcontrol=<ip>:<port>", std::string, common::SettingOptions{.legacy = true},
    "Tor control host and port to use if onion listening enabled (default: %s). If no port is specified, the default port of %i will be used.">
    ::DefaultFn<[] { return DEFAULT_TOR_CONTROL; }>
    ::HelpFn<[](const auto& help) { return help.Format(DEFAULT_TOR_CONTROL, DEFAULT_TOR_CONTROL_PORT); }>
    ::Category<OptionsCategory::CONNECTION>;

using TorPasswordSetting = common::Setting<
    "-torpassword=<pass>", std::string, common::SettingOptions{.legacy = true, .sensitive = true},
    "Tor control port password (default: empty)">
    ::Category<OptionsCategory::CONNECTION>;

using NatPmpSetting = common::Setting<
    "-natpmp", bool, common::SettingOptions{.legacy = true},
    "Use PCP or NAT-PMP to map the listening port (default: %u)">
    ::Default<DEFAULT_NATPMP>
    ::Category<OptionsCategory::CONNECTION>;

using WhiteBindSetting = common::Setting<
    "-whitebind=<[permissions@]addr>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Bind to the given address and add permission flags to the peers connecting to it. "
        "Use [host]:port notation for IPv6. Allowed permissions: %s. "
        "Specify multiple permissions separated by commas (default: download,noban,mempool,relay). Can be specified multiple times.">
    ::HelpFn<[](const auto& help) { return help.Format(util::Join(NET_PERMISSIONS_DOC, ", ")); }>
    ::Category<OptionsCategory::CONNECTION>;

using WhiteListSetting = common::Setting<
    "-whitelist=<[permissions@]IP address or network>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Add permission flags to the peers using the given IP address (e.g. 1.2.3.4) or "
        "CIDR-notated network (e.g. 1.2.3.0/24). Uses the same permissions as "
        "-whitebind. "
        "Additional flags \"in\" and \"out\" control whether permissions apply to incoming connections and/or manual (default: incoming only). "
        "Can be specified multiple times.">
    ::Category<OptionsCategory::CONNECTION>;

using ZmqPubHashBlockSetting = common::Setting<
    "-zmqpubhashblock=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash block in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubHashTxSetting = common::Setting<
    "-zmqpubhashtx=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash transaction in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubRawBlockSetting = common::Setting<
    "-zmqpubrawblock=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish raw block in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubRawTxSetting = common::Setting<
    "-zmqpubrawtx=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish raw transaction in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubSequenceSetting = common::Setting<
    "-zmqpubsequence=<address>", common::Unset, common::SettingOptions{.legacy = true},
    "Enable publish hash block and tx sequence in <address>">
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubHashBlockHwmSetting = common::Setting<
    "-zmqpubhashblockhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash block outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubHashTxHwmSetting = common::Setting<
    "-zmqpubhashtxhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash transaction outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubRawBlockHwmSetting = common::Setting<
    "-zmqpubrawblockhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish raw block outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubRawTxHwmSetting = common::Setting<
    "-zmqpubrawtxhwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish raw transaction outbound message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using ZmqPubSequenceHwmSetting = common::Setting<
    "-zmqpubsequencehwm=<n>", common::Unset, common::SettingOptions{.legacy = true},
    "Set publish hash sequence message high water mark (default: %d)">
    ::HelpArgs<CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM>
    ::Category<OptionsCategory::ZMQ>;

using CheckBlocksSetting = common::Setting<
    "-checkblocks=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "How many blocks to check at startup (default: %u, 0 = all)">
    ::Default<DEFAULT_CHECKBLOCKS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckLevelSetting = common::Setting<
    "-checklevel=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "How thorough the block verification of -checkblocks is: %s (0-4, default: %u)">
    ::Default<DEFAULT_CHECKLEVEL>
    ::HelpFn<[](const auto& help) { return help.Format(util::Join(CHECKLEVEL_DOC, ", "), DEFAULT_CHECKLEVEL); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckBlockIndexSetting = common::Setting<
    "-checkblockindex", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do a consistency check for the block tree, chainstate, and other validation data structures every <n> operations. Use 0 to disable. (default: %u, regtest: %u)">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& regtestChainParams) { return help.Format(defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckBlockIndexSettingInt = common::Setting<
    "-checkblockindex", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true}>;

using CheckMempoolSetting = common::Setting<
    "-checkmempool=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Run mempool consistency checks every <n> transactions. Use 0 to disable. (default: %u, regtest: %u)">
    ::HelpFn<[](const auto& help, const auto& defaultChainParams, const auto& regtestChainParams) { return help.Format(defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CheckPointsSetting = common::Setting<
    "-checkpoints", common::Unset, common::SettingOptions{.legacy = true},
    "">
    ::Category<OptionsCategory::HIDDEN>;

using DeprecatedRpcSetting = common::Setting<
    "-deprecatedrpc=<method>", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Allows deprecated RPC method(s) to be used">
    ::Category<OptionsCategory::DEBUG_TEST>;

using StopAfterBlockImportSetting = common::Setting<
    "-stopafterblockimport", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Stop running after importing blocks from disk (default: %u)">
    ::Default<DEFAULT_STOPAFTERBLOCKIMPORT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using StopAtHeightSetting = common::Setting<
    "-stopatheight", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Stop running after reaching the given height in the main chain (default: %u). Blocks after target height may be processed during shutdown.">
    ::HelpArgs<node::DEFAULT_STOPATHEIGHT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitAncestorCountSetting = common::Setting<
    "-limitancestorcount=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Deprecated setting to not accept transactions if number of in-mempool ancestors is <n> or more (default: %u); replaced by cluster limits (see -limitclustercount) and only used by wallet for coin selection">
    ::HelpArgs<DEFAULT_ANCESTOR_LIMIT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitAncestorSizeSetting = common::Setting<
    "-limitancestorsize", common::Unset, common::SettingOptions{.legacy = true},
    "">
    ::Category<OptionsCategory::HIDDEN>;

using LimitDescendantSizeSetting = common::Setting<
    "-limitdescendantsize", common::Unset, common::SettingOptions{.legacy = true},
    "">
    ::Category<OptionsCategory::HIDDEN>;

using LimitDescendantCountSetting = common::Setting<
    "-limitdescendantcount=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Deprecated setting to not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u); replaced by cluster limits (see -limitclustercount) and only used by wallet for coin selection">
    ::HelpArgs<DEFAULT_DESCENDANT_LIMIT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitclustercountSetting = common::Setting<
    "-limitclustercount=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions into mempool which are directly or indirectly connected to <n> or more other unconfirmed transactions (default: %u, maximum: %u)">
    ::HelpArgs<DEFAULT_CLUSTER_LIMIT, MAX_CLUSTER_COUNT_LIMIT>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LimitclustersizeSetting = common::Setting<
    "-limitclustersize=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Do not accept transactions whose virtual size with all in-mempool connected transactions exceeds <n> kilobytes (default: %u)">
    ::HelpArgs<DEFAULT_CLUSTER_SIZE_LIMIT_KVB>
    ::Category<OptionsCategory::DEBUG_TEST>;

using CaptureMessagesSetting = common::Setting<
    "-capturemessages", std::optional<bool>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Capture all P2P messages to disk">
    ::Category<OptionsCategory::DEBUG_TEST>;

using MockTimeSetting = common::Setting<
    "-mocktime=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Replace actual time with %s (default: 0)">
    ::HelpFn<[](const auto& help) { return help.Format(UNIX_EPOCH_TIME); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using MaxSigCacheSizeSetting = common::Setting<
    "-maxsigcachesize=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)">
    ::HelpArgs<(DEFAULT_VALIDATION_CACHE_BYTES >> 20)>
    ::Category<OptionsCategory::DEBUG_TEST>;

using MaxTipAgeSetting = common::Setting<
    "-maxtipage=<n>", std::optional<int64_t>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Maximum tip age in seconds to consider node in initial block download (default: %u)">
    ::HelpArgs<Ticks<std::chrono::seconds>(DEFAULT_MAX_TIP_AGE)>
    ::Category<OptionsCategory::DEBUG_TEST>;

using PrintPrioritySetting = common::Setting<
    "-printpriority", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Log transaction fee rate in %s/kvB when mining blocks (default: %u)">
    ::HelpFn<[](const auto& help) { return help.Format(CURRENCY_UNIT, node::DEFAULT_PRINT_MODIFIED_FEE); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using UaCommentSetting = common::Setting<
    "-uacomment=<cmt>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Append comment to the user agent string">
    ::Category<OptionsCategory::DEBUG_TEST>;

using AcceptNonstdTxnSetting = common::Setting<
    "-acceptnonstdtxn", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Relay and mine \"non-standard\" transactions (test networks only; default: %u)">
    ::Default<DEFAULT_ACCEPT_NON_STD_TXN>
    ::Category<OptionsCategory::NODE_RELAY>;

using IncrementalRelayFeeSetting = common::Setting<
    "-incrementalrelayfee=<amt>", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Fee rate (in %s/kvB) used to define cost of relay, used for mempool limiting and replacement policy. (default: %s)">
    ::HelpFn<[](const auto& help) { return help.Format(CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using DustRelayFeeSetting = common::Setting<
    "-dustrelayfee=<amt>", std::optional<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Fee rate (in %s/kvB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)">
    ::HelpFn<[](const auto& help) { return help.Format(CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using AcceptStaleFeeEstimatesSetting = common::Setting<
    "-acceptstalefeeestimates", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Read fee estimates even if they are stale (%sdefault: %u) fee estimates are considered stale if they are %s hours old">
    ::Default<DEFAULT_ACCEPT_STALE_FEE_ESTIMATES>
    ::HelpFn<[](const auto& help) { return help.Format("regtest only; ", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES, Ticks<std::chrono::hours>(MAX_FILE_AGE)); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using BytesPerSigOpSetting = common::Setting<
    "-bytespersigop", int64_t, common::SettingOptions{.legacy = true},
    "Equivalent bytes per sigop in transactions for relay and mining (default: %u)">
    ::DefaultFn<[] { return nBytesPerSigOp; }>
    ::HelpArgs<DEFAULT_BYTES_PER_SIGOP>
    ::Category<OptionsCategory::NODE_RELAY>;

using DataCarrierSetting = common::Setting<
    "-datacarrier", bool, common::SettingOptions{.legacy = true},
    "Relay and mine data carrier transactions (default: %u)">
    ::Default<DEFAULT_ACCEPT_DATACARRIER>
    ::Category<OptionsCategory::NODE_RELAY>;

using DataCarrierSizeSetting = common::Setting<
    "-datacarriersize", int64_t, common::SettingOptions{.legacy = true},
    "Relay and mine transactions whose data-carrying raw scriptPubKeys in aggregate are of this size or less, allowing multiple outputs (default: %u)">
    ::Default<MAX_OP_RETURN_RELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using PermitBareMultiSigSetting = common::Setting<
    "-permitbaremultisig", bool, common::SettingOptions{.legacy = true},
    "Relay transactions creating non-P2SH multisig outputs (default: %u)">
    ::Default<DEFAULT_PERMIT_BAREMULTISIG>
    ::Category<OptionsCategory::NODE_RELAY>;

using MinRelayTxFeeSetting = common::Setting<
    "-minrelaytxfee=<amt>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Fees (in %s/kvB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)">
    ::HelpFn<[](const auto& help) { return help.Format(CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)); }>
    ::Category<OptionsCategory::NODE_RELAY>;

using PrivatebroadcastSetting = common::Setting<
    "-privatebroadcast", std::optional<bool>, common::SettingOptions{.legacy = true},
    "Broadcast transactions submitted via sendrawtransaction RPC using short-lived "
                       "connections through the Tor or I2P networks, without putting them in the mempool first. "
                       "Transactions submitted through the wallet are not affected by this option "
                       "(default: %u)">
    ::HelpArgs<DEFAULT_PRIVATE_BROADCAST>
    ::Category<OptionsCategory::NODE_RELAY>;

using WhiteListForceRelaySetting = common::Setting<
    "-whitelistforcerelay", bool, common::SettingOptions{.legacy = true},
    "Add 'forcerelay' permission to whitelisted peers with default permissions. This will relay transactions even if the transactions were already in the mempool. (default: %d)">
    ::Default<DEFAULT_WHITELISTFORCERELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using WhiteListRelaySetting = common::Setting<
    "-whitelistrelay", bool, common::SettingOptions{.legacy = true},
    "Add 'relay' permission to whitelisted peers with default permissions. This will accept relayed transactions even when not relaying transactions (default: %d)">
    ::Default<DEFAULT_WHITELISTRELAY>
    ::Category<OptionsCategory::NODE_RELAY>;

using BlockMaxWeightSetting = common::Setting<
    "-blockmaxweight=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Set maximum BIP141 block weight (default: %d)">
    ::HelpArgs<DEFAULT_BLOCK_MAX_WEIGHT>
    ::Category<OptionsCategory::BLOCK_CREATION>;

using BlockreservedweightSetting = common::Setting<
    "-blockreservedweight=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Reserve space for the fixed-size block header plus the largest coinbase transaction the mining software may add to the block. (default: %d).">
    ::HelpArgs<DEFAULT_BLOCK_RESERVED_WEIGHT>
    ::Category<OptionsCategory::BLOCK_CREATION>;

using BlockMinTxFeeSetting = common::Setting<
    "-blockmintxfee=<amt>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Set lowest fee rate (in %s/kvB) for transactions to be included in block creation. (default: %s)">
    ::HelpFn<[](const auto& help) { return help.Format(CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)); }>
    ::Category<OptionsCategory::BLOCK_CREATION>;

using BlockVersionSetting = common::Setting<
    "-blockversion=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Override block version to test forking scenarios">
    ::Category<OptionsCategory::BLOCK_CREATION>;

using RestSetting = common::Setting<
    "-rest", bool, common::SettingOptions{.legacy = true},
    "Accept public REST requests (default: %u)">
    ::Default<DEFAULT_REST_ENABLE>
    ::Category<OptionsCategory::RPC>;

using RpcDocCheckSetting = common::Setting<
    "-rpcdoccheck", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Throw a non-fatal error at runtime if the documentation for an RPC is incorrect (default: %u)">
    ::Default<DEFAULT_RPC_DOC_CHECK>
    ::Category<OptionsCategory::RPC>;

using ServerSetting = common::Setting<
    "-server", bool, common::SettingOptions{.legacy = true},
    "Accept command line and JSON-RPC commands">
    ::Category<OptionsCategory::RPC>;

using DbCrashRatioSettingHidden = common::Setting<
    "-dbcrashratio", std::optional<int64_t>, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ForceCompactDbSettingHidden = common::Setting<
    "-forcecompactdb", std::optional<bool>, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ChooseDataDirSettingHidden = common::Setting<
    "-choosedatadir", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using LangSettingHidden = common::Setting<
    "-lang=<lang>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MinSettingHidden = common::Setting<
    "-min", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ResetGuiSettingsSettingHidden = common::Setting<
    "-resetguisettings", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SplashSettingHidden = common::Setting<
    "-splash", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using UiPlatformSettingHidden = common::Setting<
    "-uiplatform", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

#endif // BITCOIN_INIT_SETTINGS_H
