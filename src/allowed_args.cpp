// Copyright (c) 2017 Stephen McCarthy
// Copyright (c) 2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <set>

#include "allowed_args.h"
#include "chainparams.h"
#include "dosman.h"
#include "httpserver.h"
#include "init.h"
#include "main.h"
#include "miner.h"
#include "netbase.h"
#include "policy/policy.h"
#include "qt/guiconstants.h"
#include "requestManager.h"
#include "script/sigcache.h"
#include "tinyformat.h"
#include "torcontrol.h"
#include "tweak.h"
#include "txdb.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

// These globals are needed here so bitcoin-cli can link
const std::string CURRENCY_UNIT = "BTC";
const std::string DEFAULT_TOR_CONTROL = "127.0.0.1:9051";
const char DEFAULT_RPCCONNECT[] = "127.0.0.1";

// Variables for traffic shaping.  Needed here so bitcoin-cli can link
/** Default value for the maximum amount of data that can be received in a burst */
const int64_t DEFAULT_MAX_RECV_BURST = std::numeric_limits<int64_t>::max();
/** Default value for the maximum amount of data that can be sent in a burst */
const int64_t DEFAULT_MAX_SEND_BURST = std::numeric_limits<int64_t>::max();
/** Default value for the average amount of data received per second */
const int64_t DEFAULT_AVE_RECV = std::numeric_limits<int64_t>::max();
/** Default value for the average amount of data sent per second */
const int64_t DEFAULT_AVE_SEND = std::numeric_limits<int64_t>::max();

namespace AllowedArgs
{
enum HelpMessageMode
{
    HMM_BITCOIND,
    HMM_BITCOIN_QT
};

static const int screenWidth = 79;
static const int optIndent = 2;
static const int msgIndent = 7;

std::string HelpMessageGroup(const std::string &message) { return std::string(message) + std::string("\n\n"); }
std::string HelpMessageOpt(const std::string &option, const std::string &message)
{
    return std::string(optIndent, ' ') + std::string(option) + std::string("\n") + std::string(msgIndent, ' ') +
           FormatParagraph(message, screenWidth - msgIndent, msgIndent) + std::string("\n\n");
}

AllowedArgs::AllowedArgs(bool permit_unrecognized) : m_permit_unrecognized(permit_unrecognized) {}
AllowedArgs &AllowedArgs::addHeader(const std::string &strHeader, bool debug)
{
    m_helpList.push_back(HelpComponent{strHeader + "\n\n", debug});
    return *this;
}

AllowedArgs &AllowedArgs::addDebugArg(const std::string &strArgsDefinition,
    CheckValueFunc checkValueFunc,
    const std::string &strHelp)
{
    return addArg(strArgsDefinition, checkValueFunc, strHelp, true);
}

AllowedArgs &AllowedArgs::addArg(const std::string &strArgsDefinition,
    CheckValueFunc checkValueFunc,
    const std::string &strHelp,
    bool debug)
{
    std::string strArgs = strArgsDefinition;
    std::string strExampleValue;
    size_t is_index = strArgsDefinition.find('=');
    if (is_index != std::string::npos)
    {
        strExampleValue = strArgsDefinition.substr(is_index + 1);
        strArgs = strArgsDefinition.substr(0, is_index);
    }

    if (strArgs == "")
        strArgs = ",";

    std::stringstream streamArgs(strArgs);
    std::string strArg;
    bool firstArg = true;
    while (std::getline(streamArgs, strArg, ','))
    {
        m_args[strArg] = checkValueFunc;

        std::string optionText = std::string(optIndent, ' ') + "-" + strArg;
        if (!strExampleValue.empty())
            optionText += "=" + strExampleValue;
        optionText += "\n";
        m_helpList.push_back(HelpComponent{optionText, debug || !firstArg});

        firstArg = false;
    }

    std::string helpText =
        std::string(msgIndent, ' ') + FormatParagraph(strHelp, screenWidth - msgIndent, msgIndent) + "\n\n";
    m_helpList.push_back(HelpComponent{helpText, debug});

    return *this;
}

void AllowedArgs::checkArg(const std::string &strArg, const std::string &strValue) const
{
    if (!m_args.count(strArg))
    {
        if (m_permit_unrecognized)
            return;
        throw std::runtime_error(strprintf(_("unrecognized option '%s'"), strArg));
    }

    if (!m_args.at(strArg)(strValue))
        throw std::runtime_error(strprintf(_("invalid value '%s' for option '%s'"), strValue, strArg));
}

std::string AllowedArgs::helpMessage() const
{
    const bool showDebug = GetBoolArg("-help-debug", false);
    std::string helpMessage;

    for (HelpComponent helpComponent : m_helpList)
        if (showDebug || !helpComponent.debug)
            helpMessage += helpComponent.text;

    return helpMessage;
}

//////////////////////////////////////////////////////////////////////////////
//
// CheckValueFunc functions
//

static const std::set<std::string> boolStrings{"", "1", "0", "t", "f", "y", "n", "true", "false", "yes", "no"};
static const std::set<char> intChars{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static const std::set<char> amountChars{'.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static bool validateString(const std::string &str, const std::set<char> &validChars)
{
    for (const char &c : str)
        if (!validChars.count(c))
            return false;
    return true;
}

static bool optionalBool(const std::string &str) { return (boolStrings.count(str) != 0); }
static bool requiredStr(const std::string &str) { return !str.empty(); }
static bool optionalStr(const std::string &str) { return true; }
static bool requiredInt(const std::string &str)
{
    if (str.empty() || str == "-")
        return false;

    // Allow the first character to be '-', to allow negative numbers.
    return validateString(str[0] == '-' ? str.substr(1) : str, intChars);
}

static bool optionalInt(const std::string &str)
{
    if (str.empty())
        return true;
    return requiredInt(str);
}

static bool requiredAmount(const std::string &str)
{
    if (str.empty())
        return false;
    return validateString(str, amountChars);
}

//////////////////////////////////////////////////////////////////////////////
//
// Argument definitions
//

// When adding new arguments to a category, please keep alphabetical ordering,
// where appropriate. Do not translate _(...) addDebugArg help text: there are
// many technical terms, and only a very small audience, so it would be an
// unnecessary stress to translators.

static void addHelpOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Help options:"))
        .addArg("?,h,help", optionalBool, _("This help message"))
        .addArg("version", optionalBool, _("Print version and exit"))
        .addArg("help-debug", optionalBool, _("Show all debugging options (usage: --help -help-debug)"));
}

static void addChainSelectionOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Chain selection options:"))
        .addArg("chain_nol", optionalBool, _("Use the no-limit blockchain"))
        .addArg("testnet", optionalBool, _("Use the test chain"))
        .addDebugArg("regtest", optionalBool,
            "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
            "This is intended for regression testing tools and app development.");
}

static void addConfigurationLocationOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Configuration location options:"))
        .addArg(
            "conf=<file>", requiredStr, strprintf(_("Specify configuration file (default: %s)"), BITCOIN_CONF_FILENAME))
        .addArg("datadir=<dir>", requiredStr, _("Specify data directory"));
}

