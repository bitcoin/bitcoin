// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/request.h>

#include <common/args.h>
#include <logging.h>
#include <random.h>
#include <rpc/protocol.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/strencodings.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * JSON-RPC protocol.  Bitcoin speaks version 1.0 for maximum compatibility,
 * but uses JSON-RPC 1.1/2.0 standards for parts of the 1.0 standard that were
 * unspecified (HTTP errors and contents of 'error').
 *
 * 1.0 spec: https://www.jsonrpc.org/specification_v1
 * 1.2 spec: https://jsonrpc.org/historical/json-rpc-over-http.html
 *
 * If the server receives a request with the JSON-RPC 2.0 marker `{"jsonrpc": "2.0"}`
 * then Bitcoin will respond with a strictly specified response.
 * It will only return an HTTP error code if an actual HTTP error is encountered
 * such as the endpoint is not found (404) or the request is not formatted correctly (500).
 * Otherwise the HTTP code is always OK (200) and RPC errors will be included in the
 * response body.
 *
 * 2.0 spec: https://www.jsonrpc.org/specification
 *
 * Also see https://www.simple-is-better.org/rpc/#differences-between-1-0-and-2-0
 */

UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id)
{
    UniValue request(UniValue::VOBJ);
    request.pushKV("method", strMethod);
    request.pushKV("params", params);
    request.pushKV("id", id);
    request.pushKV("jsonrpc", "2.0");
    return request;
}

UniValue JSONRPCReplyObj(UniValue result, UniValue error, std::optional<UniValue> id, JSONRPCVersion jsonrpc_version)
{
    UniValue reply(UniValue::VOBJ);
    // Add JSON-RPC version number field in v2 only.
    if (jsonrpc_version == JSONRPCVersion::V2) reply.pushKV("jsonrpc", "2.0");

    // Add both result and error fields in v1, even though one will be null.
    // Omit the null field in v2.
    if (error.isNull()) {
        reply.pushKV("result", std::move(result));
        if (jsonrpc_version == JSONRPCVersion::V1_LEGACY) reply.pushKV("error", NullUniValue);
    } else {
        if (jsonrpc_version == JSONRPCVersion::V1_LEGACY) reply.pushKV("result", NullUniValue);
        reply.pushKV("error", std::move(error));
    }
    if (id.has_value()) reply.pushKV("id", std::move(id.value()));
    return reply;
}

UniValue JSONRPCError(int code, const std::string& message)
{
    UniValue error(UniValue::VOBJ);
    error.pushKV("code", code);
    error.pushKV("message", message);
    return error;
}

/** Username used when cookie authentication is in use (arbitrary, only for
 * recognizability in debugging/logging purposes)
 */
static const std::string COOKIEAUTH_USER = "__cookie__";
/** Default name for auth cookie file */
static const char* const COOKIEAUTH_FILE = ".cookie";

/** Get name of RPC authentication cookie file */
static fs::path GetAuthCookieFile(bool temp=false)
{
    fs::path arg = gArgs.GetPathArg("-rpccookiefile", COOKIEAUTH_FILE);
    if (arg.empty()) {
        return {}; // -norpccookiefile was specified
    }
    if (temp) {
        arg += ".tmp";
    }
    return AbsPathForConfigVal(gArgs, arg);
}

static bool g_generated_cookie = false;

GenerateAuthCookieResult GenerateAuthCookie(const std::optional<fs::perms>& cookie_perms,
                                            std::string& user,
                                            std::string& pass)
{
    const size_t COOKIE_SIZE = 32;
    unsigned char rand_pwd[COOKIE_SIZE];
    GetRandBytes(rand_pwd);
    const std::string rand_pwd_hex{HexStr(rand_pwd)};

    /** the umask determines what permissions are used to create this file -
     * these are set to 0077 in common/system.cpp.
     */
    std::ofstream file;
    fs::path filepath_tmp = GetAuthCookieFile(true);
    if (filepath_tmp.empty()) {
        return GenerateAuthCookieResult::DISABLED; // -norpccookiefile
    }
    file.open(filepath_tmp.std_path());
    if (!file.is_open()) {
        LogWarning("Unable to open cookie authentication file %s for writing", fs::PathToString(filepath_tmp));
        return GenerateAuthCookieResult::ERR;
    }
    file << COOKIEAUTH_USER << ":" << rand_pwd_hex;
    file.close();

    fs::path filepath = GetAuthCookieFile(false);
    if (!RenameOver(filepath_tmp, filepath)) {
        LogWarning("Unable to rename cookie authentication file %s to %s", fs::PathToString(filepath_tmp), fs::PathToString(filepath));
        return GenerateAuthCookieResult::ERR;
    }
    if (cookie_perms) {
        std::error_code code;
        fs::permissions(filepath, cookie_perms.value(), fs::perm_options::replace, code);
        if (code) {
            LogWarning("Unable to set permissions on cookie authentication file %s", fs::PathToString(filepath));
            return GenerateAuthCookieResult::ERR;
        }
    }

    g_generated_cookie = true;
    LogInfo("Generated RPC authentication cookie %s\n", fs::PathToString(filepath));
    LogInfo("Permissions used for cookie: %s\n", PermsToSymbolicString(fs::status(filepath).permissions()));

    user = COOKIEAUTH_USER;
    pass = rand_pwd_hex;
    return GenerateAuthCookieResult::OK;
}

