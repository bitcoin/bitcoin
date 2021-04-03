// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

static RPCHelpMan masternode_outputs()
{
    return RPCHelpMan{"masternode_outputs",
        "\nPrint masternode compatible outputs\n",
        {              
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_outputs", "")
            + HelpExampleRpc("masternode_outputs", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;


    // Find possible candidates
    std::vector<COutput> vPossibleCoins;
    {
        LOCK(pwallet->cs_wallet);
        pwallet->AvailableCoins(vPossibleCoins, true, nullptr, nMNCollateralRequired, nMNCollateralRequired, MAX_MONEY, 0, true);
    }

    UniValue obj(UniValue::VOBJ);
    for (const auto& out : vPossibleCoins) {
        obj.pushKV(out.tx->GetHash().ToString(), strprintf("%d", out.i));
    }

    return obj;
},
    };
} 



Span<const CRPCCommand> GetMasternodeWalletRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)        
//  --------------------- ------------------------  ----------------------- 
    { "masternode",           &masternode_outputs,      },

};
// clang-format on
    return MakeSpan(commands);
}