static void addGeneralOptions(AllowedArgs &allowedArgs, HelpMessageMode mode)
{
    allowedArgs.addHeader(_("General options:"))
        .addArg("alertnotify=<cmd>", requiredStr, _("Execute command when a relevant alert is received or we see a "
                                                    "really long fork (%s in cmd is replaced by message)"))
        .addArg("blocknotify=<cmd>", requiredStr,
            _("Execute command when the best block changes (%s in cmd is replaced by block hash)"))
        .addDebugArg("blocksonly", optionalBool,
            strprintf(_("Whether to operate in a blocks only mode (default: %u)"), DEFAULT_BLOCKSONLY))
        .addArg("checkblocks=<n>", requiredInt,
            strprintf(_("How many blocks to check at startup (default: %u, 0 = all)"), DEFAULT_CHECKBLOCKS))
        .addArg("checklevel=<n>", requiredInt,
            strprintf(_("How thorough the block verification of -checkblocks is (0-4, default: %u)"),
                    DEFAULT_CHECKLEVEL));

#ifndef WIN32
    if (mode == HMM_BITCOIND)
        allowedArgs.addArg("daemon", optionalBool, _("Run in the background as a daemon and accept commands"));
#endif

    allowedArgs
        .addArg("dbcache=<n>", requiredInt, strprintf(_("Set database cache size in megabytes (%d to %d, default: %d)"),
                                                nMinDbCache, nMaxDbCache, nDefaultDbCache))
        .addArg("loadblock=<file>", requiredStr, _("Imports blocks from external blk000??.dat file on startup"))
        .addArg("maxorphantx=<n>", requiredInt,
            strprintf(_("Keep at most <n> unconnectable transactions in memory (default: %u)"),
                    DEFAULT_MAX_ORPHAN_TRANSACTIONS))
        .addArg("maxmempool=<n>", requiredInt,
            strprintf(
                    _("Keep the transaction memory pool below <n> megabytes (default: %u)"), DEFAULT_MAX_MEMPOOL_SIZE))
        .addArg("mempoolexpiry=<n>", requiredInt,
            strprintf(_("Do not keep transactions in the mempool longer than <n> hours (default: %u)"),
                    DEFAULT_MEMPOOL_EXPIRY))
        .addArg("orphanpoolexpiry=<n>", requiredInt,
            strprintf(_("Do not keep transactions in the orphanpool longer than <n> hours (default: %u)"),
                    DEFAULT_ORPHANPOOL_EXPIRY))
        .addArg("par=<n>", requiredInt, strprintf(_("Set the number of script verification threads (%u to %d, 0 = "
                                                    "auto, <0 = leave that many cores free, default: %d)"),
                                            -GetNumCores(), MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS))
        .addArg("parallel=<n>", optionalBool, strprintf(_("Turn Parallel Block Validation on or off (default: %u)"), 1))
#ifndef WIN32
        .addArg("pid=<file>", requiredStr, strprintf(_("Specify pid file (default: %s)"), BITCOIN_PID_FILENAME))
#endif
        .addArg("prune=<n>", requiredInt,
            strprintf(_("Reduce storage requirements by pruning (deleting) old blocks. This mode is incompatible with "
                        "-txindex and -rescan. "
                        "Warning: Reverting this setting requires re-downloading the entire blockchain. "
                        "(default: 0 = disable pruning blocks, >%u = target size in MiB to use for block files)"),
                    MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024))
        .addArg("reindex", optionalBool, _("Rebuild block chain index from current blk000??.dat files on startup"))
        .addArg("txindex", optionalBool,
            strprintf(_("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)"),
                    DEFAULT_TXINDEX));
}

