// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httprpc.h>

#include <common/args.h>
#include <crypto/hmac_sha256.h>
#include <httpserver.h>
#include <netaddress.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/log.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <walletinitinterface.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

using util::SplitString;
using util::TrimStringView;

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/* List of -rpcauth values */
static std::vector<std::vector<std::string>> g_rpcauth;
/* RPC Auth Whitelist */
static std::map<std::string, std::set<std::string>> g_rpc_whitelist;
static bool g_rpc_whitelist_default = false;

static UniValue JSONErrorReply(UniValue objError, const JSONRPCRequest& jreq, HTTPStatusCode& nStatus)
{
    // HTTP errors should never be returned if JSON-RPC v2 was requested. This
    // function should only be called when a v1 request fails or when a request
    // cannot be parsed, so the version is unknown.
    Assume(jreq.m_json_version != JSONRPCVersion::V2);

    // Send error reply from json-rpc error object
    nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = objError.find_value("code").getInt<int>();

    if (code == RPC_INVALID_REQUEST)
        nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND)
        nStatus = HTTP_NOT_FOUND;
    else if (code == RPC_METHOD_NOT_PERMITTED)
        nStatus = HTTP_FORBIDDEN;

    return JSONRPCReplyObj(NullUniValue, std::move(objError), jreq.id, jreq.m_json_version);
}

//This function checks username and password against -rpcauth
//entries from config file.
static bool CheckUserAuthorized(std::string_view user, std::string_view pass)
{
    for (const auto& fields : g_rpcauth) {
        if (!TimingResistantEqual(std::string_view(fields[0]), user)) {
            continue;
        }

        const std::string& salt = fields[1];
        const std::string& hash = fields[2];

        std::array<unsigned char, CHMAC_SHA256::OUTPUT_SIZE> out;
        CHMAC_SHA256(UCharCast(salt.data()), salt.size()).Write(UCharCast(pass.data()), pass.size()).Finalize(out.data());
        std::string hash_from_pass = HexStr(out);

        if (TimingResistantEqual(hash_from_pass, hash)) {
            return true;
        }
    }
    return false;
}

static bool RPCAuthorized(const std::string& strAuth, std::string& strAuthUsernameOut)
{
    if (!strAuth.starts_with("Basic "))
        return false;
    std::string_view strUserPass64 = TrimStringView(std::string_view{strAuth}.substr(6));
    auto userpass_data = DecodeBase64(strUserPass64);
    std::string strUserPass;
    if (!userpass_data) return false;
    strUserPass.assign(userpass_data->begin(), userpass_data->end());

    size_t colon_pos = strUserPass.find(':');
    if (colon_pos == std::string::npos) {
        return false; // Invalid basic auth.
    }
    std::string user = strUserPass.substr(0, colon_pos);
    std::string pass = strUserPass.substr(colon_pos + 1);
    strAuthUsernameOut = user;
    return CheckUserAuthorized(user, pass);
}

/**
 * Check whether -rpcwhitelist lets `user` call `method`.
 *
 * Returns the reason the call is refused, after logging it, or std::nullopt if
 * the call is allowed.
 */
static std::optional<std::string> WhitelistDenial(const std::string& user, const std::string& method)
{
    const auto whitelist{g_rpc_whitelist.find(user)};
    if (whitelist == g_rpc_whitelist.end()) {
        if (!g_rpc_whitelist_default) return std::nullopt;
        LogWarning("RPC User %s not allowed to call any methods", user);
        return "User is not permitted to call any methods";
    }
    if (whitelist->second.contains(method)) return std::nullopt;
    LogWarning("RPC User %s not allowed to call method %s", user, method);
    return strprintf("User is not permitted to call method %s", method);
}

/**
 * Handle a JSON-RPC batch: parse each element into its own request, call
 * `handle` with each request that parses and its index in the batch, and
 * collect the replies, leaving out notifications.
 *
 * Batches never return HTTP errors. Every error, including an element's parse
 * error, goes in the reply array, which is sent with HTTP OK.
 */
