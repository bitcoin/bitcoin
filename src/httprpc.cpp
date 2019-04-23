// Copyright (c) 2015-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httprpc.h>

#include <chainparams.h>
#include <httpserver.h>
#include <key_io.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <random.h>
#include <sync.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <ui_interface.h>
#include <walletinitinterface.h>
#include <crypto/hmac_sha256.h>
#include <stdio.h>

#include <memory>

#include <boost/algorithm/string.hpp> // boost::trim

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis) :
        ev(eventBase, false, func)
    {
        struct timeval tv;
        tv.tv_sec = millis/1000;
        tv.tv_usec = (millis%1000)*1000;
        ev.trigger(&tv);
    }
private:
    HTTPEvent ev;
};

class HTTPRPCTimerInterface : public RPCTimerInterface
{
public:
    explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
    {
    }
    const char* Name() override
    {
        return "HTTP";
    }
    RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override
    {
        return new HTTPRPCTimer(base, func, millis);
    }
private:
    struct event_base* base;
};


/* Pre-base64-encoded authentication token */
static std::string strRPCUserColonPass;
/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;

static void JSONErrorReply(HTTPRequest* req, const UniValue& objError, const UniValue& id)
{
    // Send error reply from json-rpc error object
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = find_value(objError, "code").get_int();

    if (code == RPC_INVALID_REQUEST)
        nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND)
        nStatus = HTTP_NOT_FOUND;

    std::string strReply = JSONRPCReply(NullUniValue, objError, id);

    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(nStatus, strReply);
}

//This function checks username and password against -rpcauth
//entries from config file.
static bool multiUserAuthorized(std::string strUserPass)
{
    if (strUserPass.find(':') == std::string::npos) {
        return false;
    }
    std::string strUser = strUserPass.substr(0, strUserPass.find(':'));
    std::string strPass = strUserPass.substr(strUserPass.find(':') + 1);

    for (const std::string& strRPCAuth : gArgs.GetArgs("-rpcauth")) {
        //Search for multi-user login/pass "rpcauth" from config
        std::vector<std::string> vFields;
        boost::split(vFields, strRPCAuth, boost::is_any_of(":$"));
        if (vFields.size() != 3) {
            //Incorrect formatting in config file
            continue;
        }

        std::string strName = vFields[0];
        if (!TimingResistantEqual(strName, strUser)) {
            continue;
        }

        std::string strSalt = vFields[1];
        std::string strHash = vFields[2];

        static const unsigned int KEY_SIZE = 32;
        unsigned char out[KEY_SIZE];

        CHMAC_SHA256(reinterpret_cast<const unsigned char*>(strSalt.c_str()), strSalt.size()).Write(reinterpret_cast<const unsigned char*>(strPass.c_str()), strPass.size()).Finalize(out);
        std::vector<unsigned char> hexvec(out, out+KEY_SIZE);
        std::string strHashFromPass = HexStr(hexvec);

        if (TimingResistantEqual(strHashFromPass, strHash)) {
            return true;
        }
    }
    return false;
}

