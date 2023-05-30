// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/request.h>

#include <util/fs.h>

#include <common/args.h>
#include <logging.h>
#include <random.h>
#include <rpc/protocol.h>
#include <util/check.h>
#include <util/fs_helpers.h>
#include <util/strencodings.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * JSON-RPC protocol.  Bitcoin speaks version 1.0 for maximum compatibility,
 * but uses JSON-RPC 1.1/2.0 standards for parts of the 1.0 standard that were
 * unspecified (HTTP errors and contents of 'error').
 *
 * 1.0 spec: http://json-rpc.org/wiki/specification
 * 1.2 spec: http://jsonrpc.org/historical/json-rpc-over-http.html
 */

UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id)
{
    UniValue request(UniValue::VOBJ);
    request.pushKV("method", strMethod);
    request.pushKV("params", params);
    request.pushKV("id", id);
    return request;
}

UniValue JSONRPCReplyObj(const UniValue& result, const UniValue& error, const UniValue& id)
{
    UniValue reply(UniValue::VOBJ);
    if (!error.isNull())
        reply.pushKV("result", NullUniValue);
    else
        reply.pushKV("result", result);
    reply.pushKV("error", error);
    reply.pushKV("id", id);
    return reply;
}

std::string JSONRPCReply(const UniValue& result, const UniValue& error, const UniValue& id)
{
    UniValue reply = JSONRPCReplyObj(result, error, id);
    return reply.write() + "\n";
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
    if (temp) {
        arg += ".tmp";
    }
    return AbsPathForConfigVal(gArgs, arg);
}

bool GenerateAuthCookie(std::string *cookie_out)
{
    const size_t COOKIE_SIZE = 32;
    unsigned char rand_pwd[COOKIE_SIZE];
    GetRandBytes(rand_pwd);
    std::string cookie = COOKIEAUTH_USER + ":" + HexStr(rand_pwd);

    /** the umask determines what permissions are used to create this file -
     * these are set to 0077 in common/system.cpp.
     */
    std::ofstream file;
    fs::path filepath_tmp = GetAuthCookieFile(true);
    file.open(filepath_tmp);
    if (!file.is_open()) {
        LogPrintf("Unable to open cookie authentication file %s for writing\n", fs::PathToString(filepath_tmp));
        return false;
    }
    file << cookie;
    file.close();

    fs::path filepath = GetAuthCookieFile(false);
    if (!RenameOver(filepath_tmp, filepath)) {
        LogPrintf("Unable to rename cookie authentication file %s to %s\n", fs::PathToString(filepath_tmp), fs::PathToString(filepath));
        return false;
    }
    LogPrintf("Generated RPC authentication cookie %s\n", fs::PathToString(filepath));

    if (cookie_out)
        *cookie_out = cookie;
    return true;
}

bool GetAuthCookie(std::string *cookie_out)
{
    std::ifstream file;
    std::string cookie;
    fs::path filepath = GetAuthCookieFile();
    file.open(filepath);
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
        fs::remove(GetAuthCookieFile());
    } catch (const fs::filesystem_error& e) {
        LogPrintf("%s: Unable to remove random auth cookie file: %s\n", __func__, fsbridge::get_filesystem_error_message(e));
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
    id = request.find_value("id");

    // Parse method
    const UniValue& valMethod{request.find_value("method")};
    if (valMethod.isNull())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");
    if (!valMethod.isStr())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");
    strMethod = valMethod.get_str();
    if (fLogIPs)
        LogPrint(BCLog::RPC, "ThreadRPCServer method=%s user=%s peeraddr=%s\n", SanitizeString(strMethod),
            this->authUser, this->peerAddr);
    else
        LogPrint(BCLog::RPC, "ThreadRPCServer method=%s user=%s\n", SanitizeString(strMethod), this->authUser);

    // Parse params
    const UniValue& valParams{request.find_value("params")};
    if (valParams.isArray() || valParams.isObject())
        params.received = valParams;
    else if (valParams.isNull())
        params.received = UniValue(UniValue::VARR);
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array or object");
}

const UniValue& JSONRPCRequest::JSONRPCParameters::operator[](const std::string& key) const
{
    const auto it = named.find(key);
    if (it == named.end()) return NullUniValue;
    return *it->second;
}

const UniValue& JSONRPCRequest::JSONRPCParameters::operator[](size_t pos) const
{
    if (pos >= positional.size()) return NullUniValue;
    const UniValue* item = positional.at(pos);
    return item ? *item : NullUniValue;
}