static UniValue ProcessBatch(const UniValue& batch, const JSONRPCClient& client, HTTPStatusCode& status,
                             const std::function<UniValue(const JSONRPCRequest&, size_t)>& handle)
{
    status = HTTP_OK;
    UniValue reply{UniValue::VARR};
    for (size_t i{0}; i < batch.size(); ++i) {
        UniValue response;
        // Each element gets its own request, so that nothing parsed from one
        // element ends up in the reply to the next
        JSONRPCRequest req{client};
        try {
            req.parse(batch[i]);
            response = handle(req, i);
        } catch (UniValue& e) {
            response = JSONRPCReplyObj(NullUniValue, std::move(e), req.id, req.m_json_version);
        } catch (const std::exception& e) {
            response = JSONRPCReplyObj(NullUniValue, JSONRPCError(RPC_PARSE_ERROR, e.what()), req.id, req.m_json_version);
        }
        if (!req.IsNotification()) {
            reply.push_back(std::move(response));
        }
    }
    // Return no response for an all-notification batch, but only if the
    // batch request is non-empty. Technically according to the JSON-RPC
    // 2.0 spec, an empty batch request should also return no response,
    // However, if the batch request is empty, it means the request did
    // not contain any JSON-RPC version numbers, so returning an empty
    // response could break backwards compatibility with old RPC clients
    // relying on previous behavior. Return an empty array instead of an
    // empty response in this case to favor being backwards compatible
    // over complying with the JSON-RPC 2.0 spec in this case.
    if (reply.empty() && !batch.empty()) {
        status = HTTP_NO_CONTENT;
        return {};
    }
    return reply;
}

//! Maximum number of denied methods named in the error for a request held back
//! by a refused batch.
static constexpr size_t MAX_NAMED_DENIED_METHODS{3};
//! Longest method name quoted in that error. RPC method names are much shorter.
static constexpr size_t MAX_NAMED_METHOD_LENGTH{64};

/**
 * Build the reply for a batch rejected by -rpcwhitelist.
 *
 * A batch containing a method the caller may not use is refused as a whole,
 * before any of its requests run, so that a refused batch has no partial
 * effect. Report that per request rather than as one error for the batch:
 * `denied[i]` holds the reason request i was refused, or is empty if the
 * request was only held back by another one. Batch clients can then read the
 * reply in the shape they already expect and show a reason against each
 * request.
 */
static UniValue WhitelistErrorBatch(const UniValue& valRequest, const std::vector<std::string>& denied, const JSONRPCClient& client, HTTPStatusCode& status)
{
    // Name the denied methods in the errors for the held-back requests too,
    // because clients like bitcoin-cli -getinfo show only the first error in
    // a batch. Name only a few short ones: the message is repeated in every
    // held-back request's error, so listing every denied method, or a long
    // method name, would let one batch ask for a reply that grows with the
    // product of its denied and held-back requests.
    std::vector<std::string> named;
    std::unordered_set<std::string_view> seen;
    for (size_t i{0}; i < valRequest.size(); ++i) {
        if (denied[i].empty()) continue;
        const std::string& method{valRequest[i].find_value("method").get_str()};
        if (!seen.insert(method).second) continue;
        if (named.size() < MAX_NAMED_DENIED_METHODS && method.size() <= MAX_NAMED_METHOD_LENGTH) named.push_back(method);
    }
    std::string methods{util::Join(named, ", ")};
    const size_t unnamed{seen.size() - named.size()};
    if (unnamed > 0 && !named.empty()) {
        methods += strprintf(" and %d more", unnamed);
    } else if (unnamed > 0) {
        methods = unnamed == 1 ? "1 method with a long name" : strprintf("%d methods with long names", unnamed);
    }
    const std::string not_executed{strprintf("Not executed because the batch includes methods the user is not permitted to call: %s", methods)};

    // Go through the batch like the execution loop, so malformed requests get
    // their parse errors and notifications get no reply, but answer each
    // request that parses with an error instead of running it.
    return ProcessBatch(valRequest, client, status, [&](const JSONRPCRequest& req, size_t i) {
        UniValue error{denied[i].empty() ? JSONRPCError(RPC_REQUEST_NOT_EXECUTED, not_executed) :
                                           JSONRPCError(RPC_METHOD_NOT_PERMITTED, denied[i])};
        return JSONRPCReplyObj(NullUniValue, std::move(error), req.id, req.m_json_version);
    });
}