static void addConnectionOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Connection options:"))
        .addArg("addnode=<ip>", requiredStr, _("Add a node to connect to and attempt to keep the connection open"))
        .addArg("banscore=<n>", requiredInt,
            strprintf(_("Threshold for disconnecting misbehaving peers (default: %u)"), DEFAULT_BANSCORE_THRESHOLD))
        .addArg("bantime=<n>", requiredInt,
            strprintf(_("Number of seconds to keep misbehaving peers from reconnecting (default: %u)"),
                    DEFAULT_MISBEHAVING_BANTIME))
        .addArg("bind=<addr>", requiredStr,
            _("Bind to given address and always listen on it. Use [host]:port notation for IPv6"))
        .addArg("bitnodes", optionalBool,
            _("Query for peer addresses via Bitnodes API, if low on addresses (default: 1 unless -connect)"))
        .addArg("blkretryinterval", requiredInt,
            strprintf(_("Time to wait before requesting a block from a different peer, in microseconds (default: %u)"),
                    DEFAULT_MIN_BLK_REQUEST_RETRY_INTERVAL))
        .addArg("connect=<ip>", optionalStr, _("Connect only to the specified node(s)"))
        .addArg("connect-thinblock=<ip:port>", requiredStr,
            _("Connect to a thinblock node(s). Blocks will only be downloaded from a thinblock peer.  If no "
              "connections "
              "are possible then regular blocks will then be downloaded form any other connected peers"))
        .addArg("discover", optionalBool,
            _("Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)"))
        .addArg("dns", optionalBool, _("Allow DNS lookups for -addnode, -seednode and -connect") + " " +
                                         strprintf(_("(default: %u)"), DEFAULT_NAME_LOOKUP))
        .addArg("dnsseed", optionalBool,
            _("Query for peer addresses via DNS lookup, if low on addresses (default: 1 unless -connect)"))
        .addArg("externalip=<ip>", requiredStr, _("Specify your own public address"))
        .addArg("forcebitnodes", optionalBool,
            strprintf(_("Always query for peer addresses via Bitnodes API (default: %u)"), DEFAULT_FORCEBITNODES))
        .addArg("forcednsseed", optionalBool,
            strprintf(_("Always query for peer addresses via DNS lookup (default: %u)"), DEFAULT_FORCEDNSSEED))
        .addArg("listen", optionalBool, _("Accept connections from outside (default: 1 if no -proxy or -connect)"))
        .addArg("listenonion", optionalBool,
            strprintf(_("Automatically create Tor hidden service (default: %d)"), DEFAULT_LISTEN_ONION))
        .addArg("maxconnections=<n>", optionalInt,
            strprintf(_("Maintain at most <n> connections to peers (default: %u)"), DEFAULT_MAX_PEER_CONNECTIONS))
        .addArg("maxoutconnections=<n>", requiredInt,
            strprintf(_("Initiate at most <n> connections to peers (default: %u).  If this number is higher than "
                        "--maxconnections, it will be reduced to --maxconnections"),
                    DEFAULT_MAX_OUTBOUND_CONNECTIONS))
        .addArg("maxreceivebuffer=<n>", requiredInt,
            strprintf(
                    _("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)"), DEFAULT_MAXRECEIVEBUFFER))
        .addArg("maxsendbuffer=<n>", requiredInt,
            strprintf(_("Maximum per-connection send buffer, <n>*1000 bytes (default: %u)"), DEFAULT_MAXSENDBUFFER))
        .addArg("min-xthin-nodes=<n>", requiredInt,
            strprintf(_("Minimum number of xthin nodes to automatically find and connect (default: %d)"),
                    MIN_XTHIN_NODES))
        .addArg("onion=<ip:port>", requiredStr,
            strprintf(_("Use separate SOCKS5 proxy to reach peers via Tor hidden services (default: %s)"), "-proxy"))
        .addArg("onlynet=<net>", requiredStr, _("Only connect to nodes in network <net> (ipv4, ipv6 or onion)"))
        .addArg("permitbaremultisig", optionalBool,
            strprintf(_("Relay non-P2SH multisig (default: %u)"), DEFAULT_PERMIT_BAREMULTISIG))
        .addArg("peerbloomfilters", optionalBool,
            strprintf(_("Support filtering of blocks and transaction with bloom filters (default: %u)"),
                    DEFAULT_PEERBLOOMFILTERS))
        .addDebugArg("enforcenodebloom", optionalBool,
            strprintf("Enforce minimum protocol version to limit use of bloom filters (default: %u)", 0))
        .addArg(
            "port=<port>", requiredInt, strprintf(_("Listen for connections on <port> (default: %u or testnet: %u)"),
                                            DEFAULT_MAINNET_PORT, DEFAULT_TESTNET_PORT))
        .addArg("proxy=<ip:port>", requiredStr, _("Connect through SOCKS5 proxy"))
        .addArg(
            "proxyrandomize", optionalBool,
            strprintf(
                _("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)"),
                DEFAULT_PROXYRANDOMIZE))
        .addArg("seednode=<ip>", requiredStr, _("Connect to a node to retrieve peer addresses, and disconnect"))
        .addArg("timeout=<n>", requiredInt,
            strprintf(
                    _("Specify connection timeout in milliseconds (minimum: 1, default: %d)"), DEFAULT_CONNECT_TIMEOUT))
        .addArg("torcontrol=<ip>:<port>", requiredStr,
            strprintf(_("Tor control port to use if onion listening enabled (default: %s)"), DEFAULT_TOR_CONTROL))
        .addArg("torpassword=<pass>", requiredStr, _("Tor control port password (default: empty)"))
        .addArg("txretryinterval", requiredInt,
            strprintf(_("Time to wait before requesting a tx from a different peer, in microseconds (default: %u)"),
                    DEFAULT_MIN_TX_REQUEST_RETRY_INTERVAL))
