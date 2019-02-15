// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <util/strencodings.h>
#include <wallet/rpcsigner.h>
#include <wallet/wallet.h>

#ifdef ENABLE_EXTERNAL_SIGNER

// CRPCCommand table won't compile with an empty array
static RPCHelpMan dummy()
{
    return RPCHelpMan{"dummy",
                "\nDoes nothing.\n"
                "",
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return NullUniValue;
},
    };
}

Span<const CRPCCommand> GetSignerRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ // category              actor (function)
  // --------------------- ------------------------
  { "signer",              &dummy,                },
};
// clang-format on
    return MakeSpan(commands);
}


#endif // ENABLE_EXTERNAL_SIGNER
