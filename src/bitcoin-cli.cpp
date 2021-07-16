// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparamsbase.h>
#include <clientversion.h>
#include <policy/feerate.h>
#include <rpc/client.h>
#include <rpc/mining.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/url.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <stdio.h>
#include <string>
#include <tuple>

#ifndef WIN32
#include <unistd.h>
#endif

#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <support/events.h>

#include <univalue.h>
#include <compat/stdin.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = urlDecode;

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT=900;
static constexpr int DEFAULT_WAIT_CLIENT_TIMEOUT = 0;
static const bool DEFAULT_NAMED=false;
static const int CONTINUE_EXECUTION=-1;
static constexpr int8_t UNKNOWN_NETWORK{-1};

/** Default number of blocks to generate for RPC generatetoaddress. */
static const std::string DEFAULT_NBLOCKS = "1";

/** Default -color setting. */
static const std::string DEFAULT_COLOR_SETTING{"auto"};

static void SetupCliArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    const auto signetBaseParams = CreateBaseChainParams(CBaseChainParams::SIGNET);
    const auto regtestBaseParams = CreateBaseChainParams(CBaseChainParams::REGTEST);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-conf=<file>", strprintf("Specify configuration file. Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-generate", strprintf("Generate blocks immediately, equivalent to RPC getnewaddress followed by RPC generatetoaddress. Optional positional integer arguments are number of blocks to generate (default: %s) and maximum iterations to try (default: %s), equivalent to RPC generatetoaddress nblocks and maxtries arguments. Example: bitcoin-cli -generate 4 1000", DEFAULT_NBLOCKS, DEFAULT_MAX_TRIES), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-addrinfo", "Get the number of addresses known to the node, per network and total.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-getinfo", "Get general information from the remote server. Note that unlike server-side RPC calls, the results of -getinfo is the result of multiple non-atomic requests. Some entries in the result may represent results from different states (e.g. wallet balance may be as of a different block from the chain state reported)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-netinfo", "Get network peer connection information from the remote server. An optional integer argument from 0 to 4 can be passed for different peers listings (default: 0). Pass \"help\" for detailed help documentation.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    SetupChainParamsBaseOptions(argsman);
    argsman.AddArg("-color=<when>", strprintf("Color setting for CLI output (default: %s). Valid values: always, auto (add color codes when standard output is connected to a terminal and OS is not WIN32), never.", DEFAULT_COLOR_SETTING), ArgsManager::ALLOW_STRING, OptionsCategory::OPTIONS);
    argsman.AddArg("-named", strprintf("Pass named instead of positional arguments (default: %s)", DEFAULT_NAMED), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcclienttimeout=<n>", strprintf("Timeout in seconds during HTTP requests, or 0 for no timeout. (default: %d)", DEFAULT_HTTP_CLIENT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcconnect=<ip>", strprintf("Send commands to node running on <ip> (default: %s)", DEFAULT_RPCCONNECT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcport=<port>", strprintf("Connect to JSON-RPC on <port> (default: %u, testnet: %u, signet: %u, regtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcwait", "Wait for RPC server to start", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcwaittimeout=<n>", strprintf("Timeout in seconds to wait for the RPC server to start, or 0 for no timeout. (default: %d)", DEFAULT_WAIT_CLIENT_TIMEOUT), ArgsManager::ALLOW_INT, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcwallet=<walletname>", "Send RPC for non-default wallet on RPC server (needs to exactly match corresponding -wallet option passed to bitcoind). This changes the RPC endpoint used, e.g. http://127.0.0.1:8332/wallet/<walletname>", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-stdin", "Read extra arguments from standard input, one per line until EOF/Ctrl-D (recommended for sensitive information such as passphrases). When combined with -stdinrpcpass, the first line from standard input is used for the RPC password.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-stdinrpcpass", "Read RPC password from standard input as a single line. When combined with -stdin, the first line from standard input is used for the RPC password. When combined with -stdinwalletpassphrase, -stdinrpcpass consumes the first line, and -stdinwalletpassphrase consumes the second.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-stdinwalletpassphrase", "Read wallet passphrase from standard input as a single line. When combined with -stdin, the first line from standard input is used for the wallet passphrase.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
}

/** libevent event log callback */
static void libevent_log_cb(int severity, const char *msg)
{
#ifndef EVENT_LOG_ERR // EVENT_LOG_ERR was added in 2.0.19; but before then _EVENT_LOG_ERR existed.
# define EVENT_LOG_ERR _EVENT_LOG_ERR
#endif
    // Ignore everything other than errors
    if (severity >= EVENT_LOG_ERR) {
        throw std::runtime_error(strprintf("libevent error: %s", msg));
    }
}

//
// Exception thrown on connection error.  This error is used to determine
// when to wait if -rpcwait is given.
//
class CConnectionFailed : public std::runtime_error
{
public:

    explicit inline CConnectionFailed(const std::string& msg) :
        std::runtime_error(msg)
    {}

};

//
// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
//
static int AppInitRPC(int argc, char* argv[])
{
    SetupCliArgs(gArgs);
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }
    if (argc < 2 || HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        std::string strUsage = PACKAGE_NAME " RPC client version " + FormatFullVersion() + "\n";
        if (!gArgs.IsArgSet("-version")) {
            strUsage += "\n"
                "Usage:  bitcoin-cli [options] <command> [params]  Send command to " PACKAGE_NAME "\n"
                "or:     bitcoin-cli [options] -named <command> [name=value]...  Send command to " PACKAGE_NAME " (with named arguments)\n"
                "or:     bitcoin-cli [options] help                List commands\n"
                "or:     bitcoin-cli [options] help <command>      Get help for a command\n";
            strUsage += "\n" + gArgs.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", strUsage);
        if (argc < 2) {
            tfm::format(std::cerr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (!CheckDataDirOption()) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", ""));
        return EXIT_FAILURE;
    }
    if (!gArgs.ReadConfigFiles(error, true)) {
        tfm::format(std::cerr, "Error reading configuration file: %s\n", error);
        return EXIT_FAILURE;
    }
    // Check for chain settings (BaseParams() calls are only valid after this clause)
    try {
        SelectBaseParams(gArgs.GetChainName());
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return CONTINUE_EXECUTION;
}


/** Reply structure for request_done to fill in */
struct HTTPReply
{
    HTTPReply(): status(0), error(-1) {}

    int status;
    int error;
    std::string body;
};

static std::string http_errorstring(int code)
{
    switch(code) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    case EVREQ_HTTP_TIMEOUT:
        return "timeout reached";
    case EVREQ_HTTP_EOF:
        return "EOF reached";
    case EVREQ_HTTP_INVALID_HEADER:
        return "error while reading header, or invalid header";
    case EVREQ_HTTP_BUFFER_ERROR:
        return "error encountered while reading or writing";
    case EVREQ_HTTP_REQUEST_CANCEL:
        return "request was canceled";
    case EVREQ_HTTP_DATA_TOO_LONG:
        return "response body is larger than allowed";
#endif
    default:
        return "unknown";
    }
}

static void http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);

    if (req == nullptr) {
        /* If req is nullptr, it means an error occurred while connecting: the
         * error code will have been passed to http_error_cb.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

#if LIBEVENT_VERSION_NUMBER >= 0x02010300
static void http_error_cb(enum evhttp_request_error err, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);
    reply->error = err;
}
#endif

/** Class that handles the conversion from a command-line to a JSON-RPC request,
 * as well as converting back to a JSON object that can be shown as result.
 */
class BaseRequestHandler
{
public:
    virtual ~BaseRequestHandler() {}
    virtual UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) = 0;
    virtual UniValue ProcessReply(const UniValue &batch_in) = 0;
};

/** Process addrinfo requests */
class AddrinfoRequestHandler : public BaseRequestHandler
{
private:
    static constexpr std::array m_networks{"ipv4", "ipv6", "torv2", "torv3", "i2p"};
    int8_t NetworkStringToId(const std::string& str) const
    {
        for (size_t i = 0; i < m_networks.size(); ++i) {
            if (str == m_networks.at(i)) return i;
        }
        return UNKNOWN_NETWORK;
    }

public:
    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        if (!args.empty()) {
            throw std::runtime_error("-addrinfo takes no arguments");
        }
        UniValue params{RPCConvertValues("getnodeaddresses", std::vector<std::string>{{"0"}})};
        return JSONRPCRequestObj("getnodeaddresses", params, 1);
    }

    UniValue ProcessReply(const UniValue& reply) override
    {
        if (!reply["error"].isNull()) return reply;
        const std::vector<UniValue>& nodes{reply["result"].getValues()};
        if (!nodes.empty() && nodes.at(0)["network"].isNull()) {
            throw std::runtime_error("-addrinfo requires bitcoind server to be running v22.0 and up");
        }
        // Count the number of peers we know by network, including torv2 versus torv3.
        std::array<uint64_t, m_networks.size()> counts{{}};
        for (const UniValue& node : nodes) {
            std::string network_name{node["network"].get_str()};
            if (network_name == "onion") {
                network_name = node["address"].get_str().size() > 22 ? "torv3" : "torv2";
            }
            const int8_t network_id{NetworkStringToId(network_name)};
            if (network_id == UNKNOWN_NETWORK) continue;
            ++counts.at(network_id);
        }
        // Prepare result to return to user.
        UniValue result{UniValue::VOBJ}, addresses{UniValue::VOBJ};
        uint64_t total{0}; // Total address count
        for (size_t i = 0; i < m_networks.size(); ++i) {
            addresses.pushKV(m_networks.at(i), counts.at(i));
            total += counts.at(i);
        }
        addresses.pushKV("total", total);
        result.pushKV("addresses_known", addresses);
        return JSONRPCReplyObj(result, NullUniValue, 1);
    }
};

/** Process getinfo requests */
class GetinfoRequestHandler: public BaseRequestHandler
{
public:
    const int ID_NETWORKINFO = 0;
    const int ID_BLOCKCHAININFO = 1;
    const int ID_WALLETINFO = 2;
    const int ID_BALANCES = 3;

    /** Create a simulated `getinfo` request. */
    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        if (!args.empty()) {
            throw std::runtime_error("-getinfo takes no arguments");
        }
        UniValue result(UniValue::VARR);
        result.push_back(JSONRPCRequestObj("getnetworkinfo", NullUniValue, ID_NETWORKINFO));
        result.push_back(JSONRPCRequestObj("getblockchaininfo", NullUniValue, ID_BLOCKCHAININFO));
        result.push_back(JSONRPCRequestObj("getwalletinfo", NullUniValue, ID_WALLETINFO));
        result.push_back(JSONRPCRequestObj("getbalances", NullUniValue, ID_BALANCES));
        return result;
    }

    /** Collect values from the batch and form a simulated `getinfo` reply. */
    UniValue ProcessReply(const UniValue &batch_in) override
    {
        UniValue result(UniValue::VOBJ);
        const std::vector<UniValue> batch = JSONRPCProcessBatchReply(batch_in);
        // Errors in getnetworkinfo() and getblockchaininfo() are fatal, pass them on;
        // getwalletinfo() and getbalances() are allowed to fail if there is no wallet.
        if (!batch[ID_NETWORKINFO]["error"].isNull()) {
            return batch[ID_NETWORKINFO];
        }
        if (!batch[ID_BLOCKCHAININFO]["error"].isNull()) {
            return batch[ID_BLOCKCHAININFO];
        }
        result.pushKV("version", batch[ID_NETWORKINFO]["result"]["version"]);
        result.pushKV("blocks", batch[ID_BLOCKCHAININFO]["result"]["blocks"]);
        result.pushKV("headers", batch[ID_BLOCKCHAININFO]["result"]["headers"]);
        result.pushKV("verificationprogress", batch[ID_BLOCKCHAININFO]["result"]["verificationprogress"]);
        result.pushKV("timeoffset", batch[ID_NETWORKINFO]["result"]["timeoffset"]);

        UniValue connections(UniValue::VOBJ);
        connections.pushKV("in", batch[ID_NETWORKINFO]["result"]["connections_in"]);
        connections.pushKV("out", batch[ID_NETWORKINFO]["result"]["connections_out"]);
        connections.pushKV("total", batch[ID_NETWORKINFO]["result"]["connections"]);
        result.pushKV("connections", connections);

        result.pushKV("proxy", batch[ID_NETWORKINFO]["result"]["networks"][0]["proxy"]);
        result.pushKV("difficulty", batch[ID_BLOCKCHAININFO]["result"]["difficulty"]);
        result.pushKV("chain", UniValue(batch[ID_BLOCKCHAININFO]["result"]["chain"]));
        if (!batch[ID_WALLETINFO]["result"].isNull()) {
            result.pushKV("has_wallet", true);
            result.pushKV("keypoolsize", batch[ID_WALLETINFO]["result"]["keypoolsize"]);
            result.pushKV("walletname", batch[ID_WALLETINFO]["result"]["walletname"]);
            if (!batch[ID_WALLETINFO]["result"]["unlocked_until"].isNull()) {
                result.pushKV("unlocked_until", batch[ID_WALLETINFO]["result"]["unlocked_until"]);
            }
            result.pushKV("paytxfee", batch[ID_WALLETINFO]["result"]["paytxfee"]);
        }
        if (!batch[ID_BALANCES]["result"].isNull()) {
            result.pushKV("balance", batch[ID_BALANCES]["result"]["mine"]["trusted"]);
        }
        result.pushKV("relayfee", batch[ID_NETWORKINFO]["result"]["relayfee"]);
        result.pushKV("warnings", batch[ID_NETWORKINFO]["result"]["warnings"]);
        return JSONRPCReplyObj(result, NullUniValue, 1);
    }
};

/** Process netinfo requests */
class NetinfoRequestHandler : public BaseRequestHandler
{
private:
    static constexpr uint8_t MAX_DETAIL_LEVEL{4};
    static constexpr std::array m_networks{"ipv4", "ipv6", "onion", "i2p"};
    std::array<std::array<uint16_t, m_networks.size() + 1>, 3> m_counts{{{}}}; //!< Peer counts by (in/out/total, networks/total)
    uint8_t m_block_relay_peers_count{0};
    uint8_t m_manual_peers_count{0};
    int8_t NetworkStringToId(const std::string& str) const
    {
        for (size_t i = 0; i < m_networks.size(); ++i) {
            if (str == m_networks.at(i)) return i;
        }
        return UNKNOWN_NETWORK;
    }
    uint8_t m_details_level{0}; //!< Optional user-supplied arg to set dashboard details level
    bool DetailsRequested() const { return m_details_level > 0 && m_details_level < 5; }
    bool IsAddressSelected() const { return m_details_level == 2 || m_details_level == 4; }
    bool IsVersionSelected() const { return m_details_level == 3 || m_details_level == 4; }
    bool m_is_asmap_on{false};
    size_t m_max_addr_length{0};
    size_t m_max_addr_processed_length{5};
    size_t m_max_addr_rate_limited_length{6};
    size_t m_max_age_length{5};
    size_t m_max_id_length{2};
    struct Peer {
        std::string addr;
        std::string sub_version;
        std::string conn_type;
        std::string network;
        std::string age;
        double min_ping;
        double ping;
        int64_t addr_processed;
        int64_t addr_rate_limited;
        int64_t last_blck;
        int64_t last_recv;
        int64_t last_send;
        int64_t last_trxn;
        int id;
        int mapped_as;
        int version;
        bool is_bip152_hb_from;
        bool is_bip152_hb_to;
        bool is_block_relay;
        bool is_outbound;
        bool operator<(const Peer& rhs) const { return std::tie(is_outbound, min_ping) < std::tie(rhs.is_outbound, rhs.min_ping); }
    };
    std::vector<Peer> m_peers;
    std::string ChainToString() const
    {
        if (gArgs.GetChainName() == CBaseChainParams::TESTNET) return " testnet";
        if (gArgs.GetChainName() == CBaseChainParams::SIGNET) return " signet";
        if (gArgs.GetChainName() == CBaseChainParams::REGTEST) return " regtest";
        return "";
    }
    std::string PingTimeToString(double seconds) const
    {
        if (seconds < 0) return "";
        const double milliseconds{round(1000 * seconds)};
        return milliseconds > 999999 ? "-" : ToString(milliseconds);
    }
    std::string ConnectionTypeForNetinfo(const std::string& conn_type) const
    {
        if (conn_type == "outbound-full-relay") return "full";
        if (conn_type == "block-relay-only") return "block";
        if (conn_type == "manual" || conn_type == "feeler") return conn_type;
        if (conn_type == "addr-fetch") return "addr";
        return "";
    }
    const int64_t m_time_now{GetTimeSeconds()};

public:
    static constexpr int ID_PEERINFO = 0;
    static constexpr int ID_NETWORKINFO = 1;

    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        if (!args.empty()) {
            uint8_t n{0};
            if (ParseUInt8(args.at(0), &n)) {
                m_details_level = std::min(n, MAX_DETAIL_LEVEL);
            } else {
                throw std::runtime_error(strprintf("invalid -netinfo argument: %s\nFor more information, run: bitcoin-cli -netinfo help", args.at(0)));
            }
        }
        UniValue result(UniValue::VARR);
        result.push_back(JSONRPCRequestObj("getpeerinfo", NullUniValue, ID_PEERINFO));
        result.push_back(JSONRPCRequestObj("getnetworkinfo", NullUniValue, ID_NETWORKINFO));
        return result;
    }

    UniValue ProcessReply(const UniValue& batch_in) override
    {
        const std::vector<UniValue> batch{JSONRPCProcessBatchReply(batch_in)};
        if (!batch[ID_PEERINFO]["error"].isNull()) return batch[ID_PEERINFO];
        if (!batch[ID_NETWORKINFO]["error"].isNull()) return batch[ID_NETWORKINFO];

        const UniValue& networkinfo{batch[ID_NETWORKINFO]["result"]};
        if (networkinfo["version"].get_int() < 209900) {
            throw std::runtime_error("-netinfo requires bitcoind server to be running v0.21.0 and up");
        }

        // Count peer connection totals, and if DetailsRequested(), store peer data in a vector of structs.
        for (const UniValue& peer : batch[ID_PEERINFO]["result"].getValues()) {
            const std::string network{peer["network"].get_str()};
            const int8_t network_id{NetworkStringToId(network)};
            if (network_id == UNKNOWN_NETWORK) continue;
            const bool is_outbound{!peer["inbound"].get_bool()};
            const bool is_block_relay{!peer["relaytxes"].get_bool()};
            const std::string conn_type{peer["connection_type"].get_str()};
            ++m_counts.at(is_outbound).at(network_id);        // in/out by network
            ++m_counts.at(is_outbound).at(m_networks.size()); // in/out overall
            ++m_counts.at(2).at(network_id);                  // total by network
            ++m_counts.at(2).at(m_networks.size());           // total overall
            if (conn_type == "block-relay-only") ++m_block_relay_peers_count;
            if (conn_type == "manual") ++m_manual_peers_count;
            if (DetailsRequested()) {
                // Push data for this peer to the peers vector.
                const int peer_id{peer["id"].get_int()};
                const int mapped_as{peer["mapped_as"].isNull() ? 0 : peer["mapped_as"].get_int()};
                const int version{peer["version"].get_int()};
                const int64_t addr_processed{peer["addr_processed"].isNull() ? 0 : peer["addr_processed"].get_int64()};
                const int64_t addr_rate_limited{peer["addr_rate_limited"].isNull() ? 0 : peer["addr_rate_limited"].get_int64()};
                const int64_t conn_time{peer["conntime"].get_int64()};
                const int64_t last_blck{peer["last_block"].get_int64()};
                const int64_t last_recv{peer["lastrecv"].get_int64()};
                const int64_t last_send{peer["lastsend"].get_int64()};
                const int64_t last_trxn{peer["last_transaction"].get_int64()};
                const double min_ping{peer["minping"].isNull() ? -1 : peer["minping"].get_real()};
                const double ping{peer["pingtime"].isNull() ? -1 : peer["pingtime"].get_real()};
                const std::string addr{peer["addr"].get_str()};
                const std::string age{conn_time == 0 ? "" : ToString((m_time_now - conn_time) / 60)};
                const std::string sub_version{peer["subver"].get_str()};
                const bool is_bip152_hb_from{peer["bip152_hb_from"].get_bool()};
                const bool is_bip152_hb_to{peer["bip152_hb_to"].get_bool()};
                m_peers.push_back({addr, sub_version, conn_type, network, age, min_ping, ping, addr_processed, addr_rate_limited, last_blck, last_recv, last_send, last_trxn, peer_id, mapped_as, version, is_bip152_hb_from, is_bip152_hb_to, is_block_relay, is_outbound});
                m_max_addr_length = std::max(addr.length() + 1, m_max_addr_length);
                m_max_addr_processed_length = std::max(ToString(addr_processed).length(), m_max_addr_processed_length);
                m_max_addr_rate_limited_length = std::max(ToString(addr_rate_limited).length(), m_max_addr_rate_limited_length);
                m_max_age_length = std::max(age.length(), m_max_age_length);
                m_max_id_length = std::max(ToString(peer_id).length(), m_max_id_length);
                m_is_asmap_on |= (mapped_as != 0);
            }
        }

        // Generate report header.
        std::string result{strprintf("%s %s%s - %i%s\n\n", PACKAGE_NAME, FormatFullVersion(), ChainToString(), networkinfo["protocolversion"].get_int(), networkinfo["subversion"].get_str())};

        // Report detailed peer connections list sorted by direction and minimum ping time.
        if (DetailsRequested() && !m_peers.empty()) {
            std::sort(m_peers.begin(), m_peers.end());
            result += strprintf("<->   type   net  mping   ping send recv  txn  blk  hb %*s%*s%*s ",
                                m_max_addr_processed_length, "addrp",
                                m_max_addr_rate_limited_length, "addrl",
                                m_max_age_length, "age");
            if (m_is_asmap_on) result += " asmap ";
            result += strprintf("%*s %-*s%s\n", m_max_id_length, "id", IsAddressSelected() ? m_max_addr_length : 0, IsAddressSelected() ? "address" : "", IsVersionSelected() ? "version" : "");
            for (const Peer& peer : m_peers) {
                std::string version{ToString(peer.version) + peer.sub_version};
                result += strprintf(
                    "%3s %6s %5s%7s%7s%5s%5s%5s%5s  %2s %*s%*s%*s%*i %*s %-*s%s\n",
                    peer.is_outbound ? "out" : "in",
                    ConnectionTypeForNetinfo(peer.conn_type),
                    peer.network,
                    PingTimeToString(peer.min_ping),
                    PingTimeToString(peer.ping),
                    peer.last_send == 0 ? "" : ToString(m_time_now - peer.last_send),
                    peer.last_recv == 0 ? "" : ToString(m_time_now - peer.last_recv),
                    peer.last_trxn == 0 ? "" : ToString((m_time_now - peer.last_trxn) / 60),
                    peer.last_blck == 0 ? "" : ToString((m_time_now - peer.last_blck) / 60),
                    strprintf("%s%s", peer.is_bip152_hb_to ? "." : " ", peer.is_bip152_hb_from ? "*" : " "),
                    m_max_addr_processed_length, // variable spacing
                    peer.addr_processed ? ToString(peer.addr_processed) : "",
                    m_max_addr_rate_limited_length, // variable spacing
                    peer.addr_rate_limited ? ToString(peer.addr_rate_limited) : "",
                    m_max_age_length, // variable spacing
                    peer.age,
                    m_is_asmap_on ? 7 : 0, // variable spacing
                    m_is_asmap_on && peer.mapped_as != 0 ? ToString(peer.mapped_as) : "",
                    m_max_id_length, // variable spacing
                    peer.id,
                    IsAddressSelected() ? m_max_addr_length : 0, // variable spacing
                    IsAddressSelected() ? peer.addr : "",
                    IsVersionSelected() && version != "0" ? version : "");
            }
            result += strprintf("                     ms     ms  sec  sec  min  min                %*s\n\n", m_max_age_length, "min");
        }

        // Report peer connection totals by type.
        result += "        ipv4    ipv6   onion";
        const bool any_i2p_peers = m_counts.at(2).at(3); // false if total i2p peers count is 0, otherwise true
        if (any_i2p_peers) result += "     i2p";
        result += "   total   block";
        if (m_manual_peers_count) result += "  manual";
        const std::array rows{"in", "out", "total"};
        for (uint8_t i = 0; i < 3; ++i) {
            result += strprintf("\n%-5s  %5i   %5i   %5i", rows.at(i), m_counts.at(i).at(0), m_counts.at(i).at(1), m_counts.at(i).at(2)); // ipv4/ipv6/onion peers counts
            if (any_i2p_peers) result += strprintf("   %5i", m_counts.at(i).at(3)); // i2p peers count
            result += strprintf("   %5i", m_counts.at(i).at(m_networks.size())); // total peers count
            if (i == 1) { // the outbound row has two extra columns for block relay and manual peer counts
                result += strprintf("   %5i", m_block_relay_peers_count);
                if (m_manual_peers_count) result += strprintf("   %5i", m_manual_peers_count);
            }
        }

        // Report local addresses, ports, and scores.
        result += "\n\nLocal addresses";
        const std::vector<UniValue>& local_addrs{networkinfo["localaddresses"].getValues()};
        if (local_addrs.empty()) {
            result += ": n/a\n";
        } else {
            size_t max_addr_size{0};
            for (const UniValue& addr : local_addrs) {
                max_addr_size = std::max(addr["address"].get_str().length() + 1, max_addr_size);
            }
            for (const UniValue& addr : local_addrs) {
                result += strprintf("\n%-*s    port %6i    score %6i", max_addr_size, addr["address"].get_str(), addr["port"].get_int(), addr["score"].get_int());
            }
        }

        return JSONRPCReplyObj(UniValue{result}, NullUniValue, 1);
    }

    const std::string m_help_doc{
        "-netinfo level|\"help\" \n\n"
        "Returns a network peer connections dashboard with information from the remote server.\n"
        "This human-readable interface will change regularly and is not intended to be a stable API.\n"
        "Under the hood, -netinfo fetches the data by calling getpeerinfo and getnetworkinfo.\n"
        + strprintf("An optional integer argument from 0 to %d can be passed for different peers listings; %d to 255 are parsed as %d.\n", MAX_DETAIL_LEVEL, MAX_DETAIL_LEVEL, MAX_DETAIL_LEVEL) +
        "Pass \"help\" to see this detailed help documentation.\n"
        "If more than one argument is passed, only the first one is read and parsed.\n"
        "Suggestion: use with the Linux watch(1) command for a live dashboard; see example below.\n\n"
        "Arguments:\n"
        + strprintf("1. level (integer 0-%d, optional)  Specify the info level of the peers dashboard (default 0):\n", MAX_DETAIL_LEVEL) +
        "                                  0 - Connection counts and local addresses\n"
        "                                  1 - Like 0 but with a peers listing (without address or version columns)\n"
        "                                  2 - Like 1 but with an address column\n"
        "                                  3 - Like 1 but with a version column\n"
        "                                  4 - Like 1 but with both address and version columns\n"
        "2. help (string \"help\", optional) Print this help documentation instead of the dashboard.\n\n"
        "Result:\n\n"
        + strprintf("* The peers listing in levels 1-%d displays all of the peers sorted by direction and minimum ping time:\n\n", MAX_DETAIL_LEVEL) +
        "  Column   Description\n"
        "  ------   -----------\n"
        "  <->      Direction\n"
        "           \"in\"  - inbound connections are those initiated by the peer\n"
        "           \"out\" - outbound connections are those initiated by us\n"
        "  type     Type of peer connection\n"
        "           \"full\"   - full relay, the default\n"
        "           \"block\"  - block relay; like full relay but does not relay transactions or addresses\n"
        "           \"manual\" - peer we manually added using RPC addnode or the -addnode/-connect config options\n"
        "           \"feeler\" - short-lived connection for testing addresses\n"
        "           \"addr\"   - address fetch; short-lived connection for requesting addresses\n"
        "  net      Network the peer connected through (\"ipv4\", \"ipv6\", \"onion\", \"i2p\", or \"cjdns\")\n"
        "  mping    Minimum observed ping time, in milliseconds (ms)\n"
        "  ping     Last observed ping time, in milliseconds (ms)\n"
        "  send     Time since last message sent to the peer, in seconds\n"
        "  recv     Time since last message received from the peer, in seconds\n"
        "  txn      Time since last novel transaction received from the peer and accepted into our mempool, in minutes\n"
        "  blk      Time since last novel block passing initial validity checks received from the peer, in minutes\n"
        "  hb       High-bandwidth BIP152 compact block relay\n"
        "           \".\" (to)   - we selected the peer as a high-bandwidth peer\n"
        "           \"*\" (from) - the peer selected us as a high-bandwidth peer\n"
        "  addrp    Total number of addresses processed, excluding those dropped due to rate limiting\n"
        "  addrl    Total number of addresses dropped due to rate limiting\n"
        "  age      Duration of connection to the peer, in minutes\n"
        "  asmap    Mapped AS (Autonomous System) number in the BGP route to the peer, used for diversifying\n"
        "           peer selection (only displayed if the -asmap config option is set)\n"
        "  id       Peer index, in increasing order of peer connections since node startup\n"
        "  address  IP address and port of the peer\n"
        "  version  Peer version and subversion concatenated, e.g. \"70016/Satoshi:21.0.0/\"\n\n"
        "* The connection counts table displays the number of peers by direction, network, and the totals\n"
        "  for each, as well as two special outbound columns for block relay peers and manual peers.\n\n"
        "* The local addresses table lists each local address broadcast by the node, the port, and the score.\n\n"
        "Examples:\n\n"
        "Connection counts and local addresses only\n"
        "> bitcoin-cli -netinfo\n\n"
        "Compact peers listing\n"
        "> bitcoin-cli -netinfo 1\n\n"
        "Full dashboard\n"
        + strprintf("> bitcoin-cli -netinfo %d\n\n", MAX_DETAIL_LEVEL) +
        "Full live dashboard, adjust --interval or --no-title as needed (Linux)\n"
        + strprintf("> watch --interval 1 --no-title bitcoin-cli -netinfo %d\n\n", MAX_DETAIL_LEVEL) +
        "See this help\n"
        "> bitcoin-cli -netinfo help\n"};
};

