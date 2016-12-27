// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "httprpc.h"

#include "base58.h"
#include "chainparams.h"
#include "httpserver.h"
#include "rpc/protocol.h"
#include "rpc/server.h"
#include "random.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include "ui_interface.h"
#include "crypto/hmac_sha256.h"
#include <stdio.h>
#include "utilstrencodings.h"

#include <boost/algorithm/string.hpp> // boost::trim
#include <boost/foreach.hpp> //BOOST_FOREACH

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wellet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, boost::function<void(void)>& func, int64_t millis) :
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
    HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
    {
    }
    const char* Name()
    {
        return "HTTP";
    }
    RPCTimerBase* NewTimer(boost::function<void(void)>& func, int64_t millis)
    {
        return new HTTPRPCTimer(base, func, millis);
    }
private:
    struct event_base* base;
};


/* Pre-base64-encoded authentication token */
static std::string strRPCUserColonPass;
/* Stored RPC timer interface (for unregistration) */
static HTTPRPCTimerInterface* httpRPCTimerInterface = 0;

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
    if (strUserPass.find(":") == std::string::npos) {
        return false;
    }
    std::string strUser = strUserPass.substr(0, strUserPass.find(":"));
    std::string strPass = strUserPass.substr(strUserPass.find(":") + 1);

    if (mapMultiArgs.count("-rpcauth") > 0) {
        //Search for multi-user login/pass "rpcauth" from config
        BOOST_FOREACH(std::string strRPCAuth, mapMultiArgs.at("-rpcauth"))
        {
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

            unsigned int KEY_SIZE = 32;
            unsigned char *out = new unsigned char[KEY_SIZE]; 
            
            CHMAC_SHA256(reinterpret_cast<const unsigned char*>(strSalt.c_str()), strSalt.size()).Write(reinterpret_cast<const unsigned char*>(strPass.c_str()), strPass.size()).Finalize(out);
            std::vector<unsigned char> hexvec(out, out+KEY_SIZE);
            std::string strHashFromPass = HexStr(hexvec);

            if (TimingResistantEqual(strHashFromPass, strHash)) {
                return true;
            }
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

    if (strUserPass.find(":") != std::string::npos)
        strAuthUsernameOut = strUserPass.substr(0, strUserPass.find(":"));

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
            strReply = JSONRPCExecBatch(valRequest.get_array());
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
    if (GetArg("-rpcpassword", "") == "")
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
        strRPCUserColonPass = GetArg("-rpcuser", "") + ":" + GetArg("-rpcpassword", "");
    }
    return true;
}

bool StartHTTPRPC()
{
    LogPrint("rpc", "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;

    RegisterHTTPHandler("/", true, HTTPReq_JSONRPC);

    assert(EventBase());
    httpRPCTimerInterface = new HTTPRPCTimerInterface(EventBase());
    RPCSetTimerInterface(httpRPCTimerInterface);
    return true;
}

void InterruptHTTPRPC()
{
    LogPrint("rpc", "Interrupting HTTP RPC server\n");
}

void StopHTTPRPC()
{
    LogPrint("rpc", "Stopping HTTP RPC server\n");
    UnregisterHTTPHandler("/", true);
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface);
        delete httpRPCTimerInterface;
        httpRPCTimerInterface = 0;
    }
}
