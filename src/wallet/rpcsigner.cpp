// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <util/strencodings.h>
#include <wallet/rpcsigner.h>
#include <wallet/wallet.h>

#ifdef ENABLE_EXTERNAL_SIGNER

// CRPCCommand table won't compile with an empty array
static UniValue dummy(const JSONRPCRequest& request) {
    return NullUniValue;
}


Span<const CRPCCommand> GetSignerRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------
    { "signer",    "dummy",               &dummy,            {} },
};
// clang-format on
    return MakeSpan(commands);
}


#endif // ENABLE_EXTERNAL_SIGNER