/** Process RPC generatetoaddress request. */
class GenerateToAddressRequestHandler : public BaseRequestHandler
{
public:
    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        address_str = args.at(1);
        UniValue params{RPCConvertValues("generatetoaddress", args)};
        return JSONRPCRequestObj("generatetoaddress", params, 1);
    }

    UniValue ProcessReply(const UniValue &reply) override
    {
        UniValue result(UniValue::VOBJ);
        result.pushKV("address", address_str);
        result.pushKV("blocks", reply.get_obj()["result"]);
        return JSONRPCReplyObj(result, NullUniValue, 1);
    }
protected:
    std::string address_str;
};

/** Process default single requests */
class DefaultRequestHandler: public BaseRequestHandler {
public:
    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        UniValue params;
        if(gArgs.GetBoolArg("-named", DEFAULT_NAMED)) {
            params = RPCConvertNamedValues(method, args);
        } else {
            params = RPCConvertValues(method, args);
        }
        return JSONRPCRequestObj(method, params, 1);
    }

    UniValue ProcessReply(const UniValue &reply) override
    {
        return reply.get_obj();
    }
};

static UniValue CallRPC(BaseRequestHandler* rh, const std::string& strMethod, const std::vector<std::string>& args, const std::optional<std::string>& rpcwallet = {})
{
    std::string host;
    // In preference order, we choose the following for the port:
    //     1. -rpcport
    //     2. port in -rpcconnect (ie following : in ipv4 or ]: in ipv6)
    //     3. default port for chain
    uint16_t port{BaseParams().RPCPort()};
    SplitHostPort(gArgs.GetArg("-rpcconnect", DEFAULT_RPCCONNECT), port, host);
    port = static_cast<uint16_t>(gArgs.GetArg("-rpcport", port));

    // Obtain event base
    raii_event_base base = obtain_event_base();

    // Synchronously look up hostname
    raii_evhttp_connection evcon = obtain_evhttp_connection_base(base.get(), host, port);

    // Set connection timeout
    {
        const int timeout = gArgs.GetArg("-rpcclienttimeout", DEFAULT_HTTP_CLIENT_TIMEOUT);
        if (timeout > 0) {
            evhttp_connection_set_timeout(evcon.get(), timeout);
        } else {
            // Indefinite request timeouts are not possible in libevent-http, so we
            // set the timeout to a very long time period instead.

            constexpr int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
            evhttp_connection_set_timeout(evcon.get(), 5 * YEAR_IN_SECONDS);
        }
    }

    HTTPReply response;
    raii_evhttp_request req = obtain_evhttp_request(http_request_done, (void*)&response);
    if (req == nullptr)
        throw std::runtime_error("create http request failed");
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    evhttp_request_set_error_cb(req.get(), http_error_cb);
#endif

    // Get credentials
    std::string strRPCUserColonPass;
    bool failedToGetAuthCookie = false;
    if (gArgs.GetArg("-rpcpassword", "") == "") {
        // Try fall back to cookie-based authentication if no password is provided
        if (!GetAuthCookie(&strRPCUserColonPass)) {
            failedToGetAuthCookie = true;
        }
    } else {
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }

    struct evkeyvalq* output_headers = evhttp_request_get_output_headers(req.get());
    assert(output_headers);
    evhttp_add_header(output_headers, "Host", host.c_str());
    evhttp_add_header(output_headers, "Connection", "close");
    evhttp_add_header(output_headers, "Content-Type", "application/json");
    evhttp_add_header(output_headers, "Authorization", (std::string("Basic ") + EncodeBase64(strRPCUserColonPass)).c_str());

    // Attach request data
    std::string strRequest = rh->PrepareRequest(strMethod, args).write() + "\n";
    struct evbuffer* output_buffer = evhttp_request_get_output_buffer(req.get());
    assert(output_buffer);
    evbuffer_add(output_buffer, strRequest.data(), strRequest.size());

    // check if we should use a special wallet endpoint
    std::string endpoint = "/";
    if (rpcwallet) {
        char* encodedURI = evhttp_uriencode(rpcwallet->data(), rpcwallet->size(), false);
        if (encodedURI) {
            endpoint = "/wallet/" + std::string(encodedURI);
            free(encodedURI);
        } else {
            throw CConnectionFailed("uri-encode failed");
        }
    }
    int r = evhttp_make_request(evcon.get(), req.get(), EVHTTP_REQ_POST, endpoint.c_str());
    req.release(); // ownership moved to evcon in above call
    if (r != 0) {
        throw CConnectionFailed("send http request failed");
    }

    event_base_dispatch(base.get());

    if (response.status == 0) {
        std::string responseErrorMessage;
        if (response.error != -1) {
            responseErrorMessage = strprintf(" (error code %d - \"%s\")", response.error, http_errorstring(response.error));
        }
        throw CConnectionFailed(strprintf("Could not connect to the server %s:%d%s\n\nMake sure the bitcoind server is running and that you are connecting to the correct RPC port.", host, port, responseErrorMessage));
    } else if (response.status == HTTP_UNAUTHORIZED) {
        if (failedToGetAuthCookie) {
            throw std::runtime_error(strprintf(
                "Could not locate RPC credentials. No authentication cookie could be found, and RPC password is not set.  See -rpcpassword and -stdinrpcpass.  Configuration file: (%s)",
                GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME)).string()));
        } else {
            throw std::runtime_error("Authorization failed: Incorrect rpcuser or rpcpassword");
        }
    } else if (response.status == HTTP_SERVICE_UNAVAILABLE) {
        throw std::runtime_error(strprintf("Server response: %s", response.body));
    } else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST && response.status != HTTP_NOT_FOUND && response.status != HTTP_INTERNAL_SERVER_ERROR)
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.status));
    else if (response.body.empty())
        throw std::runtime_error("no response from server");

    // Parse reply
    UniValue valReply(UniValue::VSTR);
    if (!valReply.read(response.body))
        throw std::runtime_error("couldn't parse reply from server");
    const UniValue reply = rh->ProcessReply(valReply);
    if (reply.empty())
        throw std::runtime_error("expected reply to have result, error and id properties");

    return reply;
}

