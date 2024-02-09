// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/url.h>
#include <compat/compat.h>
#include <compat/stdin.h>
#include <logging.h>
#include <policy/feerate.h>
#include <rpc/client.h>
#include <rpc/mining.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/exception.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>

#ifndef WIN32
#include <unistd.h>
#endif

#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <support/events.h>

// The server returns time values from a mockable system clock, but it is not
// trivial to get the mocked time from the server, nor is it needed for now, so
// just use a plain system_clock.
using CliClock = std::chrono::system_clock;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = urlDecode;

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT = 900;
static constexpr int DEFAULT_WAIT_CLIENT_TIMEOUT = 0;
static const bool DEFAULT_NAMED = false;
static const int CONTINUE_EXECUTION = -1;
static constexpr int8_t UNKNOWN_NETWORK{-1};
// See GetNetworkName() in netbase.cpp
static constexpr std::array NETWORKS{"not_publicly_routable", "ipv4", "ipv6", "onion", "i2p", "cjdns", "internal"};
static constexpr std::array NETWORK_SHORT_NAMES{"npr", "ipv4", "ipv6", "onion", "i2p", "cjdns", "int"};
static constexpr std::array UNREACHABLE_NETWORK_IDS{/*not_publicly_routable*/ 0, /*internal*/ 6};
static const char* const DEFAULT_LOGFILE = "staker.log";

/** Default number of blocks to generate for RPC generatetoaddress. */
static const std::string DEFAULT_NBLOCKS = "1";

/** Default -color setting. */
static const std::string DEFAULT_COLOR_SETTING{"auto"};