#ifdef USE_UPNP
#if USE_UPNP
        .addArg("upnp", optionalBool, _("Use UPnP to map the listening port (default: 1 when listening and no -proxy)"))
#else
        .addArg("upnp", optionalBool, _("Use UPnP to map the listening port (default: 0)"))
#endif
#endif
        .addArg("usednsseed=<host>", requiredStr, _("Add a custom DNS seed to use.  If at least one custom DNS seed "
                                                    "is set, the default DNS seeds will be ignored."))
        .addArg("whitebind=<addr>", requiredStr,
            _("Bind to given address and whitelist peers connecting to it. Use [host]:port notation for IPv6"))
        .addArg("whitelist=<netmask>", requiredStr,
            _("Whitelist peers connecting from the given netmask or IP address. Can be specified multiple times.") +
                " " + _("Whitelisted peers cannot be DoS banned and their transactions are always relayed, even if "
                        "they are already in the mempool, useful e.g. for a gateway"))
        .addArg("whitelistrelay", optionalBool, strprintf(_("Accept relayed transactions received from whitelisted "
                                                            "peers even when not relaying transactions (default: %d)"),
                                                    DEFAULT_WHITELISTRELAY))
        .addArg("whitelistforcerelay", optionalBool, strprintf(_("Force relay of transactions from whitelisted peers "
                                                                 "even they violate local relay policy (default: %d)"),
                                                         DEFAULT_WHITELISTFORCERELAY))
        .addArg(
            "maxuploadtarget=<n>", requiredInt,
            strprintf(
                _("Tries to keep outbound traffic under the given target (in MiB per 24h), 0 = no limit (default: %d)"),
                DEFAULT_MAX_UPLOAD_TARGET));
}

static void addWalletOptions(AllowedArgs &allowedArgs)
{
#ifdef ENABLE_WALLET
    allowedArgs.addHeader(_("Wallet options:"))
        .addArg("disablewallet", optionalBool, _("Do not load the wallet and disable wallet RPC calls"))
        .addArg(
            "keypool=<n>", requiredInt, strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE))
        .addArg(
            "fallbackfee=<amt>", requiredAmount,
            strprintf(
                _("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)))
        .addArg(
            "mintxfee=<amt>", requiredAmount,
            strprintf(
                _("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)))
        .addArg("paytxfee=<amt>", requiredAmount,
            strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"), CURRENCY_UNIT,
                    FormatMoney(DEFAULT_TRANSACTION_FEE)))
        .addArg("rescan", optionalBool, _("Rescan the block chain for missing wallet transactions on startup"))
        .addArg(
            "salvagewallet", optionalBool, _("Attempt to recover private keys from a corrupt wallet.dat on startup"))
        .addArg("sendfreetransactions", optionalBool,
            strprintf(_("Send transactions as zero-fee transactions if possible (default: %u)"),
                    DEFAULT_SEND_FREE_TRANSACTIONS))
        .addArg("spendzeroconfchange", optionalBool,
            strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"),
                    DEFAULT_SPEND_ZEROCONF_CHANGE))
        .addArg("txconfirmtarget=<n>", requiredInt,
            strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average "
                        "within n blocks (default: %u)"),
                    DEFAULT_TX_CONFIRM_TARGET))
        .addArg("maxtxfee=<amt>", requiredAmount,
            strprintf(_("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may "
                        "abort large transactions (default: %s)"),
                    CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)))
        .addArg("upgradewallet", optionalInt, _("Upgrade wallet to latest format on startup"))
        .addArg("wallet=<file>", requiredStr,
            _("Specify wallet file (within data directory)") + " " + strprintf(_("(default: %s)"), "wallet.dat"))
        .addArg("walletbroadcast", optionalBool,
            _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST))
        .addArg("walletnotify=<cmd>", requiredStr,
            _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"))
        .addArg("zapwallettxes=<mode>", optionalInt,
            _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on "
              "startup") +
                " " +
                _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));