/**
 * ConnectAndCallRPC wraps CallRPC with -rpcwait and an exception handler.
 *
 * @param[in] rh         Pointer to RequestHandler.
 * @param[in] strMethod  Reference to const string method to forward to CallRPC.
 * @param[in] rpcwallet  Reference to const optional string wallet name to forward to CallRPC.
 * @returns the RPC response as a UniValue object.
 * @throws a CConnectionFailed std::runtime_error if connection failed or RPC server still in warmup.
 */
static UniValue ConnectAndCallRPC(BaseRequestHandler* rh, const std::string& strMethod, const std::vector<std::string>& args, const std::optional<std::string>& rpcwallet = {})
{
    UniValue response(UniValue::VOBJ);
    // Execute and handle connection failures with -rpcwait.
    const bool fWait = gArgs.GetBoolArg("-rpcwait", false);
    const int timeout = gArgs.GetArg("-rpcwaittimeout", DEFAULT_WAIT_CLIENT_TIMEOUT);
    const auto deadline{GetTime<std::chrono::microseconds>() + 1s * timeout};

    do {
        try {
            response = CallRPC(rh, strMethod, args, rpcwallet);
            if (fWait) {
                const UniValue& error = find_value(response, "error");
                if (!error.isNull() && error["code"].get_int() == RPC_IN_WARMUP) {
                    throw CConnectionFailed("server in warmup");
                }
            }
            break; // Connection succeeded, no need to retry.
        } catch (const CConnectionFailed& e) {
            const auto now{GetTime<std::chrono::microseconds>()};
            if (fWait && (timeout <= 0 || now < deadline)) {
                UninterruptibleSleep(1s);
            } else {
                throw CConnectionFailed(strprintf("timeout on transient error: %s", e.what()));
            }
        }
    } while (fWait);
    return response;
}