static void SetupCliArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    const auto defaultBaseParams = CreateBaseChainParams(ChainType::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(ChainType::TESTNET);
    const auto signetBaseParams = CreateBaseChainParams(ChainType::SIGNET);
    const auto regtestBaseParams = CreateBaseChainParams(ChainType::REGTEST);
    const auto blsctRegtestBaseParams = CreateBaseChainParams(ChainType::BLSCTREGTEST);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-conf=<file>", strprintf("Specify configuration file. Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    SetupChainParamsBaseOptions(argsman);
    argsman.AddArg("-debuglogfile=<file|false>", strprintf("Specify log file. Set to false to disable logging to file (default: %s)", DEFAULT_LOGFILE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-color=<when>", strprintf("Color setting for CLI output (default: %s). Valid values: always, auto (add color codes when standard output is connected to a terminal and OS is not WIN32), never.", DEFAULT_COLOR_SETTING), ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-printtoconsole", "Prints debug to stdout", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcclienttimeout=<n>", strprintf("Timeout in seconds during HTTP requests, or 0 for no timeout. (default: %d)", DEFAULT_HTTP_CLIENT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcconnect=<ip>", strprintf("Send commands to node running on <ip> (default: %s)", DEFAULT_RPCCONNECT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcport=<port>", strprintf("Connect to JSON-RPC on <port> (default: %u, testnet: %u, signet: %u, regtest: %u, blsctregtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort(), blsctRegtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcwait", "Wait for RPC server to start", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcwaittimeout=<n>", strprintf("Timeout in seconds to wait for the RPC server to start, or 0 for no timeout. (default: %d)", DEFAULT_WAIT_CLIENT_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-stdinrpcpass", "Read RPC password from standard input as a single line. When combined with -stdin, the first line from standard input is used for the RPC password. When combined with -stdinwalletpassphrase, -stdinrpcpass consumes the first line, and -stdinwalletpassphrase consumes the second.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-stdinwalletpassphrase", "Read wallet passphrase from standard input as a single line. When combined with -stdin, the first line from standard input is used for the wallet passphrase.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-wallet=<wallet-name>", "Specify wallet name", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-walletpassphrase=<password>", "Specify the password to unlock the wallet", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
}

/** libevent event log callback */
static void libevent_log_cb(int severity, const char* msg)
{
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
    explicit inline CConnectionFailed(const std::string& msg) : std::runtime_error(msg)
    {
    }
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

        if (gArgs.IsArgSet("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                        "Usage:  bitcoin-cli [options] <command> [params]  Send command to " PACKAGE_NAME "\n"
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
    if (!CheckDataDirOption(gArgs)) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", ""));
        return EXIT_FAILURE;
    }
    if (!gArgs.ReadConfigFiles(error, true)) {
        tfm::format(std::cerr, "Error reading configuration file: %s\n", error);
        return EXIT_FAILURE;
    }
    // Check for chain settings (BaseParams() calls are only valid after this clause)
    try {
        SelectBaseParams(gArgs.GetChainType());
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return CONTINUE_EXECUTION;
}


/** Reply structure for request_done to fill in */
struct HTTPReply {
    HTTPReply() = default;

    int status{0};
    int error{-1};
    std::string body;
};

static std::string http_errorstring(int code)
{
    switch (code) {
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
    default:
        return "unknown";
    }
}

static void http_request_done(struct evhttp_request* req, void* ctx)
{
    HTTPReply* reply = static_cast<HTTPReply*>(ctx);

    if (req == nullptr) {
        /* If req is nullptr, it means an error occurred while connecting: the
         * error code will have been passed to http_error_cb.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer* buf = evhttp_request_get_input_buffer(req);
    if (buf) {
        size_t size = evbuffer_get_length(buf);
        const char* data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

static void http_error_cb(enum evhttp_request_error err, void* ctx)
{
    HTTPReply* reply = static_cast<HTTPReply*>(ctx);
    reply->error = err;
}

/** Class that handles the conversion from a command-line to a JSON-RPC request,
 * as well as converting back to a JSON object that can be shown as result.
 */
class BaseRequestHandler
{
public:
    virtual ~BaseRequestHandler() = default;
    virtual UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) = 0;
    virtual UniValue ProcessReply(const UniValue& batch_in) = 0;
};

/** Process default single requests */
class DefaultRequestHandler : public BaseRequestHandler
{
public:
    UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) override
    {
        UniValue params;
        params = RPCConvertValues(method, args);
        return JSONRPCRequestObj(method, params, 1);
    }

    UniValue ProcessReply(const UniValue& reply) override
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
    port = static_cast<uint16_t>(gArgs.GetIntArg("-rpcport", port));

    // Obtain event base
    raii_event_base base = obtain_event_base();

    // Synchronously look up hostname
    raii_evhttp_connection evcon = obtain_evhttp_connection_base(base.get(), host, port);

    // Set connection timeout
    {
        const int timeout = gArgs.GetIntArg("-rpcclienttimeout", DEFAULT_HTTP_CLIENT_TIMEOUT);
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
    if (req == nullptr) {
        throw std::runtime_error("create http request failed");
    }

    evhttp_request_set_error_cb(req.get(), http_error_cb);

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
                fs::PathToString(gArgs.GetConfigFilePath())));
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
    UniValue reply = rh->ProcessReply(valReply);
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
    const int timeout = gArgs.GetIntArg("-rpcwaittimeout", DEFAULT_WAIT_CLIENT_TIMEOUT);
    const auto deadline{std::chrono::steady_clock::now() + 1s * timeout};

    do {
        try {
            response = CallRPC(rh, strMethod, args, rpcwallet);
            if (fWait) {
                const UniValue& error = response.find_value("error");
                if (!error.isNull() && error["code"].getInt<int>() == RPC_IN_WARMUP) {
                    throw CConnectionFailed("server in warmup");
                }
            }
            break; // Connection succeeded, no need to retry.
        } catch (const CConnectionFailed& e) {
            if (fWait && (timeout <= 0 || std::chrono::steady_clock::now() < deadline)) {
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
        const UniValue& err_code = error.find_value("code");
        const UniValue& err_msg = error.find_value("message");
        if (!err_code.isNull()) {
            strPrint = "error code: " + err_code.getValStr() + "\n";
        }
        if (err_msg.isStr()) {
            strPrint += ("error message:\n" + err_msg.get_str());
        }
        if (err_code.isNum() && (err_code.getInt<int>() == RPC_WALLET_NOT_SPECIFIED || err_code.getInt<int>() == RPC_WALLET_NOT_FOUND)) {
            strPrint += "\nTry adding \"-wallet=<filename>\" option to navcoin-staker command line.";
        }
        if (err_code.isNum() && err_code.getInt<int>() == RPC_WALLET_PASSPHRASE_INCORRECT) {
            strPrint += "\nThe wallet passphrase is incorrect.";
        }
    } else {
        strPrint = "error: " + error.write();
    }
    nRet = abs(error["code"].getInt<int>());
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
    if (!listwallets.find_value("error").isNull()) return;
    const UniValue& wallets = listwallets.find_value("result");
    if (wallets.size() <= 1) return;

    UniValue balances(UniValue::VOBJ);
    for (const UniValue& wallet : wallets.getValues()) {
        const std::string& wallet_name = wallet.get_str();
        const UniValue getbalances = ConnectAndCallRPC(&rh, "getbalances", /* args=*/{}, wallet_name);
        const UniValue& balance = getbalances.find_value("result")["mine"]["trusted"];
        balances.pushKV(wallet_name, balance);
    }
    result.pushKV("balances", balances);
}

/**
 * GetProgressBar constructs a progress bar with 5% intervals.
 *
 * @param[in]   progress      The proportion of the progress bar to be filled between 0 and 1.
 * @param[out]  progress_bar  String representation of the progress bar.
 */
static void GetProgressBar(double progress, std::string& progress_bar)
{
    if (progress < 0 || progress > 1) return;

    static constexpr double INCREMENT{0.05};
    static const std::string COMPLETE_BAR{"\u2592"};
    static const std::string INCOMPLETE_BAR{"\u2591"};

    for (int i = 0; i < progress / INCREMENT; ++i) {
        progress_bar += COMPLETE_BAR;
    }

    for (int i = 0; i < (1 - progress) / INCREMENT; ++i) {
        progress_bar += INCOMPLETE_BAR;
    }
}

static std::string rpcPass = "";
static std::string walletName = "";
static std::string walletPassphrase = "";
static bool mustUnlockWallet = false;

static int CommandLineRPC(int argc, char* argv[])
{
    std::string strPrint;
    int nRet = 0;
    try {
        // Skip switches
        while (argc > 1 && IsSwitchChar(argv[1][0])) {
            argc--;
            argv++;
        }
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

        rh.reset(new DefaultRequestHandler());
        if (args.size() < 1) {
            throw std::runtime_error("too few parameters (need at least command)");
        }
        method = args[0];
        args.erase(args.begin()); // Remove trailing method name from arguments vector

        if (nRet == 0) {
            // Perform RPC call
            std::optional<std::string> wallet_name{};
            if (gArgs.IsArgSet("-wallet")) wallet_name = gArgs.GetArg("-wallet", "");
            const UniValue reply = ConnectAndCallRPC(rh.get(), method, args, wallet_name);

            // Parse reply
            UniValue result = reply.find_value("result");
            const UniValue& error = reply.find_value("error");
            if (error.isNull()) {
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

void Setup()
{
    LogInstance().m_print_to_file = !gArgs.IsArgNegated("-debuglogfile");
    LogInstance().m_file_path = AbsPathForConfigVal(gArgs, gArgs.GetPathArg("-debuglogfile", DEFAULT_LOGFILE));
    LogInstance().m_print_to_console = gArgs.GetBoolArg("-printtoconsole", true);

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
    } else if (gArgs.IsArgSet("-rpcpass"))
        rpcPass = gArgs.GetArg("-rpcpass", {});


    if (gArgs.GetBoolArg("-stdinrpcwalletpassphrase", false)) {
        NO_STDIN_ECHO();
        if (!StdinReady()) {
            fputs("Wallet password> ", stderr);
            fflush(stderr);
        }
        if (!std::getline(std::cin, walletPassphrase)) {
            throw std::runtime_error("-stdinrpcwalletpassphrase specified but failed to read from standard input");
        }
        if (StdinTerminal()) {
            fputc('\n', stdout);
        }
        gArgs.ForceSetArg("-rpcwalletpassphrase", walletPassphrase);
    } else if (gArgs.IsArgSet("-rpcwalletpassphrase"))
        walletPassphrase = gArgs.GetArg("-rpcwalletpassphrase", {});


    if (gArgs.IsArgSet("-wallet")) walletName = gArgs.GetArg("-wallet", {});

    if (walletName == "")
        throw std::runtime_error(strprintf("Please specify a wallet name with -wallet=<name>"));

    if (!LogInstance().StartLogging()) {
        throw std::runtime_error(strprintf("Could not open debug log file %s",
                                           fs::PathToString(LogInstance().m_file_path)));
    }
}

bool TestSetup()
{
    std::unique_ptr<BaseRequestHandler> rh;

    rh.reset(new DefaultRequestHandler());

    try {
        UniValue reply = ConnectAndCallRPC(rh.get(), "listwallets", /* args=*/{});

        // Parse reply
        UniValue result = reply.find_value("result");
        const UniValue& error = reply.find_value("error");
        if (error.isNull()) {
            LogPrintf("Test connection to RPC: OK\n");

            reply = ConnectAndCallRPC(rh.get(), "loadwallet", /* args=*/{walletName});

            UniValue result = reply.find_value("result");
            const UniValue& error = reply.find_value("error");

            if (error.isNull()) {
                LogPrintf("Test load wallet %s: OK\n", walletName);

                reply = ConnectAndCallRPC(rh.get(), "getwalletinfo", /* args=*/{}, walletName);

                UniValue result = reply.find_value("result");
                const UniValue& error = reply.find_value("error");

                if (error.isNull()) {
                    if (!result["blsct"].get_bool()) {
                        LogPrintf("Wallet %s is not of type blsct\n", walletName);
                        return false;
                    }

                    if (result["unlocked_until"].isNull())
                        return true;

                    if (result["unlocked_until"].get_real() == 0) {
                        LogPrintf("Wallet is locked. Testing password.\n");

                        mustUnlockWallet = true;

                        reply = ConnectAndCallRPC(rh.get(), "walletpassphrase", /* args=*/{walletPassphrase, "1"}, walletName);

                        UniValue result = reply.find_value("result");
                        const UniValue& error = reply.find_value("error");

                        if (error.isNull()) {
                            LogPrintf("Wallet passphrase test: OK\n");
                            return true;
                        } else {
                            std::string strError;
                            int nRet;

                            ParseError(error, strError, nRet);
                            LogPrintf("Could not unlock wallet %s (%s)\n", walletName, strError);

                            return false;
                        }
                    }

                    return true;
                } else {
                    std::string strError;
                    int nRet;

                    ParseError(error, strError, nRet);
                    LogPrintf("Could not get wallet %s info (%s)\n", walletName, strError);

                    return false;
                }
            } else {
                std::string strError;
                int nRet;

                ParseError(error, strError, nRet);

                LogPrintf("Could not load wallet %s (%s)\n", walletName, strError);
                return false;
            }
        } else {
            LogPrintf("Could not connect to RPC node: %s\n", error.getValStr());
            return false;
            // ParseError(error, strPrint, nRet);
        }
    } catch (const std::exception& e) {
        LogPrintf("error: %s\n", e.what());
        return false;
    }

    return true;
}

MAIN_FUNCTION
{
#ifdef WIN32
    common::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
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
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRPC()");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;

    Setup();
    if (!TestSetup())
        return ret;

    return ret;

    try {
        ret = CommandLineRPC(argc, argv);
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
    }
    return ret;
}
