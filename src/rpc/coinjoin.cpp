// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#ifdef ENABLE_WALLET
#include <coinjoin/coinjoin-client.h>
#include <coinjoin/coinjoin-client-options.h>
#include <wallet/rpcwallet.h>
#endif // ENABLE_WALLET
#include <coinjoin/coinjoin-server.h>
#include <rpc/server.h>
#include <utilstrencodings.h>

#include <univalue.h>

#ifdef ENABLE_WALLET
UniValue coinjoin(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "coinjoin \"command\"\n"
            "\nArguments:\n"
            "1. \"command\"        (string or set of strings, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  start       - Start mixing\n"
            "  stop        - Stop mixing\n"
            "  reset       - Reset mixing\n"
        );

    if (fMasternodeMode)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Client-side mixing is not supported on masternodes");

    if (!CCoinJoinClientOptions::IsEnabled()) {
        if (!gArgs.GetBoolArg("-enablecoinjoin", true)) {
            // otherwise it's on by default, unless cmd line option says otherwise
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing is disabled via -enablecoinjoin=0 command line option, remove it to enable mixing again");
        } else {
            // not enablecoinjoin=false case,
            // most likely something bad happened and we disabled it while running the wallet
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing is disabled due to some internal error");
        }
    }

    auto it = coinJoinClientManagers.find(pwallet->GetName());

    if (request.params[0].get_str() == "start") {
        {
            LOCK(pwallet->cs_wallet);
            if (pwallet->IsLocked(true))
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please unlock wallet for mixing with walletpassphrase first.");
        }

        if (!it->second->StartMixing()) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing has been started already.");
        }

        bool result = it->second->DoAutomaticDenominating(*g_connman);
        return "Mixing " + (result ? "started successfully" : ("start failed: " + it->second->GetStatuses() + ", will retry"));
    }

    if (request.params[0].get_str() == "stop") {
        it->second->StopMixing();
        return "Mixing was stopped";
    }

    if (request.params[0].get_str() == "reset") {
        it->second->ResetPool();
        return "Mixing was reset";
    }

    return "Unknown command, please see \"help coinjoin\"";
}
#endif // ENABLE_WALLET

UniValue getpoolinfo(const JSONRPCRequest& request)
{
    throw std::runtime_error(
            "getpoolinfo\n"
            "DEPRECATED. Please use getcoinjoininfo instead.\n"
    );
}

UniValue getcoinjoininfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
                "getcoinjoininfo\n"
                "Returns an object containing an information about CoinJoin settings and state.\n"
                "\nResult (for regular nodes):\n"
                "{\n"
                "  \"enabled\": true|false,             (bool) Whether mixing functionality is enabled\n"
                "  \"multisession\": true|false,        (bool) Whether CoinJoin Multisession option is enabled\n"
                "  \"max_sessions\": xxx,               (numeric) How many parallel mixing sessions can there be at once\n"
                "  \"max_rounds\": xxx,                 (numeric) How many rounds to mix\n"
                "  \"max_amount\": xxx,                 (numeric) Target CoinJoin balance in " + CURRENCY_UNIT + "\n"
                "  \"denoms_goal\": xxx,                (numeric) How many inputs of each denominated amount to target\n"
                "  \"denoms_hardcap\": xxx,             (numeric) Maximum limit of how many inputs of each denominated amount to create\n"
                "  \"queue_size\": xxx,                 (numeric) How many queues there are currently on the network\n"
                "  \"running\": true|false,             (bool) Whether mixing is currently running\n"
                "  \"sessions\":                        (array of json objects)\n"
                "    [\n"
                "      {\n"
                "      \"protxhash\": \"...\",            (string) The ProTxHash of the masternode\n"
                "      \"outpoint\": \"txid-index\",      (string) The outpoint of the masternode\n"
                "      \"service\": \"host:port\",        (string) The IP address and port of the masternode\n"
                "      \"denomination\": xxx,           (numeric) The denomination of the mixing session in " + CURRENCY_UNIT + "\n"
                "      \"state\": \"...\",                (string) Current state of the mixing session\n"
                "      \"entries_count\": xxx,          (numeric) The number of entries in the mixing session\n"
                "      }\n"
                "      ,...\n"
                "    ],\n"
                "  \"keys_left\": xxx,                  (numeric) How many new keys are left since last automatic backup\n"
                "  \"warnings\": \"...\"                  (string) Warnings if any\n"
                "}\n"
                "\nResult (for masternodes):\n"
                "{\n"
                "  \"queue_size\": xxx,                 (numeric) How many queues there are currently on the network\n"
                "  \"denomination\": xxx,               (numeric) The denomination of the mixing session in " + CURRENCY_UNIT + "\n"
                "  \"state\": \"...\",                    (string) Current state of the mixing session\n"
                "  \"entries_count\": xxx,              (numeric) The number of entries in the mixing session\n"
                "}\n"
                "\nExamples:\n"
                + HelpExampleCli("getcoinjoininfo", "")
                + HelpExampleRpc("getcoinjoininfo", "")
        );
    }

    UniValue obj(UniValue::VOBJ);

    if (fMasternodeMode) {
        coinJoinServer.GetJsonInfo(obj);
        return obj;
    }


#ifdef ENABLE_WALLET

    CCoinJoinClientOptions::GetJsonInfo(obj);

    obj.pushKV("queue_size", coinJoinClientQueueManager.GetQueueSize());

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!pwallet) {
        return obj;
    }

    coinJoinClientManagers.at(pwallet->GetName())->GetJsonInfo(obj);

    obj.pushKV("keys_left",     pwallet->nKeysLeftSinceAutoBackup);
    obj.pushKV("warnings",      pwallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_WARNING
                                        ? "WARNING: keypool is almost depleted!" : "");
#endif // ENABLE_WALLET

    return obj;
}

static const CRPCCommand commands[] =
    { //  category              name                      actor (function)         argNames
        //  --------------------- ------------------------  ---------------------------------
        { "dash",               "getpoolinfo",            &getpoolinfo,            {} },
        { "dash",               "getcoinjoininfo",        &getcoinjoininfo,        {} },
#ifdef ENABLE_WALLET
        { "dash",               "coinjoin",               &coinjoin,               {} },
#endif // ENABLE_WALLET
};

void RegisterCoinJoinRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