/** Parse UniValue result to update the message to print to std::cout. */
static void ParseResult(const UniValue& result, std::string& strPrint)
{
    if (result.isNull()) return;
    strPrint = result.isStr() ? result.get_str() : result.write(2);
}

/** Parse UniValue error to update the message to print to std::cerr and the code to return. */
static void ParseError(const UniValue& error, std::string& strPrint, int& nRet)
{
    if (error.isObject()) {
        const UniValue& err_code = find_value(error, "code");
        const UniValue& err_msg = find_value(error, "message");
        if (!err_code.isNull()) {
            strPrint = "error code: " + err_code.getValStr() + "\n";
        }
        if (err_msg.isStr()) {
            strPrint += ("error message:\n" + err_msg.get_str());
        }
        if (err_code.isNum() && err_code.get_int() == RPC_WALLET_NOT_SPECIFIED) {
            strPrint += "\nTry adding \"-rpcwallet=<filename>\" option to bitcoin-cli command line.";
        }
    } else {
        strPrint = "error: " + error.write();
    }
    nRet = abs(error["code"].get_int());
}

/**
 * GetWalletBalances calls listwallets; if more than one wallet is loaded, it then
 * fetches mine.trusted balances for each loaded wallet and pushes them to `result`.
 *
 * @param result  Reference to UniValue object the wallet names and balances are pushed to.
 */
