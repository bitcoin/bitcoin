// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"

#include "validation.h"
#include "txdb.h"


static bool IsDigits(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), ::isdigit);
};

UniValue anonoutput(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "anonoutput [index/hex]\n"
            "\n");

    UniValue result(UniValue::VOBJ);

    if (request.params.size() == 0)
    {
        int64_t nLastRCTOutIndex;
        if (!pblocktree->ReadLastRCTOutput(nLastRCTOutIndex))
            nLastRCTOutIndex = 0;
        result.push_back(Pair("lastindex", (int)nLastRCTOutIndex));
        return result;
    };

    std::string sIn = request.params[0].get_str();

    int64_t nIndex;
    if (IsDigits(sIn))
    {
        errno = 0;
        nIndex = strtoll(sIn.c_str(), nullptr, 10);
        if (errno)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid index");
    } else
    {
        if (!IsHex(sIn))
            throw JSONRPCError(RPC_INVALID_PARAMETER, sIn+" is not a hexadecimal or decimal string.");
        std::vector<uint8_t> vIn = ParseHex(sIn);

        CCmpPubKey pk(vIn.begin(), vIn.end());

        if (!pk.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, sIn+" is not a valid compressed public key.");

        if (!pblocktree->ReadRCTOutputLink(pk, nIndex))
            throw JSONRPCError(RPC_MISC_ERROR, "Output not indexed.");
    };

    CAnonOutput ao;
    if (!pblocktree->ReadRCTOutput(nIndex, ao))
        throw JSONRPCError(RPC_MISC_ERROR, "Unknown index.");

    result.pushKV("index", (int)nIndex);
    result.pushKV("publickey", HexStr(ao.pubkey.begin(), ao.pubkey.end()));
    result.pushKV("txnhash", HexStr(ao.outpoint.hash.begin(), ao.outpoint.hash.end()));
    result.pushKV("n", (int)ao.outpoint.n);
    result.pushKV("blockheight", ao.nBlockHeight);

    return result;
};

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "anon",               "anonoutput",             &anonoutput,             true, {} },
};

void RegisterAnonRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
