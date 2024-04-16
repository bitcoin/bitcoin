// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <blsct/arith/mcl/mcl_init.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <common/url.h>
#include <compat/compat.h>
#include <compat/stdin.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <logging.h>
#include <policy/feerate.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/client.h>
#include <rpc/mining.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/exception.h>
#include <util/strencodings.h>
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
static const int CONTINUE_EXECUTION = -1;
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
    argsman.AddArg("-coinbasedest=<address>", "Specify the address to collect the staking rewards", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
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
                        "Usage:  navcoin-staker [options] Start the staker \n"
                        "or:     navcoin-staker [options] help        List commands\n";
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
        throw CConnectionFailed(strprintf("Could not connect to the server %s:%d%s\n\nMake sure the navcoind server is running and that you are connecting to the correct RPC port.", host, port, responseErrorMessage));
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

/** Parse UniValue error to update the message to print to std::cerr and the code to return. */
static void ParseError(const UniValue& error, std::string& strPrint, int& nRet)
{
    if (error.isObject()) {
        const UniValue& err_code = error.find_value("code");
        const UniValue& err_msg = error.find_value("message");
        if (!err_code.isNull()) {
            strPrint = "error code: " + err_code.getValStr() + " ";
        }
        if (err_msg.isStr()) {
            strPrint += ("error message: " + err_msg.get_str());
        }
        if (err_code.isNum() && (err_code.getInt<int>() == RPC_WALLET_NOT_SPECIFIED || err_code.getInt<int>() == RPC_WALLET_NOT_FOUND)) {
            strPrint += " Try adding \"-wallet=<filename>\" option to navcoin-staker command line.";
        }
        if (err_code.isNum() && err_code.getInt<int>() == RPC_WALLET_PASSPHRASE_INCORRECT) {
            strPrint += " The wallet passphrase is incorrect.";
        }
    } else {
        strPrint = "error: " + error.write();
    }
    nRet = abs(error["code"].getInt<int>());
}

static std::string rpcPass = "";
static std::string walletName = "";
static std::string walletPassphrase = "";
static std::string coinbase_dest = "";
static bool mustUnlockWallet = false;
static arith_uint256 currentDifficulty;

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
    } else if (gArgs.IsArgSet("-rpcpassword"))
        rpcPass = gArgs.GetArg("-rpcpassword", {});


    if (gArgs.GetBoolArg("-stdinwalletpassphrase", false)) {
        NO_STDIN_ECHO();
        if (!StdinReady()) {
            fputs("Wallet password> ", stderr);
            fflush(stderr);
        }
        if (!std::getline(std::cin, walletPassphrase)) {
            throw std::runtime_error("-stdinwalletpassphrase specified but failed to read from standard input");
        }
        if (StdinTerminal()) {
            fputc('\n', stdout);
        }
        gArgs.ForceSetArg("-walletpassphrase", walletPassphrase);
    } else if (gArgs.IsArgSet("-walletpassphrase"))
        walletPassphrase = gArgs.GetArg("-walletpassphrase", {});

    if (gArgs.IsArgSet("-coinbasedest"))
        coinbase_dest = gArgs.GetArg("-coinbasedest", {});

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

        std::string strError;
        int nRet;

        if (error.isNull()) {
            LogPrintf("%s: [%s] Test connection to RPC: OK\n", __func__, walletName);

            reply = ConnectAndCallRPC(rh.get(), "loadwallet", /* args=*/{walletName});

            UniValue result = reply.find_value("result");
            const UniValue& error = reply.find_value("error");

            strError.clear();
            nRet = 0;

            if (!error.isNull()) {
                ParseError(error, strError, nRet);
            }

            if (error.isNull() || nRet == 35) {
                LogPrintf("%s: [%s] Test load wallet: OK\n", __func__, walletName);

                reply = ConnectAndCallRPC(rh.get(), "getwalletinfo", /* args=*/{}, walletName);

                UniValue result = reply.find_value("result");
                const UniValue& error = reply.find_value("error");

                strError.clear();
                nRet = 0;

                if (!error.isNull()) {
                    ParseError(error, strError, nRet);
                }

                if (error.isNull()) {
                    if (!result["blsct"].get_bool()) {
                        LogPrintf("%s: [%s] Wallet is not of type blsct\n", __func__, walletName);
                        return false;
                    }

                    if (!result["unlocked_until"].isNull() && result["unlocked_until"].get_real() == 0) {
                        LogPrintf("%s: [%s] Wallet is locked. Testing password.\n", __func__, walletName);

                        mustUnlockWallet = true;

                        reply = ConnectAndCallRPC(rh.get(), "walletpassphrase", /* args=*/{walletPassphrase, "1"}, walletName);

                        UniValue result = reply.find_value("result");
                        const UniValue& error = reply.find_value("error");

                        strError.clear();
                        nRet = 0;

                        if (error.isNull()) {
                            LogPrintf("%s: [%s] Wallet passphrase test: OK\n", __func__, walletName);
                        } else {
                            ParseError(error, strError, nRet);
                            LogPrintf("%s: [%s] Could not unlock wallet (%s)\n", __func__, walletName, strError);

                            return false;
                        }
                    }

                    if (coinbase_dest == "") {
                        reply = ConnectAndCallRPC(rh.get(), "getaddressesbylabel", /* args=*/{"Staking"}, walletName);

                        UniValue result = reply.find_value("result");
                        const UniValue& error = reply.find_value("error");

                        if (error.isNull() && result.isObject()) {
                            auto array = result.get_obj();

                            for (auto& it : array.getKeys()) {
                                auto obj = array.find_value(it);

                                if (obj.isObject()) {
                                    if (obj.get_obj().find_value("purpose").get_str() == "receive") {
                                        coinbase_dest = it;
                                        break;
                                    }
                                }
                            }
                        }

                        if (coinbase_dest == "") {
                            reply = ConnectAndCallRPC(rh.get(), "getnewaddress", /* args=*/{"Staking", "blsct"}, walletName);

                            UniValue result = reply.find_value("result");
                            const UniValue& error = reply.find_value("error");

                            strError.clear();
                            nRet = 0;

                            if (error.isNull() || !result.isStr()) {
                                coinbase_dest = result.get_str();
                            } else {
                                ParseError(error, strError, nRet);
                                LogPrintf("%s: [%s] Could not get an address for rewards from wallet (%s)\n", __func__, walletName, strError);

                                return false;
                            }
                        }
                    }

                    LogPrintf("%s: [%s] Rewards address: %s\n", __func__, walletName, coinbase_dest);

                    return true;
                } else {
                    LogPrintf("%s: [%s] Could not get wallet info (%s)\n", __func__, walletName, strError);
                    return false;
                }
            } else {
                LogPrintf("%s: [%s] Could not load wallet (%s)\n", __func__, walletName, strError);
                return false;
            }
        } else {
            LogPrintf("%s: [%s] Could not connect to RPC node: %s\n", __func__, walletName, error.getValStr());
            return false;
        }
    } catch (const std::exception& e) {
        LogPrintf("%s: [%s] error: %s\n", __func__, walletName, e.what());
        return false;
    }

    return true;
}