#endif
}

static void addZmqOptions(AllowedArgs &allowedArgs)
{
#if ENABLE_ZMQ
    allowedArgs.addHeader(_("ZeroMQ notification options:"))
        .addArg("zmqpubhashblock=<address>", requiredStr, _("Enable publish hash block in <address>"))
        .addArg("zmqpubhashtx=<address>", requiredStr, _("Enable publish hash transaction in <address>"))
        .addArg("zmqpubrawblock=<address>", requiredStr, _("Enable publish raw block in <address>"))
        .addArg("zmqpubrawtx=<address>", requiredStr, _("Enable publish raw transaction in <address>"));
#endif
}

static void addDebuggingOptions(AllowedArgs &allowedArgs, HelpMessageMode mode)
{
    std::string debugCategories = "addrman, alert, bench, coindb, db, lock, rand, rpc, selectcoins, mempool, "
                                  "mempoolrej, net, proxy, prune, http, libevent, tor, zmq, thin, NWM";
    if (mode == HMM_BITCOIN_QT)
        debugCategories += ", qt";

    allowedArgs.addHeader(_("Debugging/Testing options:"))
        .addArg("uacomment=<cmt>", requiredStr, _("Append comment to the user agent string"))
        .addDebugArg("checkblockindex", optionalBool,
            strprintf("Do a full consistency check for mapBlockIndex, setBlockIndexCandidates, chainActive and "
                      "mapBlocksUnlinked occasionally (default: %u)",
                         false))
        .addDebugArg(
            "checkmempool=<n>", requiredInt, strprintf("Run checks every <n> transactions (default: %u)", false))
        .addDebugArg("checkpoints", optionalBool,
            strprintf("Disable expensive verification for known chain history (default: %u)",
                         DEFAULT_CHECKPOINTS_ENABLED))
#ifdef ENABLE_WALLET
        .addDebugArg("dblogsize=<n>", requiredInt,
            strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)",
                         DEFAULT_WALLET_DBLOGSIZE))
#endif
        .addDebugArg("disablesafemode", optionalBool,
            strprintf("Disable safemode, override a real safe mode event (default: %u)", DEFAULT_DISABLE_SAFEMODE))
        .addDebugArg("testsafemode", optionalBool, strprintf("Force safe mode (default: %u)", DEFAULT_TESTSAFEMODE))
        .addDebugArg("dropmessagestest=<n>", requiredInt, "Randomly drop 1 of every <n> network messages")
        .addDebugArg("fuzzmessagestest=<n>", requiredInt, "Randomly fuzz 1 of every <n> network messages")
        .addDebugArg("pvtest", optionalBool, strprintf("Slow down input checking to 1 every second (default: %u)", DEFAULT_PV_TESTMODE))
#ifdef ENABLE_WALLET
        .addDebugArg("flushwallet", optionalBool,
            strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET))