bool GetAuthCookie(std::string *cookie_out)
{
    std::ifstream file;
    std::string cookie;
    fs::path filepath = GetAuthCookieFile();
    if (filepath.empty()) {
        return true; // -norpccookiefile
    }
    file.open(filepath.std_path());
    if (!file.is_open())
        return false;
    std::getline(file, cookie);
    file.close();

    if (cookie_out)
        *cookie_out = cookie;
    return true;
}

void DeleteAuthCookie()
{
    try {
        if (g_generated_cookie) {
            // Delete the cookie file if it was generated by this process
            fs::remove(GetAuthCookieFile());
        }
    } catch (const fs::filesystem_error& e) {
        LogWarning("Unable to remove random auth cookie file %s: %s\n", fs::PathToString(e.path1()), e.code().message());
    }
}

std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in)
{
    if (!in.isArray()) {
        throw std::runtime_error("Batch must be an array");
    }
    const size_t num {in.size()};
    std::vector<UniValue> batch(num);
    for (const UniValue& rec : in.getValues()) {
        if (!rec.isObject()) {
            throw std::runtime_error("Batch member must be an object");
        }
        size_t id = rec["id"].getInt<int>();
        if (id >= num) {
            throw std::runtime_error("Batch member id is larger than batch size");
        }
        batch[id] = rec;
    }
    return batch;
}

void JSONRPCRequest::parse(const UniValue& valRequest)
{
    // Parse request
    if (!valRequest.isObject())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");
    const UniValue& request = valRequest.get_obj();

    // Parse id now so errors from here on will have the id
    if (request.exists("id")) {
        id = request.find_value("id");
    } else {
        id = std::nullopt;
    }

    // Check for JSON-RPC 2.0 (default 1.1)
    m_json_version = JSONRPCVersion::V1_LEGACY;
    const UniValue& jsonrpc_version = request.find_value("jsonrpc");
    if (!jsonrpc_version.isNull()) {
        if (!jsonrpc_version.isStr()) {
            throw JSONRPCError(RPC_INVALID_REQUEST, "jsonrpc field must be a string");
        }
        // The "jsonrpc" key was added in the 2.0 spec, but some older documentation
        // incorrectly included {"jsonrpc":"1.0"} in a request object, so we
        // maintain that for backwards compatibility.
        if (jsonrpc_version.get_str() == "1.0") {
            m_json_version = JSONRPCVersion::V1_LEGACY;
        } else if (jsonrpc_version.get_str() == "2.0") {
            m_json_version = JSONRPCVersion::V2;
        } else {
            throw JSONRPCError(RPC_INVALID_REQUEST, "JSON-RPC version not supported");
        }
    }

    // Parse method
    const UniValue& valMethod{request.find_value("method")};
    if (valMethod.isNull())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");
    if (!valMethod.isStr())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");
    strMethod = valMethod.get_str();
    if (fLogIPs)
        LogDebug(BCLog::RPC, "ThreadRPCServer method=%s user=%s peeraddr=%s\n", SanitizeString(strMethod),
            this->authUser, this->peerAddr);
    else
        LogDebug(BCLog::RPC, "ThreadRPCServer method=%s user=%s\n", SanitizeString(strMethod), this->authUser);

    // Parse params
    const UniValue& valParams{request.find_value("params")};
    if (valParams.isArray() || valParams.isObject())
        params = valParams;
    else if (valParams.isNull())
        params = UniValue(UniValue::VARR);
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array or object");
}