static bool RPCAuthorized(const std::string& strAuth, std::string& strAuthUsernameOut)
{
    if (strRPCUserColonPass.empty()) // Belt-and-suspenders measure if InitRPCAuthentication was not called
        return false;
    if (strAuth.substr(0, 6) != "Basic ")
        return false;
    std::string strUserPass64 = strAuth.substr(6);
    boost::trim(strUserPass64);
    std::string strUserPass = DecodeBase64(strUserPass64);

    if (strUserPass.find(':') != std::string::npos)
        strAuthUsernameOut = strUserPass.substr(0, strUserPass.find(':'));

    //Check if authorized under single-user field
    if (TimingResistantEqual(strUserPass, strRPCUserColonPass)) {
        return true;
    }
    return multiUserAuthorized(strUserPass);
}
// SYSCOIN This is not a CORS check. Only check if OPTIONS are
// set in order to allow localhost to localhost communications
static bool checkPreflight(HTTPRequest* req)
{
    // https://www.w3.org/TR/cors/#resource-requests

    // 1. If the Origin header is not present terminate this set of steps.
    // The request is outside the scope of this specification.
    // Syscoin Keep origin so as not to have to set Access-Control-Allow-Origin
    // to *
    std::pair<bool, std::string> origin = req->GetHeader("origin");
    if (!origin.first) {
        return false;
    }

    // 2. If the value of the Origin header is not a case-sensitive match for
    // any of the values in list of origins do not set any additional headers
    // and terminate this set of steps.
    // Note: Always matching is acceptable since the list of origins can be
    // unbounded.
    // Syscoin We are only satifying the OPTIONS request in this method in order to
    // allow localhost to localhost communications. If CORS is implemented origin
    // would be checked against an acceptable domain
    // if (origin.second != strRPCCORSDomain) {
    //     return false;
    // }

    if (req->GetRequestMethod() == HTTPRequest::OPTIONS) {
        // 6.2 Preflight Request
        // In response to a preflight request the resource indicates which
        // methods and headers (other than simple methods and simple
        // headers) it is willing to handle and whether it supports
        // credentials.
        // Resources must use the following set of steps to determine which
        // additional headers to use in the response:

        // 3. Let method be the value as result of parsing the
        // Access-Control-Request-Method header.
        // If there is no Access-Control-Request-Method header or if parsing
        // failed, do not set any additional headers and terminate this set
        // of steps. The request is outside the scope of this specification.
        std::pair<bool, std::string> method =
            req->GetHeader("access-control-request-method");
        if (!method.first) {
            return false;
        }

        // 4. Let header field-names be the values as result of parsing
        // the Access-Control-Request-Headers headers.
        // If there are no Access-Control-Request-Headers headers let header
        // field-names be the empty list.
        // If parsing failed do not set any additional headers and terminate
        // this set of steps. The request is outside the scope of this
        // specification.
        std::pair<bool, std::string> header_field_names =
            req->GetHeader("access-control-request-headers");

        // 5. If method is not a case-sensitive match for any of the
        // values in list of methods do not set any additional headers
        // and terminate this set of steps.
        // Note: Always matching is acceptable since the list of methods
        // can be unbounded.
        if (method.second != "POST") {
            return false;
        }

        // 6. If any of the header field-names is not a ASCII case-
        // insensitive match for any of the values in list of headers do not
        // set any additional headers and terminate this set of steps.
        // Note: Always matching is acceptable since the list of headers can
        // be unbounded.
        const std::string& list_of_headers = "authorization,content-type";

        // 7. If the resource supports credentials add a single
        // Access-Control-Allow-Origin header, with the value of the Origin
        // header as value, and add a single
        // Access-Control-Allow-Credentials header with the case-sensitive
        // string "true" as value.
        req->WriteHeader("Access-Control-Allow-Origin", origin.second);
        // Syscoin as we are only handling OPTIONS there is
        // no need to expose the response
        //req->WriteHeader("Access-Control-Allow-Credentials", "true");

        // 8. Optionally add a single Access-Control-Max-Age header with as
        // value the amount of seconds the user agent is allowed to cache
        // the result of the request.

        // 9. If method is a simple method this step may be skipped.
        // Add one or more Access-Control-Allow-Methods headers consisting
        // of (a subset of) the list of methods.
        // If a method is a simple method it does not need to be listed, but
        // this is not prohibited.
        // Note: Since the list of methods can be unbounded, simply
        // returning the method indicated by
        // Access-Control-Request-Method (if supported) can be enough.
        req->WriteHeader("Access-Control-Allow-Methods", method.second);

        // 10. If each of the header field-names is a simple header and none
        // is Content-Type, this step may be skipped.
        // Add one or more Access-Control-Allow-Headers headers consisting
        // of (a subset of) the list of headers.
        req->WriteHeader(
            "Access-Control-Allow-Headers",
            header_field_names.first ? header_field_names.second
                                        : list_of_headers);
        req->WriteReply(HTTP_OK);
        return true;
    }

    // 6.1 Simple Cross-Origin Request, Actual Request, and Redirects
    // In response to a simple cross-origin request or actual request the
    // resource indicates whether or not to share the response.
    // If the resource has been relocated, it indicates whether to share its
    // new URL.
    // Resources must use the following set of steps to determine which
    // additional headers to use in the response:

    // 3. If the resource supports credentials add a single
    // Access-Control-Allow-Origin header, with the value of the Origin
    // header as value, and add a single Access-Control-Allow-Credentials
    // header with the case-sensitive string "true" as value.
    req->WriteHeader("Access-Control-Allow-Origin", origin.second);
    // Syscoin as we are only handling OPTIONS there is
    // no need to expose the response
    // req->WriteHeader("Access-Control-Allow-Credentials", "true");

    // 4. If the list of exposed headers is not empty add one or more
    // Access-Control-Expose-Headers headers, with as values the header
    // field names given in the list of exposed headers.
    req->WriteHeader("Access-Control-Expose-Headers", "WWW-Authenticate");

    return false;
}
static bool HTTPReq_JSONRPC(HTTPRequest* req, const std::string &)
{    
    // SYSCOIN Check if this is a preflight request - if so set the necessary
    // headers to allow localhost to localhost communications
    if (checkPreflight(req)) {
        return true;
    }
    // JSONRPC handles only POST
    if (req->GetRequestMethod() != HTTPRequest::POST) {
        req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
        return false;
    }
    // Check authorization
    std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
    if (!authHeader.first) {
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    JSONRPCRequest jreq;
    jreq.peerAddr = req->GetPeer().ToString();
    if (!RPCAuthorized(authHeader.second, jreq.authUser)) {
        LogPrintf("ThreadRPCServer incorrect password attempt from %s\n", jreq.peerAddr);

        /* Deter brute-forcing
           If this results in a DoS the user really
           shouldn't have their RPC port exposed. */
        MilliSleep(250);

        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    try {
        // Parse request
        UniValue valRequest;
        if (!valRequest.read(req->ReadBody()))
            throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");

        // Set the URI
        jreq.URI = req->GetURI();

        std::string strReply;
        // singleton request
        if (valRequest.isObject()) {
            jreq.parse(valRequest);

            UniValue result = tableRPC.execute(jreq);

            // Send reply
            strReply = JSONRPCReply(result, NullUniValue, jreq.id);

        // array of requests
        } else if (valRequest.isArray())
            strReply = JSONRPCExecBatch(jreq, valRequest.get_array());
        else
            throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");

        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strReply);
    } catch (const UniValue& objError) {
        JSONErrorReply(req, objError, jreq.id);
        return false;
    } catch (const std::exception& e) {
        JSONErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        return false;
    }
    return true;
}

static bool InitRPCAuthentication()
{
    if (gArgs.GetArg("-rpcpassword", "") == "")
    {
        LogPrintf("No rpcpassword set - using random cookie authentication.\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            uiInterface.ThreadSafeMessageBox(
                _("Error: A fatal internal error occurred, see debug.log for details"), // Same message as AbortNode
                "", CClientUIInterface::MSG_ERROR);
            return false;
        }
    } else {
        LogPrintf("Config options rpcuser and rpcpassword will soon be deprecated. Locally-run instances may remove rpcuser to use cookie-based auth, or may be replaced with rpcauth. Please see share/rpcauth for rpcauth auth generation.\n");
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }
    if (gArgs.GetArg("-rpcauth","") != "")
    {
        LogPrintf("Using rpcauth authentication.\n");
    }
    return true;
}

bool StartHTTPRPC()
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;

    RegisterHTTPHandler("/", true, HTTPReq_JSONRPC);
    if (g_wallet_init_interface.HasWalletSupport()) {
        RegisterHTTPHandler("/wallet/", false, HTTPReq_JSONRPC);
    }
    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    return true;
}

void InterruptHTTPRPC()
{
    LogPrint(BCLog::RPC, "Interrupting HTTP RPC server\n");
}

void StopHTTPRPC()
{
    LogPrint(BCLog::RPC, "Stopping HTTP RPC server\n");
    UnregisterHTTPHandler("/", true);
    if (g_wallet_init_interface.HasWalletSupport()) {
        UnregisterHTTPHandler("/wallet/", false);
    }
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