#endif
        .addDebugArg("stopafterblockimport", optionalBool,
            strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT))
        .addDebugArg("limitancestorcount=<n>", requiredInt,
            strprintf("Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)",
                         DEFAULT_ANCESTOR_LIMIT))
        .addDebugArg(
            "limitancestorsize=<n>", requiredInt, strprintf("Do not accept transactions whose size with all in-mempool "
                                                            "ancestors exceeds <n> kilobytes (default: %u)",
                                                      DEFAULT_ANCESTOR_SIZE_LIMIT))
        .addDebugArg(
            "limitdescendantcount=<n>", requiredInt, strprintf("Do not accept transactions if any ancestor would have "
                                                               "<n> or more in-mempool descendants (default: %u)",
                                                         DEFAULT_DESCENDANT_LIMIT))
        .addDebugArg("limitdescendantsize=<n>", requiredInt,
            strprintf("Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool "
                      "descendants (default: %u).",
                         DEFAULT_DESCENDANT_SIZE_LIMIT))
        .addArg("debug=<category>", optionalStr,
            strprintf(_("Output debugging information (default: %u, supplying <category> is optional)"), 0) + ". " +
                _("If <category> is not supplied or if <category> = 1, output all debugging information.") +
                _("<category> can be:") + " " + debugCategories + ".")
        .addArg("flextrans", optionalBool, "Accept and relay transactions of version 4")
        .addArg("ft-strict", optionalBool,
            "On incoming FlexTrans transactions reject tx that have not specified tokens. default: false")
        .addArg("gen", optionalBool, strprintf(_("Generate coins (default: %u)"), DEFAULT_GENERATE))
        .addArg("genproclimit=<n>", requiredInt,
            strprintf(_("Set the number of threads for coin generation if enabled (-1 = all cores, default: %d)"),
                    DEFAULT_GENERATE_THREADS))
        .addArg("gencoinbase=<pubkey>", requiredStr,
            "When generating coins a coinbase has to be provided in the form of a public key")
        .addArg(
            "logips", optionalBool, strprintf(_("Include IP addresses in debug output (default: %u)"), DEFAULT_LOGIPS))
        .addArg("logtimestamps", optionalBool,
            strprintf(_("Prepend debug output with timestamp (default: %u)"), DEFAULT_LOGTIMESTAMPS))
        .addDebugArg("logtimemicros", optionalBool,
            strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS))
        .addDebugArg("mocktime=<n>", requiredInt, "Replace actual time with <n> seconds since epoch (default: 0)")
        .addDebugArg("limitfreerelay=<n>", optionalInt,
            strprintf("Continuously rate-limit free transactions to <n>*1000 bytes per minute (default: %u)",
                         DEFAULT_LIMITFREERELAY))
        .addDebugArg("relaypriority", optionalBool,
            strprintf("Require high priority for relaying free or low-fee transactions (default: %u)",
                         DEFAULT_RELAYPRIORITY))
        .addDebugArg("maxsigcachesize=<n>", requiredInt,
            strprintf("Limit size of signature cache to <n> MiB (default: %u)", DEFAULT_MAX_SIG_CACHE_SIZE))
        .addArg("printtoconsole", optionalBool, _("Send trace/debug info to console instead of debug.log file"))
        .addDebugArg("printpriority", optionalBool,
            strprintf("Log transaction priority and fee per kB when mining blocks (default: %u)",
                         DEFAULT_PRINTPRIORITY))
#ifdef ENABLE_WALLET
        .addDebugArg("privdb", optionalBool,
            strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB))
#endif
        .addArg(
            "shrinkdebugfile", optionalBool, _("Shrink debug.log file on client startup (default: 1 when no -debug)"));
}

static void addNodeRelayOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Node relay options:"))
        .addDebugArg("acceptnonstdtxn", optionalBool,
            strprintf("Relay and mine \"non-standard\" transactions (%sdefault: %u)", "testnet/regtest only; ", true))
        .addArg("bytespersigop=<n>", requiredInt,
            strprintf(_("Minimum bytes per sigop in transactions we relay and mine (default: %u)"),
                    DEFAULT_BYTES_PER_SIGOP))
        .addArg("datacarrier", optionalBool,
            strprintf(_("Relay and mine data carrier transactions (default: %u)"), DEFAULT_ACCEPT_DATACARRIER))
        .addArg("datacarriersize=<n>", requiredInt,
            strprintf(_("Maximum size of data in data carrier transactions we relay and mine (default: %u)"),
                    MAX_OP_RETURN_RELAY))
        .addArg("excessiveacceptdepth=<n>", requiredInt,
            strprintf(_("Excessive blocks are accepted if this many blocks are mined on top of them (default: %u)"),
                    DEFAULT_EXCESSIVE_ACCEPT_DEPTH))
        .addArg("excessiveblocksize=<n>", requiredInt,
            strprintf(_("Blocks above this size in bytes are considered excessive.  (default: %u)"),
                    DEFAULT_EXCESSIVE_BLOCK_SIZE))
        .addArg("expeditedblock=<host>", requiredStr,
            _("Request expedited blocks from this host whenever we are connected to it"))
        .addArg("maxexpeditedblockrecipients=<n>", requiredInt,
            _("The maximum number of nodes this node will forward expedited blocks to"))
        .addArg("maxexpeditedtxrecipients=<n>", requiredInt,
            _("The maximum number of nodes this node will forward expedited transactions to"))
        .addArg("maxlimitertxfee=<amt>", requiredAmount,
            strprintf(_("Fees (in satoshi/byte) larger than this are always relayed (default: %s)"),
                    DEFAULT_MAXLIMITERTXFEE))
        .addArg("minlimitertxfee=<amt>", requiredAmount,
            strprintf(_("Fees (in satoshi/byte) smaller than this are considered "
                        "zero fee and subject to -limitfreerelay (default: %s)"),
                    DEFAULT_MINLIMITERTXFEE))
        .addArg("minrelaytxfee=<amt>", requiredAmount,
            strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for relaying, mining and "
                        "transaction creation (default: %s)"),
                    CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)))
        .addArg("receiveavg-<n>", requiredInt,
            strprintf(_("The average rate that data can be received in kB/s (default: %u)"), DEFAULT_AVE_RECV))
        .addArg("receiveburst=<n>", requiredInt,
            strprintf(_("The maximum rate that data can be received in kB/s.  If there has been a period of lower "
                        "than average data rates, the client may receive extra data to bring the average back to "
                        "'-receiveavg' but the data rate will not exceed this parameter (default: %u)"),
                    DEFAULT_MAX_RECV_BURST))
        .addArg("sendavg-<n>", requiredInt,
            strprintf(_("The average rate that data can be sent in kB/s (default: %u)"), DEFAULT_AVE_SEND))
        .addArg("sendburst-<n>", requiredInt,
            strprintf(_("The maximum rate that data can be sent in kB/s.  If there has been a period of lower than "
                        "average data rates, the client may send extra data to bring the average back to '-receiveavg' "
                        "but the data rate will not exceed this parameter (default: %u)"),
                    DEFAULT_MAX_SEND_BURST))
        .addArg("use-thinblocks", optionalBool, _("Enable thin blocks to speed up the relay of blocks (default: 1)"));
}