size_t JSONRPCRequest::JSONRPCParameters::size() const
{
    return positional.size();
}

UniValue JSONRPCRequest::JSONRPCParameters::AsUniValueArray() const
{
    UniValue out{UniValue::VARR};
    for (size_t i = 0; i < size(); ++i) {
        out.push_back((*this)[i]);
    }
    return out;
}

void JSONRPCRequest::JSONRPCParameters::ProcessParameters(const std::vector<std::pair<std::string, bool>>& argNames)
{
    positional.clear();
    positional.reserve(argNames.size());
    named.clear();

    // Build a map of parameters, and remove ones that have been processed, so that we can throw a focused error if
    // there is an unknown one.
    std::unordered_map<std::string, const UniValue*> named_args;
    if (received.isObject()) {
        const std::vector<std::string>& keys = received.getKeys();
        const std::vector<UniValue>& values = received.getValues();
        for (size_t i = 0; i < keys.size(); ++i) {
            auto [_, inserted] = named_args.emplace(keys[i], &values[i]);
            if (!inserted) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s specified multiple times", keys[i]));
            }
        }
    }

    // Determine where are positional args are. They could be the received params, or in an "args" parameter
    const UniValue* positional_args = nullptr;
    if (received.isObject() && named_args.count("args") > 0) {
        positional_args = named_args.extract("args").mapped();
    } else if (received.isArray()) {
        positional_args = &received;
    }

    // Process expected parameters. If any parameters were left unspecified in
    // the request before a parameter that was specified, null values need to be
    // inserted at the unspecifed parameter positions.
    size_t pos = 0;
    UniValue* options = nullptr;
    for (const auto& [name_pattern, named_only] : argNames) {
        std::vector<std::string> aliases = SplitString(name_pattern, '|');

        // Search for this parameter in our named args
        auto named_it = named_args.end();
        for (const std::string& alias : aliases) {
            named_it = named_args.find(alias);
            if (named_it != named_args.end()) break;
        }
        const UniValue* arg = nullptr;
        bool found_named = named_it != named_args.end();
        if (found_named) {
            arg = named_it->second;
        }

        // Handle named-only parameters by pushing them into a temporary options
        // object, and then pushing the accumulated options as the next
        // positional argument.
        if (named_only) {
            if (found_named) {
                if (!options) {
                    constructed.emplace_back(UniValue::VOBJ);
                    options = &constructed.back();
                }
                if (options->exists(named_it->first)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s specified multiple times", named_it->first));
                }
                options->pushKVEnd(named_it->first, *arg);
                named_args.erase(named_it);
            }
            continue;
        }

        // Search for this parameter in our positional args
        if (positional_args && pos < positional_args->size()) {
            // Cannot be both positional and named
            if (arg) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s specified twice both as positional and named argument", named_it->first));
            }
            arg = &(*positional_args)[pos];
        }
        ++pos;

        // Insert the parameter into our mappings
        if (options) {
            // Non-empty options means that we've parsed all of the named-only parameters
            // that belong to this options object parameter
            positional.emplace_back(options);
            for (const std::string& alias : aliases) {
                auto [_, inserted] = named.emplace(alias, options);
                if (!inserted) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s specified multiple times", alias));
                }
            }
            // The current parameter is the options object. There should not be any parameter for it.
            if (arg) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s conflicts with parameter %s", aliases.front(), options->getKeys().front()));
            }
            options = nullptr;
        } else if (arg) {
            // Normal parameter, specified once
            positional.emplace_back(arg);
            for (const std::string& alias : aliases) {
                auto [_, inserted] = named.emplace(alias, arg);
                if (!inserted) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parameter %s specified multiple times", alias));
                }
            }
            if (found_named) {
                named_args.erase(named_it);
            }
        } else {
            // Not found, insert a nullptr into positionals, and do nothing for named
            positional.emplace_back(nullptr);
        }
    }

    // If there are still arguments in named_args, this is an error.
    if (!named_args.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown named parameter %s", named_args.begin()->first));
    }

    // If there are any leftover positional args, stick them onto the end of "positional"
    // This can only happen if no named arguments were given.
    for (; pos < (positional_args ? positional_args->size() : 0) ; ++pos) {
        positional.emplace_back(&(*positional_args)[pos]);
    }

    // Trim trailing nullptrs
    positional.erase(std::find_if(positional.rbegin(), positional.rend(), [](const UniValue* ptr) {
        return ptr;
    }).base(), positional.end());
}
