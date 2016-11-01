// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparamsbase.h>
#include <clientversion.h>
#include <fs.h>
#include <rpc/client.h>
#include <rpc/protocol.h>
#include <util.h>
#include <utilstrencodings.h>

#include <stdio.h>

#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <support/events.h>

#include <univalue.h>

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT=900;
static const bool DEFAULT_NAMED=false;
static const int CONTINUE_EXECUTION=-1;

std::string HelpMessageCli()
{
    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    std::string strUsage;
    strUsage += HelpMessageGroup(_("Options:"));
    strUsage += HelpMessageOpt("-?", _("This help message"));
    strUsage += HelpMessageOpt("-conf=<file>", strprintf(_("Specify configuration file (default: %s)"), BITCOIN_CONF_FILENAME));
    strUsage += HelpMessageOpt("-datadir=<dir>", _("Specify data directory"));
    strUsage += HelpMessageOpt("-getinfo", _("Get general information from the remote server. Note that unlike server-side RPC calls, the results of -getinfo is the result of multiple non-atomic requests. Some entries in the result may represent results from different states (e.g. wallet balance may be as of a different block from the chain state reported)"));
    AppendParamsHelpMessages(strUsage);
    strUsage += HelpMessageOpt("-named", strprintf(_("Pass named instead of positional arguments (default: %s)"), DEFAULT_NAMED));
    strUsage += HelpMessageOpt("-rpcconnect=<ip>", strprintf(_("Send commands to node running on <ip> (default: %s)"), DEFAULT_RPCCONNECT));
    strUsage += HelpMessageOpt("-rpcport=<port>", strprintf(_("Connect to JSON-RPC on <port> (default: %u or testnet: %u)"), defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort()));
    strUsage += HelpMessageOpt("-rpcwait", _("Wait for RPC server to start"));
    strUsage += HelpMessageOpt("-rpcuser=<user>", _("Username for JSON-RPC connections"));
    strUsage += HelpMessageOpt("-rpcpassword=<pw>", _("Password for JSON-RPC connections"));
    strUsage += HelpMessageOpt("-rpcclienttimeout=<n>", strprintf(_("Timeout in seconds during HTTP requests, or 0 for no timeout. (default: %d)"), DEFAULT_HTTP_CLIENT_TIMEOUT));
    strUsage += HelpMessageOpt("-stdinrpcpass", strprintf(_("Read RPC password from standard input as a single line.  When combined with -stdin, the first line from standard input is used for the RPC password.")));
    strUsage += HelpMessageOpt("-stdin", _("Read extra arguments from standard input, one per line until EOF/Ctrl-D (recommended for sensitive information such as passphrases).  When combined with -stdinrpcpass, the first line from standard input is used for the RPC password."));
    strUsage += HelpMessageOpt("-rpcwallet=<walletname>", _("Send RPC for non-default wallet on RPC server (argument is wallet filename in litecoind directory, required if litecoind/-Qt runs with multiple wallets)"));

    return strUsage;
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//

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
    //
    // Parameters
    //
    gArgs.ParseParameters(argc, argv);
    if (argc<2 || gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") || gArgs.IsArgSet("-help") || gArgs.IsArgSet("-version")) {
        std::string strUsage = strprintf(_("%s RPC client version"), _(PACKAGE_NAME)) + " " + FormatFullVersion() + "\n";
        if (!gArgs.IsArgSet("-version")) {
            strUsage += "\n" + _("Usage:") + "\n" +
                  "  litecoin-cli [options] <command> [params]  " + strprintf(_("Send command to %s"), _(PACKAGE_NAME)) + "\n" +
                  "  litecoin-cli [options] -named <command> [name=value] ... " + strprintf(_("Send command to %s (with named arguments)"), _(PACKAGE_NAME)) + "\n" +
                  "  litecoin-cli [options] help                " + _("List commands") + "\n" +
                  "  litecoin-cli [options] help <command>      " + _("Get help for a command") + "\n";

            strUsage += "\n" + HelpMessageCli();
        }

        fprintf(stdout, "%s", strUsage.c_str());
        if (argc < 2) {
            fprintf(stderr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (!fs::is_directory(GetDataDir(false))) {
        fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
        return EXIT_FAILURE;
    }
    try {
        gArgs.ReadConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));
    } catch (const std::exception& e) {
        fprintf(stderr,"Error reading configuration file: %s\n", e.what());
        return EXIT_FAILURE;
    }
    // Check for -testnet or -regtest parameter (BaseParams() calls are only valid after this clause)
    try {
        SelectBaseParams(ChainNameFromCommandLine());
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    if (gArgs.GetBoolArg("-rpcssl", false))
    {
        fprintf(stderr, "Error: SSL mode for RPC (-rpcssl) is no longer supported.\n");
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

const char *http_errorstring(int code)
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
    virtual UniValue PrepareRequest(const std::string& method, const std::vector<std::string>& args) = 0;
    virtual UniValue ProcessReply(const UniValue &batch_in) = 0;
};

/** Process getinfo requests */
class GetinfoRequestHandler: public BaseRequestHandler
{
public:
    const int ID_NETWORKINFO = 0;
    const int ID_BLOCKCHAININFO = 1;
    const int ID_WALLETINFO = 2;

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
        return result;
    }

    /** Collect values from the batch and form a simulated `getinfo` reply. */
    UniValue ProcessReply(const UniValue &batch_in) override
    {
        UniValue result(UniValue::VOBJ);
        std::vector<UniValue> batch = JSONRPCProcessBatchReply(batch_in, 3);
        // Errors in getnetworkinfo() and getblockchaininfo() are fatal, pass them on
        // getwalletinfo() is allowed to fail in case there is no wallet.
        if (!batch[ID_NETWORKINFO]["error"].isNull()) {
            return batch[ID_NETWORKINFO];
        }
        if (!batch[ID_BLOCKCHAININFO]["error"].isNull()) {
            return batch[ID_BLOCKCHAININFO];
        }
        result.pushKV("version", batch[ID_NETWORKINFO]["result"]["version"]);
        result.pushKV("protocolversion", batch[ID_NETWORKINFO]["result"]["protocolversion"]);
        if (!batch[ID_WALLETINFO].isNull()) {
            result.pushKV("walletversion", batch[ID_WALLETINFO]["result"]["walletversion"]);
            result.pushKV("balance", batch[ID_WALLETINFO]["result"]["balance"]);
        }
        result.pushKV("blocks", batch[ID_BLOCKCHAININFO]["result"]["blocks"]);
        result.pushKV("timeoffset", batch[ID_NETWORKINFO]["result"]["timeoffset"]);
        result.pushKV("connections", batch[ID_NETWORKINFO]["result"]["connections"]);
        result.pushKV("proxy", batch[ID_NETWORKINFO]["result"]["networks"][0]["proxy"]);
        result.pushKV("difficulty", batch[ID_BLOCKCHAININFO]["result"]["difficulty"]);
        result.pushKV("testnet", UniValue(batch[ID_BLOCKCHAININFO]["result"]["chain"].get_str() == "test"));
        if (!batch[ID_WALLETINFO].isNull()) {
            result.pushKV("walletversion", batch[ID_WALLETINFO]["result"]["walletversion"]);
            result.pushKV("balance", batch[ID_WALLETINFO]["result"]["balance"]);
            result.pushKV("keypoololdest", batch[ID_WALLETINFO]["result"]["keypoololdest"]);
            result.pushKV("keypoolsize", batch[ID_WALLETINFO]["result"]["keypoolsize"]);
            if (!batch[ID_WALLETINFO]["result"]["unlocked_until"].isNull()) {
                result.pushKV("unlocked_until", batch[ID_WALLETINFO]["result"]["unlocked_until"]);
            }
            result.pushKV("paytxfee", batch[ID_WALLETINFO]["result"]["paytxfee"]);
        }
        result.pushKV("relayfee", batch[ID_NETWORKINFO]["result"]["relayfee"]);
        result.pushKV("warnings", batch[ID_NETWORKINFO]["result"]["warnings"]);
        return JSONRPCReplyObj(result, NullUniValue, 1);
    }
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

static UniValue CallRPC(BaseRequestHandler *rh, const std::string& strMethod, const std::vector<std::string>& args)
{
    std::string host;
    // In preference order, we choose the following for the port:
    //     1. -rpcport
    //     2. port in -rpcconnect (ie following : in ipv4 or ]: in ipv6)
    //     3. default port for chain
    int port = BaseParams().RPCPort();
    SplitHostPort(gArgs.GetArg("-rpcconnect", DEFAULT_RPCCONNECT), port, host);
    port = gArgs.GetArg("-rpcport", port);

    // Obtain event base
    raii_event_base base = obtain_event_base();

    // Synchronously look up hostname
    raii_evhttp_connection evcon = obtain_evhttp_connection_base(base.get(), host, port);
    evhttp_connection_set_timeout(evcon.get(), gArgs.GetArg("-rpcclienttimeout", DEFAULT_HTTP_CLIENT_TIMEOUT));

    HTTPReply response;
    raii_evhttp_request req = obtain_evhttp_request(http_request_done, (void*)&response);
    if (req == nullptr)
        throw std::runtime_error("create http request failed");
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    evhttp_request_set_error_cb(req.get(), http_error_cb);
#endif

    // Get credentials
    std::string strRPCUserColonPass;
    if (gArgs.GetArg("-rpcpassword", "") == "") {
        // Try fall back to cookie-based authentication if no password is provided
        if (!GetAuthCookie(&strRPCUserColonPass)) {
            throw std::runtime_error(strprintf(
                _("Could not locate RPC credentials. No authentication cookie could be found, and RPC password is not set.  See -rpcpassword and -stdinrpcpass.  Configuration file: (%s)"),
                    GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME)).string().c_str()));

        }
    } else {
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }

    struct evkeyvalq* output_headers = evhttp_request_get_output_headers(req.get());
    assert(output_headers);
    evhttp_add_header(output_headers, "Host", host.c_str());
    evhttp_add_header(output_headers, "Connection", "close");
    evhttp_add_header(output_headers, "Authorization", (std::string("Basic ") + EncodeBase64(strRPCUserColonPass)).c_str());

    // Attach request data
    std::string strRequest = rh->PrepareRequest(strMethod, args).write() + "\n";
    struct evbuffer* output_buffer = evhttp_request_get_output_buffer(req.get());
    assert(output_buffer);
    evbuffer_add(output_buffer, strRequest.data(), strRequest.size());

    // check if we should use a special wallet endpoint
    std::string endpoint = "/";
    std::string walletName = gArgs.GetArg("-rpcwallet", "");
    if (!walletName.empty()) {
        char *encodedURI = evhttp_uriencode(walletName.c_str(), walletName.size(), false);
        if (encodedURI) {
            endpoint = "/wallet/"+ std::string(encodedURI);
            free(encodedURI);
        }
        else {
            throw CConnectionFailed("uri-encode failed");
        }
    }
    int r = evhttp_make_request(evcon.get(), req.get(), EVHTTP_REQ_POST, endpoint.c_str());
    req.release(); // ownership moved to evcon in above call
    if (r != 0) {
        throw CConnectionFailed("send http request failed");
    }

    event_base_dispatch(base.get());

    if (response.status == 0)
        throw CConnectionFailed(strprintf("couldn't connect to server: %s (code %d)\n(make sure server is running and you are connecting to the correct RPC port)", http_errorstring(response.error), response.error));
    else if (response.status == HTTP_UNAUTHORIZED)
        throw std::runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
    else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST && response.status != HTTP_NOT_FOUND && response.status != HTTP_INTERNAL_SERVER_ERROR)
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

int CommandLineRPC(int argc, char *argv[])
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
            if (!std::getline(std::cin, rpcPass)) {
                throw std::runtime_error("-stdinrpcpass specified but failed to read from standard input");
            }
            gArgs.ForceSetArg("-rpcpassword", rpcPass);
        }
        std::vector<std::string> args = std::vector<std::string>(&argv[1], &argv[argc]);
        if (gArgs.GetBoolArg("-stdin", false)) {
            // Read one arg per line from stdin and append
            std::string line;
            while (std::getline(std::cin, line)) {
                args.push_back(line);
            }
        }
        std::unique_ptr<BaseRequestHandler> rh;
        std::string method;
        if (gArgs.GetBoolArg("-getinfo", false)) {
            rh.reset(new GetinfoRequestHandler());
            method = "";
        } else {
            rh.reset(new DefaultRequestHandler());
            if (args.size() < 1) {
                throw std::runtime_error("too few parameters (need at least command)");
            }
            method = args[0];
            args.erase(args.begin()); // Remove trailing method name from arguments vector
        }

        // Execute and handle connection failures with -rpcwait
        const bool fWait = gArgs.GetBoolArg("-rpcwait", false);
        do {
            try {
                const UniValue reply = CallRPC(rh.get(), method, args);

                // Parse reply
                const UniValue& result = find_value(reply, "result");
                const UniValue& error  = find_value(reply, "error");

                if (!error.isNull()) {
                    // Error
                    int code = error["code"].get_int();
                    if (fWait && code == RPC_IN_WARMUP)
                        throw CConnectionFailed("server in warmup");
                    strPrint = "error: " + error.write();
                    nRet = abs(code);
                    if (error.isObject())
                    {
                        UniValue errCode = find_value(error, "code");
                        UniValue errMsg  = find_value(error, "message");
                        strPrint = errCode.isNull() ? "" : "error code: "+errCode.getValStr()+"\n";

                        if (errMsg.isStr())
                            strPrint += "error message:\n"+errMsg.get_str();

                        if (errCode.isNum() && errCode.get_int() == RPC_WALLET_NOT_SPECIFIED) {
                            strPrint += "\nTry adding \"-rpcwallet=<filename>\" option to litecoin-cli command line.";
                        }
                    }
                } else {
                    // Result
                    if (result.isNull())
                        strPrint = "";
                    else if (result.isStr())
                        strPrint = result.get_str();
                    else
                        strPrint = result.write(2);
                }
                // Connection succeeded, no need to retry.
                break;
            }
            catch (const CConnectionFailed&) {
                if (fWait)
                    MilliSleep(1000);
                else
                    throw;
            }
        } while (fWait);
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    }
    catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
        throw;
    }

    if (strPrint != "") {
        fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
    }
    return nRet;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();
    if (!SetupNetworking()) {
        fprintf(stderr, "Error: Initializing networking failed\n");
        return EXIT_FAILURE;
    }

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