static void addBlockCreationOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("Block creation options:"))
        .addArg("blockminsize=<n>", requiredInt,
            strprintf(_("Set minimum block size in bytes (default: %u)"), DEFAULT_BLOCK_MIN_SIZE))
        .addArg("blockmaxsize=<n>", requiredInt,
            strprintf("Set maximum block size in bytes (default: %d)", DEFAULT_BLOCK_MAX_SIZE))
        .addArg("blockprioritysize=<n>", requiredInt,
            strprintf(_("Set maximum size of high-priority/low-fee transactions in bytes (default: %d)"),
                    DEFAULT_BLOCK_PRIORITY_SIZE))
        .addArg("blockversion=<n>", requiredInt, _("Generated block version number.  Value must be an integer"));
}

static void addRpcServerOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("RPC server options:"))
        .addArg("server", optionalBool, _("Accept command line and JSON-RPC commands"))
        .addArg("rest", optionalBool, strprintf(_("Accept public REST requests (default: %u)"), DEFAULT_REST_ENABLE))
        .addArg("rpcbind=<addr>", requiredStr,
            _("Bind to given address to listen for JSON-RPC connections. Use [host]:port notation for IPv6. This "
              "option can be specified multiple times (default: bind to all interfaces)"))
        .addArg("rpccookiefile=<loc>", requiredStr, _("Location of the auth cookie (default: data dir)"))
        .addArg("rpcuser=<user>", requiredStr, _("Username for JSON-RPC connections"))
        .addArg("rpcpassword=<pw>", requiredStr, _("Password for JSON-RPC connections"))
        .addArg("rpcauth=<userpw>", requiredStr,
            _("Username and hashed password for JSON-RPC connections. The field <userpw> comes in the format: "
              "<USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcuser. This option can be "
              "specified multiple times"))
        .addArg("rpcport=<port>", requiredInt,
            strprintf(_("Listen for JSON-RPC connections on <port> (default: %u or testnet: %u)"),
                    BaseParams(CBaseChainParams::MAIN).RPCPort(), BaseParams(CBaseChainParams::TESTNET).RPCPort()))
        .addArg("rpcallowip=<ip>", requiredStr,
            _("Allow JSON-RPC connections from specified source. Valid for <ip> are a single IP (e.g. 1.2.3.4), a "
              "network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24). This option can be "
              "specified multiple times"))
        .addArg("rpcthreads=<n>", requiredInt,
            strprintf(_("Set the number of threads to service RPC calls (default: %d)"), DEFAULT_HTTP_THREADS))
        .addDebugArg("rpcworkqueue=<n>", requiredInt,
            strprintf("Set the depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE))
        .addDebugArg("rpcservertimeout=<n>", requiredInt,
            strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT));
}

static void addUiOptions(AllowedArgs &allowedArgs)
{
    allowedArgs.addHeader(_("UI Options:"))
        .addDebugArg("allowselfsignedrootcertificates", optionalBool,
            strprintf("Allow self signed root certificates (default: %u)", DEFAULT_SELFSIGNED_ROOTCERTS))
        .addArg("choosedatadir", optionalBool,
            strprintf(_("Choose data directory on startup (default: %u)"), DEFAULT_CHOOSE_DATADIR))
        .addArg("lang=<lang>", requiredStr, _("Set language, for example \"de_DE\" (default: system locale)"))
        .addArg("min", optionalBool, _("Start minimized"))
        .addArg("rootcertificates=<file>", optionalStr,
            _("Set SSL root certificates for payment request (default: -system-)"))
        .addArg(
            "splash", optionalBool, strprintf(_("Show splash screen on startup (default: %u)"), DEFAULT_SPLASHSCREEN))
        .addArg("resetguisettings", optionalBool, _("Reset all settings changes made over the GUI"))
        .addDebugArg("uiplatform=<platform>", requiredStr,
            strprintf("Select platform to customize UI for (one of windows, macosx, other; default: %s)",
                         DEFAULT_UIPLATFORM));
}

