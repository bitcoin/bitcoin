// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REQUEST_H
#define BITCOIN_RPC_REQUEST_H

#include <any>
#include <optional>
#include <string>

#include <univalue.h>
#include <util/fs.h>

enum class JSONRPCVersion {
    V1_LEGACY,
    V2
};

/** JSON-RPC 2.0 request, only used in bitcoin-cli **/
UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id);
UniValue JSONRPCReplyObj(UniValue result, UniValue error, std::optional<UniValue> id, JSONRPCVersion jsonrpc_version);
UniValue JSONRPCError(int code, const std::string& message);

enum class GenerateAuthCookieResult : uint8_t {
    DISABLED, // -norpccookiefile
    ERR,
    OK,
};

/**
 * Generate a new RPC authentication cookie and write it to disk
 * @param[in] cookie_perms Filesystem permissions to use for the cookie file.
 * @param[out] user Generated username, only set if `OK` is returned.
 * @param[out] pass Generated password, only set if `OK` is returned.
 * @retval GenerateAuthCookieResult::DISABLED Authentication via cookie is disabled.
 * @retval GenerateAuthCookieResult::ERROR Error occurred, auth data could not be saved to disk.
 * @retval GenerateAuthCookieResult::OK Auth data was generated, saved to disk and in `user` and `pass`.
 */
GenerateAuthCookieResult GenerateAuthCookie(const std::optional<fs::perms>& cookie_perms,
                                            std::string& user,
                                            std::string& pass);

/** Read the RPC authentication cookie from disk */
bool GetAuthCookie(std::string *cookie_out);
/** Delete RPC authentication cookie from disk */
void DeleteAuthCookie();
/** Parse JSON-RPC batch reply into a vector */
std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in);

class JSONRPCRequest
{
public:
    std::optional<UniValue> id = UniValue::VNULL;
    std::string strMethod;
    UniValue params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    std::any context;
    JSONRPCVersion m_json_version = JSONRPCVersion::V1_LEGACY;

    void parse(const UniValue& valRequest);
    [[nodiscard]] bool IsNotification() const { return !id.has_value() && m_json_version == JSONRPCVersion::V2; };
};

#endif // BITCOIN_RPC_REQUEST_H