UniValue ExecuteHTTPRPC(const UniValue& valRequest, const JSONRPCClient& client, HTTPStatusCode& status)
{
    status = HTTP_OK;
    // Only parsed for a single request. Used for its id and version in error
    // replies, which stay unset for a batch or a body that is not an object.
    JSONRPCRequest jreq{client};
    try {
        // singleton request
        if (valRequest.isObject()) {
            jreq.parse(valRequest);

            // Legacy 1.0/1.1 behavior is for failed requests to throw
            // exceptions which return HTTP errors and RPC errors to the client.
            // 2.0 behavior is to catch exceptions and return HTTP success with
            // RPC errors, as long as there is not an actual HTTP server error.
            const bool catch_errors{jreq.m_json_version == JSONRPCVersion::V2};
            UniValue reply;
            if (auto denial{WhitelistDenial(client.authUser, jreq.strMethod)}) {
                // Report a refused method like an error thrown by the method:
                // legacy requests throw, so the catch below sets the HTTP status
                // (403), and 2.0 requests get the error in the reply.
                UniValue error{JSONRPCError(RPC_METHOD_NOT_PERMITTED, *denial)};
                if (!catch_errors) throw error;
                reply = JSONRPCReplyObj(NullUniValue, std::move(error), jreq.id, jreq.m_json_version);
            } else {
                reply = JSONRPCExec(jreq, catch_errors);
            }
            if (jreq.IsNotification()) {
                // Even though we do execute notifications, we do not respond to them
                status = HTTP_NO_CONTENT;
                return {};
            }
            return reply;
        // array of requests
        } else if (valRequest.isArray()) {
            // Check authorization for every request's method before running
            // any of them, so a batch holding a method the user may not call
            // has no effect at all. Collect all the denials rather than
            // stopping at the first, so one reply can name every method the
            // user needs to remove.
            std::vector<std::string> denied(valRequest.size());
            bool any_denied{false};
            for (size_t i{0}; i < valRequest.size(); ++i) {
                // Leave malformed requests to the execution loop below, which
                // reports them per request. They cannot slip past this check,
                // because parsing them there fails before the method would run.
                const UniValue& request{valRequest[i]};
                if (!request.isObject()) continue;
                const UniValue& method{request.find_value("method")};
                if (!method.isStr()) continue;
                if (auto denial{WhitelistDenial(client.authUser, method.get_str())}) {
                    denied[i] = std::move(*denial);
                    any_denied = true;
                }
            }
            if (any_denied) return WhitelistErrorBatch(valRequest, denied, client, status);

            return ProcessBatch(valRequest, client, status, [](const JSONRPCRequest& req, size_t) {
                return JSONRPCExec(req, /*catch_errors=*/true);
            });
        }
        else
            throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");
    } catch (UniValue& e) {
        return JSONErrorReply(std::move(e), jreq, status);
    } catch (const std::exception& e) {
        return JSONErrorReply(JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq, status);
    }
}

static void HTTPReq_JSONRPC(const std::any& context, HTTPRequest* req)
{
    // JSONRPC handles only POST
    if (req->GetRequestMethod() != HTTPRequestMethod::POST) {
        req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
        return;
    }
    // Check authorization
    std::optional<std::string> auth_header = req->GetHeader("authorization");
    if (!auth_header) {
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return;
    }

    JSONRPCClient client;
    client.context = context;
    client.peerAddr = req->GetPeer().ToStringAddrPort();
    client.URI = req->GetURI();
    if (!RPCAuthorized(*auth_header, client.authUser)) {
        LogWarning("ThreadRPCServer incorrect password attempt from %s", client.peerAddr);

        /* Deter brute-forcing
           If this results in a DoS the user really
           shouldn't have their RPC port exposed. */
        UninterruptibleSleep(std::chrono::milliseconds{250});

        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return;
    }

    // Generate reply
    HTTPStatusCode status;
    UniValue reply;
    UniValue request;
    if (request.read(req->ReadBody())) {
        reply = ExecuteHTTPRPC(request, client, status);
    } else {
        reply = JSONErrorReply(JSONRPCError(RPC_PARSE_ERROR, "Parse error"), JSONRPCRequest{client}, status);
    }

    // Write reply
    if (reply.isNull()) {
        // Error case or no-content notification reply.
        req->WriteReply(status);
    } else {
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(status, reply.write() + "\n");
    }
}