static void addTweaks(AllowedArgs &allowedArgs, CTweakMap *pTweaks)
{
    CTweakMap::iterator i;

    allowedArgs.addHeader(_(PACKAGE_NAME) + _(" configuration tweaks:"));

    for (i = pTweaks->begin(); i != pTweaks->end(); ++i)
    {
        CTweakBase *tweak = i->second;
        std::string optName = tweak->GetName();

        if (dynamic_cast<CTweak<CAmount> *>(tweak))
            allowedArgs.addArg(optName + "=<amt>", requiredAmount, tweak->GetHelp());
        else if (dynamic_cast<CTweakRef<CAmount> *>(tweak))
            allowedArgs.addArg(optName + "=<amt>", requiredAmount, tweak->GetHelp());
        else if (dynamic_cast<CTweak<std::string> *>(tweak))
            allowedArgs.addArg(optName + "=<str>", requiredStr, tweak->GetHelp());
        else if (dynamic_cast<CTweakRef<std::string> *>(tweak))
            allowedArgs.addArg(optName + "=<str>", requiredStr, tweak->GetHelp());
        else
            allowedArgs.addArg(optName + "=<n>", requiredInt, tweak->GetHelp());
    }
}

static void addAllNodeOptions(AllowedArgs &allowedArgs, HelpMessageMode mode, CTweakMap *pTweaks)
{
    addHelpOptions(allowedArgs);
    addConfigurationLocationOptions(allowedArgs);
    addGeneralOptions(allowedArgs, mode);
    addConnectionOptions(allowedArgs);
    addWalletOptions(allowedArgs);
    addZmqOptions(allowedArgs);
    addDebuggingOptions(allowedArgs, mode);
    addChainSelectionOptions(allowedArgs);
    addNodeRelayOptions(allowedArgs);
    addBlockCreationOptions(allowedArgs);
    addRpcServerOptions(allowedArgs);
    if (pTweaks)
        addTweaks(allowedArgs, pTweaks);
    if (mode == HMM_BITCOIN_QT)
        addUiOptions(allowedArgs);
}

// bitcoin-cli does not know about tweaks so we have to silently ignore unknown options
BitcoinCli::BitcoinCli() : AllowedArgs(true)
{
    addHelpOptions(*this);
    addChainSelectionOptions(*this);
    addConfigurationLocationOptions(*this);

    addHeader(_("RPC client options:"))
        .addArg("rpcconnect=<ip>", requiredStr,
            strprintf(_("Send commands to node running on <ip> (default: %s)"), DEFAULT_RPCCONNECT))
        .addArg("rpcport=<port>", requiredInt,
            strprintf(_("Connect to JSON-RPC on <port> (default: %u or testnet: %u)"),
                    BaseParams(CBaseChainParams::MAIN).RPCPort(), BaseParams(CBaseChainParams::TESTNET).RPCPort()))
        .addArg("rpcwait", optionalBool, _("Wait for RPC server to start"))
        .addArg("rpcuser=<user>", requiredStr, _("Username for JSON-RPC connections"))
        .addArg("rpcpassword=<pw>", requiredStr, _("Password for JSON-RPC connections"))
        .addArg("rpcclienttimeout=<n>", requiredInt,
            strprintf(_("Timeout during HTTP requests (default: %d)"), DEFAULT_HTTP_CLIENT_TIMEOUT))
        .addArg("stdin", optionalBool, _("Read extra arguments from standard input, one per line until EOF/Ctrl-D "
                                         "(recommended for sensitive information such as passphrases)"));
}

Bitcoind::Bitcoind(CTweakMap *pTweaks) : AllowedArgs(false) { addAllNodeOptions(*this, HMM_BITCOIND, pTweaks); }
BitcoinQt::BitcoinQt(CTweakMap *pTweaks) : AllowedArgs(false) { addAllNodeOptions(*this, HMM_BITCOIN_QT, pTweaks); }
BitcoinTx::BitcoinTx() : AllowedArgs(false)
{
    addHelpOptions(*this);
    addChainSelectionOptions(*this);

    addHeader(_("Transaction options:"))
        .addArg("create", optionalBool, _("Create new, empty TX."))
        .addArg("json", optionalBool, _("Select JSON output"))
        .addArg("txid", optionalBool, _("Output only the hex-encoded transaction id of the resultant transaction."))
        .addDebugArg("", optionalBool, "Read hex-encoded bitcoin transaction from stdin.");
}

ConfigFile::ConfigFile(CTweakMap *pTweaks) : AllowedArgs(false)
{
    // Merges all allowed args from BitcoinCli, Bitcoind, and BitcoinQt.
    // Excludes args from BitcoinTx, because bitcoin-tx does not read
    // from the config file. Does not set a help message, because the
    // program does not output a config file help message anywhere.

    BitcoinCli bitcoinCli;
    Bitcoind bitcoind(pTweaks);
    BitcoinQt bitcoinQt;

    m_args.insert(bitcoinCli.getArgs().begin(), bitcoinCli.getArgs().end());
    m_args.insert(bitcoind.getArgs().begin(), bitcoind.getArgs().end());
    m_args.insert(bitcoinQt.getArgs().begin(), bitcoinQt.getArgs().end());
}

} // namespace AllowedArgs
