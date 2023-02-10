// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <wallet/rpc/util.h>
#include <wallet/spend.h>
#include <wallet/rpc/wallet.h>
using namespace wallet;
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
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<wallet::CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;


    // Find possible candidates
    std::vector<wallet::COutput> vPossibleCoins;
    {
        LOCK(pwallet->cs_wallet);
        CoinFilterParams coins_params;
        coins_params.min_amount = nMNCollateralRequired;
        coins_params.max_amount = nMNCollateralRequired;
        coins_params.skip_locked = false;
        vPossibleCoins = AvailableCoins(*pwallet, nullptr, /*feerate=*/ std::nullopt, coins_params).All();
    }

    UniValue objArr(UniValue::VARR);
    for (const auto& out : vPossibleCoins) {
        UniValue outObj(UniValue::VOBJ);
        outObj.pushKV("txid", out.outpoint.hash.ToString());  
        outObj.pushKV("index", out.outpoint.n);
        objArr.push_back(outObj);
    }

    return objArr;
},
    };
} 



Span<const CRPCCommand> wallet::GetMasternodeWalletRPCCommands()
{
    static const CRPCCommand commands[]{
        {"masternode", &masternode_outputs},
    };
    return commands;
}