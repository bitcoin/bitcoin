// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httprpc.h>

#include <base58.h>
#include <chainparams.h>
#include <httpserver.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <random.h>
#include <sync.h>
#include <util.h>
#include <utilstrencodings.h>
#include <ui_interface.h>
#include <crypto/hmac_sha256.h>
#include <stdio.h>

#include <boost/algorithm/string.hpp> // boost::trim

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void(void)>& func, int64_t millis) :
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
    RPCTimerBase* NewTimer(std::function<void(void)>& func, int64_t millis) override
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

static void PoCJSONErrorReply(HTTPRequest* req, int nErrorCode, const std::string &strErrorDescription) 
{
    UniValue result;
    result.pushKV("errorCode", nErrorCode);
    result.pushKV("errorDescription", strErrorDescription);

    req->WriteHeader("Content-Type", "application/json; charset=UTF-8");
    req->WriteReply(HTTP_OK, result.write());
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

static bool HTTPReq_JSONRPC(HTTPRequest* req, const std::string &)
{
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
    if (!RPCAuthorized(authHeader.second, jreq.authUser)) {
        LogPrintf("ThreadRPCServer incorrect password attempt from %s\n", req->GetPeer().ToString());

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

static bool AdjustSubmitNonceParam(JSONRPCRequest& jreq, HTTPRequest* req, const std::map<std::string, std::string>& parameters)
{
    const auto secretPhrase = parameters.find("secretPhrase");
    const auto plotterId = parameters.find("accountId");
    const auto nonce = parameters.find("nonce");
    const auto height = parameters.find("height");
    const auto address = parameters.find("address");
    const auto checkBind = parameters.find("checkBind");

    if (nonce != parameters.cend()) {
        jreq.params.pushKV("nonce", nonce->second);
    } else {
        PoCJSONErrorReply(req, 1, "Incorrect request");
        return false;
    }

    if (plotterId != parameters.cend() && !plotterId->second.empty()) {
        jreq.params.pushKV("plotterId", plotterId->second);
    } else if (secretPhrase != parameters.cend() && !secretPhrase->second.empty()) {
        // secretPhrase to plotterId
        jreq.params.pushKV("plotterId", PocLegacy::GeneratePlotterId(secretPhrase->second));
    } else {
        PoCJSONErrorReply(req, 1, "Incorrect request");
        return false;
    }

    // Other optional parameters
    if (height != parameters.cend())
        jreq.params.pushKV("height", height->second);
    if (address != parameters.cend())
        jreq.params.pushKV("address", address->second);
    if (checkBind != parameters.cend())
        jreq.params.pushKV("checkBind", checkBind->second);

    return true;
}

static bool HTTPReq_PoCJSONRPC(HTTPRequest* req, const std::string &)
{
    JSONRPCRequest jreq;
    try {
        // Parse request
        const std::map<std::string,std::string> parameters = req->GetParameters();
        const auto requestType = parameters.find("requestType");
        if (requestType == parameters.cend()) {
            LogPrintf("Not find requestType, threadid=%ld \n", std::this_thread::get_id());
            PoCJSONErrorReply(req, 1, "Incorrect request");
            return false;
        }
        // getMiningInfo support GET
        if (req->GetRequestMethod() != HTTPRequest::POST && (requestType->second != "getMiningInfo" || req->GetRequestMethod() != HTTPRequest::GET)) {
            LogPrintf("The uri: %s\n", req->GetURI().c_str());
            req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
            return false;
        }
        const time_t startTime = ::time(nullptr);

        // Set the request
        jreq.URI = req->GetURI();
        jreq.params.setObject();
        if (requestType->second == "submitNonce") {
            jreq.strMethod = requestType->second;
            if (!AdjustSubmitNonceParam(jreq, req, parameters))
                return false;
        } 
        else {
            jreq.strMethod = requestType->second;
            for (auto it = parameters.cbegin(); it != parameters.cend(); it++) {
                if (it->first == "requestType") {
                    continue;
                }
                jreq.params.pushKV(it->first, it->second);
            }
        }

        // Call
        UniValue result = tableRPC.execute(jreq);
        if (result.isBool()) {
            if (result.isFalse()) {
                PoCJSONErrorReply(req, 1, "Incorrect request");
                return false;
            } else {
                result.setObject();
            }
        } else if (result.isNull() || !result.isObject()) {
            PoCJSONErrorReply(req, 1, "Incorrect request");
            return false;
        }

        result.pushKV("requestProcessingTime", UniValue((int64_t)(::time(nullptr) - startTime)));

        // Send reply
        req->WriteHeader("Content-Type", "application/json; charset=UTF-8");
        req->WriteReply(HTTP_OK, result.write());
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
        LogPrintf("No rpcpassword set - using random cookie authentication\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            uiInterface.ThreadSafeMessageBox(
                _("Error: A fatal internal error occurred, see debug.log for details"), // Same message as AbortNode
                "", CClientUIInterface::MSG_ERROR);
            return false;
        }
    } else {
        LogPrintf("Config options rpcuser and rpcpassword will soon be deprecated. Locally-run instances may remove rpcuser to use cookie-based auth, or may be replaced with rpcauth. Please see share/rpcuser for rpcauth auth generation.\n");
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }
    return true;
}

bool StartHTTPRPC()
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");

    if (!InitRPCAuthentication())
        return false;

    RegisterHTTPHandler("/", true, HTTPReq_JSONRPC);

#ifdef ENABLE_WALLET
    // ifdef can be removed once we switch to better endpoint support and API versioning
    RegisterHTTPHandler("/wallet/", false, HTTPReq_JSONRPC);
#endif

    RegisterHTTPHandler("/burst", false, HTTPReq_PoCJSONRPC);

    assert(EventBase());
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(EventBase());
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
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