static bool InitRPCAuthentication()
{
    std::string user;
    std::string pass;

    if (gArgs.GetArg("-rpcpassword", "") == "")
    {
        std::optional<fs::perms> cookie_perms{std::nullopt};
        auto cookie_perms_arg{gArgs.GetArg("-rpccookieperms")};
        if (cookie_perms_arg) {
            auto perm_opt = InterpretPermString(*cookie_perms_arg);
            if (!perm_opt) {
                LogError("Invalid -rpccookieperms=%s; must be one of 'owner', 'group', or 'all'.", *cookie_perms_arg);
                return false;
            }
            cookie_perms = *perm_opt;
        }

        switch (GenerateAuthCookie(cookie_perms, user, pass)) {
        case AuthCookieResult::Error:
            return false;
        case AuthCookieResult::Disabled:
            LogInfo("RPC authentication cookie file generation is disabled.");
            break;
        case AuthCookieResult::Ok:
            LogInfo("Using random cookie authentication.");
            break;
        }
    } else {
        LogInfo("Using rpcuser/rpcpassword authentication.");
        LogWarning("The use of rpcuser/rpcpassword is less secure, because credentials are configured in plain text. It is recommended that locally-run instances switch to cookie-based auth, or otherwise to use hashed rpcauth credentials. See share/rpcauth in the source directory for more information.");
        user = gArgs.GetArg("-rpcuser", "");
        pass = gArgs.GetArg("-rpcpassword", "");
    }

    // If there is a plaintext credential, hash it with a random salt before storage.
    if (!user.empty() || !pass.empty()) {
        // Generate a random 16 byte hex salt.
        std::array<unsigned char, 16> raw_salt;
        GetStrongRandBytes(raw_salt);
        std::string salt = HexStr(raw_salt);

        // Compute HMAC.
        std::array<unsigned char, CHMAC_SHA256::OUTPUT_SIZE> out;
        CHMAC_SHA256(UCharCast(salt.data()), salt.size()).Write(UCharCast(pass.data()), pass.size()).Finalize(out.data());
        std::string hash = HexStr(out);

        g_rpcauth.push_back({user, salt, hash});
    }

    if (!gArgs.GetArgs("-rpcauth").empty()) {
        LogInfo("Using rpcauth authentication.\n");
        for (const std::string& rpcauth : gArgs.GetArgs("-rpcauth")) {
            std::vector<std::string> fields{SplitString(rpcauth, ':')};
            const std::vector<std::string> salt_hmac{SplitString(fields.back(), '$')};
            if (fields.size() == 2 && salt_hmac.size() == 2) {
                fields.pop_back();
                fields.insert(fields.end(), salt_hmac.begin(), salt_hmac.end());
                g_rpcauth.push_back(fields);
            } else {
                LogWarning("Invalid -rpcauth argument.");
                return false;
            }
        }
    }

    g_rpc_whitelist_default = gArgs.GetBoolArg("-rpcwhitelistdefault", !gArgs.GetArgs("-rpcwhitelist").empty());
    for (const std::string& strRPCWhitelist : gArgs.GetArgs("-rpcwhitelist")) {
        auto pos = strRPCWhitelist.find(':');
        std::string strUser = strRPCWhitelist.substr(0, pos);
        bool intersect = g_rpc_whitelist.contains(strUser);
        std::set<std::string>& whitelist = g_rpc_whitelist[strUser];
        if (pos != std::string::npos) {
            std::string strWhitelist = strRPCWhitelist.substr(pos + 1);
            std::vector<std::string> whitelist_split = SplitString(strWhitelist, ", ");
            std::set<std::string> new_whitelist{
                std::make_move_iterator(whitelist_split.begin()),
                std::make_move_iterator(whitelist_split.end())};
            if (intersect) {
                std::set<std::string> tmp_whitelist;
                std::set_intersection(new_whitelist.begin(), new_whitelist.end(),
                       whitelist.begin(), whitelist.end(), std::inserter(tmp_whitelist, tmp_whitelist.end()));
                new_whitelist = std::move(tmp_whitelist);
            }
            whitelist = std::move(new_whitelist);
        }
    }

    return true;
}

bool StartHTTPRPC(const std::any& context)
{
    LogDebug(BCLog::RPC, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;

    auto handle_rpc = [context](HTTPRequest* req, const std::string&) { return HTTPReq_JSONRPC(context, req); };
    RegisterHTTPHandler("/", true, handle_rpc);
    if (g_wallet_init_interface.HasWalletSupport()) {
        RegisterHTTPHandler("/wallet/", false, handle_rpc);
    }
    return true;
}

void InterruptHTTPRPC()
{
    LogDebug(BCLog::RPC, "Interrupting HTTP RPC server\n");
}

void StopHTTPRPC()
{
    LogDebug(BCLog::RPC, "Stopping HTTP RPC server\n");
    UnregisterHTTPHandler("/", true);
    if (g_wallet_init_interface.HasWalletSupport()) {
        UnregisterHTTPHandler("/wallet/", false);
    }
}