static void GetWalletBalances(UniValue& result)
{
    DefaultRequestHandler rh;
    const UniValue listwallets = ConnectAndCallRPC(&rh, "listwallets", /* args=*/{});
    if (!find_value(listwallets, "error").isNull()) return;
    const UniValue& wallets = find_value(listwallets, "result");
    if (wallets.size() <= 1) return;

    UniValue balances(UniValue::VOBJ);
    for (const UniValue& wallet : wallets.getValues()) {
        const std::string wallet_name = wallet.get_str();
        const UniValue getbalances = ConnectAndCallRPC(&rh, "getbalances", /* args=*/{}, wallet_name);
        const UniValue& balance = find_value(getbalances, "result")["mine"]["trusted"];
        balances.pushKV(wallet_name, balance);
    }
    result.pushKV("balances", balances);
}

/**
 * ParseGetInfoResult takes in -getinfo result in UniValue object and parses it
 * into a user friendly UniValue string to be printed on the console.
 * @param[out] result  Reference to UniValue result containing the -getinfo output.
 */
static void ParseGetInfoResult(UniValue& result)
{
    if (!find_value(result, "error").isNull()) return;

    std::string RESET, GREEN, BLUE, YELLOW, MAGENTA, CYAN;
    bool should_colorize = false;

#ifndef WIN32
    if (isatty(fileno(stdout))) {
        // By default, only print colored text if OS is not WIN32 and stdout is connected to a terminal.
        should_colorize = true;
    }
#endif

    if (gArgs.IsArgSet("-color")) {
        const std::string color{gArgs.GetArg("-color", DEFAULT_COLOR_SETTING)};
        if (color == "always") {
            should_colorize = true;
        } else if (color == "never") {
            should_colorize = false;
        } else if (color != "auto") {
            throw std::runtime_error("Invalid value for -color option. Valid values: always, auto, never.");
        }
    }

    if (should_colorize) {
        RESET = "\x1B[0m";
        GREEN = "\x1B[32m";
        BLUE = "\x1B[34m";
        YELLOW = "\x1B[33m";
        MAGENTA = "\x1B[35m";
        CYAN = "\x1B[36m";
    }

    std::string result_string = strprintf("%sChain: %s%s\n", BLUE, result["chain"].getValStr(), RESET);
    result_string += strprintf("Blocks: %s\n", result["blocks"].getValStr());
    result_string += strprintf("Headers: %s\n", result["headers"].getValStr());
    result_string += strprintf("Verification progress: %.4f%%\n", result["verificationprogress"].get_real() * 100);
    result_string += strprintf("Difficulty: %s\n\n", result["difficulty"].getValStr());

    result_string += strprintf(
        "%sNetwork: in %s, out %s, total %s%s\n",
        GREEN,
        result["connections"]["in"].getValStr(),
        result["connections"]["out"].getValStr(),
        result["connections"]["total"].getValStr(),
        RESET);
    result_string += strprintf("Version: %s\n", result["version"].getValStr());
    result_string += strprintf("Time offset (s): %s\n", result["timeoffset"].getValStr());
    const std::string proxy = result["proxy"].getValStr();
    result_string += strprintf("Proxy: %s\n", proxy.empty() ? "N/A" : proxy);
    result_string += strprintf("Min tx relay fee rate (%s/kvB): %s\n\n", CURRENCY_UNIT, result["relayfee"].getValStr());

    if (!result["has_wallet"].isNull()) {
        const std::string walletname = result["walletname"].getValStr();
        result_string += strprintf("%sWallet: %s%s\n", MAGENTA, walletname.empty() ? "\"\"" : walletname, RESET);

        result_string += strprintf("Keypool size: %s\n", result["keypoolsize"].getValStr());
        if (!result["unlocked_until"].isNull()) {
            result_string += strprintf("Unlocked until: %s\n", result["unlocked_until"].getValStr());
        }
        result_string += strprintf("Transaction fee rate (-paytxfee) (%s/kvB): %s\n\n", CURRENCY_UNIT, result["paytxfee"].getValStr());
    }
    if (!result["balance"].isNull()) {
        result_string += strprintf("%sBalance:%s %s\n\n", CYAN, RESET, result["balance"].getValStr());
    }

    if (!result["balances"].isNull()) {
        result_string += strprintf("%sBalances%s\n", CYAN, RESET);

        size_t max_balance_length{10};

        for (const std::string& wallet : result["balances"].getKeys()) {
            max_balance_length = std::max(result["balances"][wallet].getValStr().length(), max_balance_length);
        }

        for (const std::string& wallet : result["balances"].getKeys()) {
            result_string += strprintf("%*s %s\n",
                                       max_balance_length,
                                       result["balances"][wallet].getValStr(),
                                       wallet.empty() ? "\"\"" : wallet);
        }
        result_string += "\n";
    }

    result_string += strprintf("%sWarnings:%s %s", YELLOW, RESET, result["warnings"].getValStr());
    result.setStr(result_string);
}

