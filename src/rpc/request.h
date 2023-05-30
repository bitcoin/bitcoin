// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REQUEST_H
#define BITCOIN_RPC_REQUEST_H

#include <any>
#include <string>
#include <unordered_map>

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
    class JSONRPCParameters
    {
    public:
        /** The parameters as received */
        UniValue received;
        /** When processing the parameters, we may construct new UniValue objects which we will store here */
        std::vector<UniValue> constructed;
        /** Parameter name to parameter. Only provided parameters will be present. */
        std::unordered_map<std::string, const UniValue*> named;
        /** Parameter position to parameter.
         * Parameters not provided at all (neither named nor positional) will be stored as nullptr,
         * with the exception of trailing parameters (i.e. this vector never ends with nullptrs)
         * to preserve backwards compatibility that act based on the number of specified parameters. */
        std::vector<const UniValue*> positional;

        /** Process the received parameters into named and positional mappings */
        void ProcessParameters(const std::vector<std::pair<std::string, bool>>& argNames);

        /** Retrieve a parameter by its name. Unknown parameters are returned as NullUniValue. */
        const UniValue& operator[](const std::string& key) const;
        /** Retrieve a parameter by its position. Out of bound parameters are returned as NullUniValue */
        const UniValue& operator[](size_t pos) const;

        /** The number of parameters received */
        size_t size() const;

        /** Produce a UniValue array with each argument in its expected position */
        UniValue AsUniValueArray() const;
    };

    UniValue id;
    std::string strMethod;
    JSONRPCParameters params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    std::any context;

    void parse(const UniValue& valRequest);
};

#endif // BITCOIN_RPC_REQUEST_H