struct StakedCommitment {
    MclG1Point point;
    MclScalar value;
    MclScalar gamma;
};

std::vector<StakedCommitment>
UniValueArrayToStakedCommitmentsMine(const UniValue& array)
{
    std::vector<StakedCommitment> ret;

    for (const UniValue& elementobject : array.getValues()) {
        auto obj = elementobject.get_obj();
        MclG1Point point;
        MclScalar value;
        MclScalar gamma;
        point.SetVch(ParseHex(obj.find_value("commitment").get_str()));
        value.SetVch(ParseHex(obj.find_value("value").get_str()));
        gamma.SetVch(ParseHex(obj.find_value("gamma").get_str()));
        ret.push_back({point, value, gamma});
    }
    return ret;
}

std::vector<MclG1Point>
UniValueArrayToStakedCommitments(const UniValue& array)
{
    std::vector<MclG1Point> ret;

    for (const UniValue& elementobject : array.getValues()) {
        MclG1Point point;
        point.SetVch(ParseHex(elementobject.get_str()));
        ret.push_back(point);
    }

    sort(ret.begin(), ret.end(), [](const auto& lhs, const auto& rhs) {
        return lhs < rhs;
    });

    return ret;
}

std::vector<std::shared_ptr<const CTransaction>>
UniValueArrayToTransactions(const UniValue& array)
{
    std::vector<std::shared_ptr<const CTransaction>> ret;

    for (const UniValue& object : array.getValues()) {
        CMutableTransaction tx;
        if (DecodeHexTx(tx, object.find_value("data").get_str()))
            ret.push_back(MakeTransactionRef(CTransaction(tx)));
    }

    return ret;
}

std::string EncodeHexBlock(const CBlock& block)
{
    DataStream ssBlock;
    ssBlock << TX_WITH_WITNESS(block);
    return HexStr(ssBlock);
}

std::vector<StakedCommitment> GetStakedCommitments(const std::unique_ptr<BaseRequestHandler>& rh)
{
    UniValue reply_staked = ConnectAndCallRPC(rh.get(), "liststakedcommitments", /* args=*/{}, walletName);

    UniValue result = reply_staked.find_value("result");

    return UniValueArrayToStakedCommitmentsMine(result.get_array());
}

