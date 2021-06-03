// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REQUEST_H
#define BITCOIN_RPC_REQUEST_H

#include <any>
#include <string>

#include <univalue.h>

UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id);
UniValue JSONRPCReplyObj(const UniValue& result, const UniValue& error, const UniValue& id);
std::string JSONRPCReply(const UniValue& result, const UniValue& error, const UniValue& id);
UniValue JSONRPCError(int code, const std::string& message);

/** Generate a new RPC authentication cookie and write it to disk */
bool GenerateAuthCookie(std::string *cookie_out);
/** Read the RPC authentication cookie from disk */
bool GetAuthCookie(std::string *cookie_out);
/** Delete RPC authentication cookie from disk */
void DeleteAuthCookie();
/** Parse JSON-RPC batch reply into a vector */
std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in);

class JSONRPCRequest
{
public:
    UniValue id;
    std::string strMethod;
    UniValue params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    const std::any& context;
    std::string createContractCode;
    std::string nftCommand;

    explicit JSONRPCRequest(const std::any& context) : id(NullUniValue), params(NullUniValue), context(context) {}

    //! Initializes request information from another request object and the
    //! given context. The implementation should be updated if any members are
    //! added or removed above.
    JSONRPCRequest(const JSONRPCRequest& other, const std::any& context)
        : id(other.id), strMethod(other.strMethod), params(other.params), mode(other.mode), URI(other.URI),
          authUser(other.authUser), peerAddr(other.peerAddr), context(context), createContractCode(other.createContractCode), nftCommand(other.nftCommand)
    {
    }

    void parse(const UniValue& valRequest);
};

#endif // BITCOIN_RPC_REQUEST_H