/**
 * Call RPC getnewaddress.
 * @returns getnewaddress response as a UniValue object.
 */
static UniValue GetNewAddress()
{
    std::optional<std::string> wallet_name{};
    if (gArgs.IsArgSet("-rpcwallet")) wallet_name = gArgs.GetArg("-rpcwallet", "");
    DefaultRequestHandler rh;
    return ConnectAndCallRPC(&rh, "getnewaddress", /* args=*/{}, wallet_name);
}

/**
 * Check bounds and set up args for RPC generatetoaddress params: nblocks, address, maxtries.
 * @param[in] address  Reference to const string address to insert into the args.
 * @param     args     Reference to vector of string args to modify.
 */
static void SetGenerateToAddressArgs(const std::string& address, std::vector<std::string>& args)
{
    if (args.size() > 2) throw std::runtime_error("too many arguments (maximum 2 for nblocks and maxtries)");
    if (args.size() == 0) {
        args.emplace_back(DEFAULT_NBLOCKS);
    } else if (args.at(0) == "0") {
        throw std::runtime_error("the first argument (number of blocks to generate, default: " + DEFAULT_NBLOCKS + ") must be an integer value greater than zero");
    }
    args.emplace(args.begin() + 1, address);
}

static int CommandLineRPC(int argc, char *argv[])
{
    std::string strPrint;
    int nRet = 0;
    try {
        // Skip switches
        while (argc > 1 && IsSwitchChar(argv[1][0])) {
            argc--;
            argv++;
        }
        std::string rpcPass;
        if (gArgs.GetBoolArg("-stdinrpcpass", false)) {
            NO_STDIN_ECHO();
            if (!StdinReady()) {
                fputs("RPC password> ", stderr);
                fflush(stderr);
            }
            if (!std::getline(std::cin, rpcPass)) {
                throw std::runtime_error("-stdinrpcpass specified but failed to read from standard input");
            }
            if (StdinTerminal()) {
                fputc('\n', stdout);
            }
            gArgs.ForceSetArg("-rpcpassword", rpcPass);
        }
        std::vector<std::string> args = std::vector<std::string>(&argv[1], &argv[argc]);
        if (gArgs.GetBoolArg("-stdinwalletpassphrase", false)) {
            NO_STDIN_ECHO();
            std::string walletPass;
            if (args.size() < 1 || args[0].substr(0, 16) != "walletpassphrase") {
                throw std::runtime_error("-stdinwalletpassphrase is only applicable for walletpassphrase(change)");
            }
            if (!StdinReady()) {
                fputs("Wallet passphrase> ", stderr);
                fflush(stderr);
            }
            if (!std::getline(std::cin, walletPass)) {
                throw std::runtime_error("-stdinwalletpassphrase specified but failed to read from standard input");
            }
            if (StdinTerminal()) {
                fputc('\n', stdout);
            }
            args.insert(args.begin() + 1, walletPass);
        }
        if (gArgs.GetBoolArg("-stdin", false)) {
            // Read one arg per line from stdin and append
            std::string line;
            while (std::getline(std::cin, line)) {
                args.push_back(line);
            }
            if (StdinTerminal()) {
                fputc('\n', stdout);
            }
        }
        std::unique_ptr<BaseRequestHandler> rh;
        std::string method;
        if (gArgs.IsArgSet("-getinfo")) {
            rh.reset(new GetinfoRequestHandler());
        } else if (gArgs.GetBoolArg("-netinfo", false)) {
            if (!args.empty() && args.at(0) == "help") {
                tfm::format(std::cout, "%s\n", NetinfoRequestHandler().m_help_doc);
                return 0;
            }
            rh.reset(new NetinfoRequestHandler());
        } else if (gArgs.GetBoolArg("-generate", false)) {
            const UniValue getnewaddress{GetNewAddress()};
            const UniValue& error{find_value(getnewaddress, "error")};
            if (error.isNull()) {
                SetGenerateToAddressArgs(find_value(getnewaddress, "result").get_str(), args);
                rh.reset(new GenerateToAddressRequestHandler());
            } else {
                ParseError(error, strPrint, nRet);
            }
        } else if (gArgs.GetBoolArg("-addrinfo", false)) {
            rh.reset(new AddrinfoRequestHandler());
        } else {
            rh.reset(new DefaultRequestHandler());
            if (args.size() < 1) {
                throw std::runtime_error("too few parameters (need at least command)");
            }
            method = args[0];
            args.erase(args.begin()); // Remove trailing method name from arguments vector
        }
        if (nRet == 0) {
            // Perform RPC call
            std::optional<std::string> wallet_name{};
            if (gArgs.IsArgSet("-rpcwallet")) wallet_name = gArgs.GetArg("-rpcwallet", "");
            const UniValue reply = ConnectAndCallRPC(rh.get(), method, args, wallet_name);

            // Parse reply
            UniValue result = find_value(reply, "result");
            const UniValue& error = find_value(reply, "error");
            if (error.isNull()) {
                if (gArgs.GetBoolArg("-getinfo", false)) {
                    if (!gArgs.IsArgSet("-rpcwallet")) {
                        GetWalletBalances(result); // fetch multiwallet balances and append to result
                    }
                    ParseGetInfoResult(result);
                }

                ParseResult(result, strPrint);
            } else {
                ParseError(error, strPrint, nRet);
            }
        }
    } catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
        throw;
    }

    if (strPrint != "") {
        tfm::format(nRet == 0 ? std::cout : std::cerr, "%s\n", strPrint);
    }
    return nRet;
}

#ifdef WIN32
// Export main() and ensure working ASLR on Windows.
// Exporting a symbol will prevent the linker from stripping
// the .reloc section from the binary, which is a requirement
// for ASLR. This is a temporary workaround until a fixed
// version of binutils is used for releases.
__declspec(dllexport) int main(int argc, char* argv[])
{
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#else
int main(int argc, char* argv[])
{
#endif
    SetupEnvironment();
    if (!SetupNetworking()) {
        tfm::format(std::cerr, "Error: Initializing networking failed\n");
        return EXIT_FAILURE;
    }
    event_set_log_callback(&libevent_log_cb);

    try {
        int ret = AppInitRPC(argc, argv);
        if (ret != CONTINUE_EXECUTION)
            return ret;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRPC()");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    try {
        ret = CommandLineRPC(argc, argv);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
    }
    return ret;
}