std::optional<CBlock> GetBlockProposal(const std::unique_ptr<BaseRequestHandler>& rh, const StakedCommitment& staked_commitment)
{
    CBlock proposal;

    auto reply = ConnectAndCallRPC(rh.get(), "getblocktemplate", /* args=*/{"{\"rules\": [\"\"], \"coinbasedest\": \"" + coinbase_dest + "\"}"}, walletName);
    UniValue result = reply.find_value("result");
    const UniValue& error = reply.find_value("error");

    std::string strError = "";
    auto nRet = 0;

    if (!error.isNull()) {
        ParseError(error, strError, nRet);
        LogPrintf("%s: [%s] Could not get block template (%s)\n", __func__, walletName, strError);

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        return std::nullopt;
    }

    auto eta_fiat_shamir = ParseHex(result.find_value("eta_fiat_shamir").get_str());
    auto eta_phi = ParseHex(result.find_value("eta_phi").get_str());

    MclScalar m = staked_commitment.value;
    MclScalar f = staked_commitment.gamma;

    uint32_t prev_time = result.find_value("prev_time").get_real();
    uint64_t modifier = result.find_value("modifier").get_uint64();
    uint64_t next_target = stoi(result.find_value("bits").get_str(), 0, 16);
    currentDifficulty.SetCompact(next_target);

    proposal.nVersion = result.find_value("version").get_real();
    proposal.nTime = result.find_value("curtime").get_real();
    proposal.nBits = next_target;
    proposal.hashPrevBlock = uint256S(result.find_value("previousblockhash").get_str());
    proposal.vtx = UniValueArrayToTransactions(result.find_value("transactions").get_array());

    auto staked_elements = UniValueArrayToStakedCommitments(result.find_value("staked_commitments").get_array());

    proposal.posProof = blsct::ProofOfStake(staked_elements, eta_fiat_shamir, eta_phi, m, f, prev_time, modifier, proposal.nTime, next_target);
    proposal.hashMerkleRoot = BlockMerkleRoot(proposal);

    auto valid = blsct::ProofOfStake(proposal.posProof).Verify(staked_elements, eta_fiat_shamir, eta_phi, blsct::CalculateKernelHash(prev_time, modifier, proposal.posProof.setMemProof.phi, proposal.nTime), next_target);

    if (valid) return proposal;

    return std::nullopt;
}


void Loop()
{
    std::unique_ptr<BaseRequestHandler> rh;

    rh.reset(new DefaultRequestHandler());

    auto last_update{SteadyClock::now()};
    auto start{SteadyClock::now()};
    double nFound = 0;

    LogPrintf("%s: [%s] Starting staking...\n", __func__, walletName);

    while (true) {
        auto staked_commitments = GetStakedCommitments(rh);
        CBlock proposal;
        CAmount nTotalMoney = 0;
        bool found = false;

        for (auto& it : staked_commitments) {
            nTotalMoney += it.value.GetUint64();

            if (!found) {
                auto candidate = GetBlockProposal(rh, it);

                found = candidate.has_value();
                if (found)
                    proposal = candidate.value();
            }
        }

        if (found) {
            UniValue reply_submit = ConnectAndCallRPC(rh.get(), "submitblock", /* args=*/{EncodeHexBlock(proposal)}, walletName);

            UniValue result_submit = reply_submit.find_value("result");
            UniValue error_submit = reply_submit.find_value("error");

            if (error_submit.isNull()) {
                if (result_submit.isNull()) {
                    nFound++;
                    last_update = SteadyClock::now();
                }
                LogPrintf("%s: [%s] Found block %s (%s%s). Current difficulty: %s\n", __func__, walletName, proposal.GetHash().ToString(), result_submit.isNull() ? "ACCEPTED" : "REJECTED: ", result_submit.isNull() ? "" : reply_submit.write(0, 0), currentDifficulty.ToString());

                auto elapsed = Ticks<std::chrono::minutes>(SteadyClock::now() - start);

                if (elapsed > 60) {
                    LogPrintf("%s: [%s] Stake rate: %.4f blocks/hour. Staking balance: %s\n", __func__, walletName, (nFound / elapsed) * 60.0f, FormatMoney(nTotalMoney));
                }
            }
        }

        if (Ticks<std::chrono::milliseconds>(SteadyClock::now() - last_update) > 60000) {
            last_update = SteadyClock::now();
            LogPrintf("%s: [%s] Did not find a block yet. Current difficulty: %s\n", __func__, walletName, currentDifficulty.ToString());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

MAIN_FUNCTION
{
#ifdef WIN32
    common::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    volatile MclInit for_side_effect_only;

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

    Loop();

    return EXIT_SUCCESS;
}
