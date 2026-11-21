// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REQUEST_H
#define BITCOIN_RPC_REQUEST_H

#include <attributes.h>
#include <univalue.h>
#include <util/fs.h>

#include <any>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class JSONRPCVersion {
    V1_LEGACY,
    V2
};

/** JSON-RPC 2.0 request, only used in bitcoin-cli **/
UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id);
UniValue JSONRPCReplyObj(UniValue result, UniValue error, std::optional<UniValue> id, JSONRPCVersion jsonrpc_version);
UniValue JSONRPCError(int code, const std::string& message);

enum class AuthCookieResult : uint8_t {
    Disabled, // -norpccookiefile
    Error,
    Ok,
};

/**
 * Generate a new RPC authentication cookie and write it to disk
 * @param[in] cookie_perms Filesystem permissions to use for the cookie file.
 * @param[out] user Generated username, only set if `OK` is returned.
 * @param[out] pass Generated password, only set if `OK` is returned.
 * @retval AuthCookieResult::Disabled Authentication via cookie is disabled.
 * @retval AuthCookieResult::Error Error occurred, auth data could not be saved to disk.
 * @retval AuthCookieResult::Ok Auth data was generated, saved to disk and in `user` and `pass`.
 */
AuthCookieResult GenerateAuthCookie(const std::optional<fs::perms>& cookie_perms,
                                            std::string& user,
                                            std::string& pass);

/** Read the RPC authentication cookie from disk */
AuthCookieResult GetAuthCookie(std::string& cookie_out);
/** Delete RPC authentication cookie from disk */
void DeleteAuthCookie();
/** Parse JSON-RPC batch reply into a vector */
std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in);

/**
 * Client that sent a JSON-RPC request: the connection-level fields shared by
 * every request in an HTTP body or IPC call, including every element of a
 * batch.
 */
struct JSONRPCClient
{
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    //! Default context for requests from this client.
    std::any context;
};

/**
 * Single JSON-RPC call. Refers to the client that sent it instead of holding
 * a copy, so requests parsed from a batch start out empty and share nothing
 * but the client.
 */
class JSONRPCRequest
{
public:
    //! Request with no client, for calls made internally and in tests.
    JSONRPCRequest();
    explicit JSONRPCRequest(const JSONRPCClient& client LIFETIMEBOUND) : client{client}, context{client.context} {}

    const JSONRPCClient& client;
    std::optional<UniValue> id = UniValue::VNULL;
    std::string strMethod;
    UniValue params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    //! Context passed to the handler. Starts as the client's context, and
    //! dispatchers like the wallet's replace it with their own.
    std::any context;
    JSONRPCVersion m_json_version = JSONRPCVersion::V1_LEGACY;

    void parse(const UniValue& valRequest);
    [[nodiscard]] bool IsNotification() const { return !id.has_value() && m_json_version == JSONRPCVersion::V2; };
};

#endif // BITCOIN_RPC_REQUEST_H
