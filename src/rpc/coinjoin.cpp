// Copyright (c) 2019-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>
#include <validation.h>
#include <coinjoin/context.h>
#include <coinjoin/server.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/strencodings.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#include <coinjoin/options.h>
#include <wallet/rpcwallet.h>
#endif // ENABLE_WALLET

#include <univalue.h>

#ifdef ENABLE_WALLET
static UniValue coinjoin(const JSONRPCRequest& request)
{
            RPCHelpMan{"coinjoin",
                "\nAvailable commands:\n"
                "  start       - Start mixing\n"
                "  stop        - Stop mixing\n"
                "  reset       - Reset mixing",
                {
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
                },
                RPCResults{},
                RPCExamples{""},
            }.Check(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

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

    auto cj_clientman = ::coinJoinClientManagers->Get(*wallet);
    CHECK_NONFATAL(cj_clientman != nullptr);

    if (request.params[0].get_str() == "start") {
        {
            LOCK(wallet->cs_wallet);
            if (wallet->IsLocked(true))
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please unlock wallet for mixing with walletpassphrase first.");
        }

        if (!cj_clientman->StartMixing()) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing has been started already.");
        }

        const NodeContext& node = EnsureAnyNodeContext(request.context);
        CTxMemPool& mempool = EnsureMemPool(node);
        CBlockPolicyEstimator& fee_estimator = EnsureFeeEstimator(node);
        bool result = cj_clientman->DoAutomaticDenominating(*node.connman, fee_estimator, mempool);
        return "Mixing " + (result ? "started successfully" : ("start failed: " + cj_clientman->GetStatuses().original + ", will retry"));
    }

    if (request.params[0].get_str() == "stop") {
        cj_clientman->StopMixing();
        return "Mixing was stopped";
    }

    if (request.params[0].get_str() == "reset") {
        cj_clientman->ResetPool();
        return "Mixing was reset";
    }

    return "Unknown command, please see \"help coinjoin\"";
}
#endif // ENABLE_WALLET

static UniValue getpoolinfo(const JSONRPCRequest& request)
{
    throw std::runtime_error(
            RPCHelpMan{"getpoolinfo",
                "DEPRECATED. Please use getcoinjoininfo instead.\n",
                {},
                RPCResults{},
                RPCExamples{""}}
            .ToString());
}

static UniValue getcoinjoininfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getcoinjoininfo",
                "Returns an object containing an information about CoinJoin settings and state.\n",
                {},
                {
                    RPCResult{"for regular nodes",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "enabled", "Whether mixing functionality is enabled"},
                            {RPCResult::Type::BOOL, "multisession", "Whether CoinJoin Multisession option is enabled"},
                            {RPCResult::Type::NUM, "max_sessions", "How many parallel mixing sessions can there be at once"},
                            {RPCResult::Type::NUM, "max_rounds", "How many rounds to mix"},
                            {RPCResult::Type::NUM, "max_amount", "Target CoinJoin balance in " + CURRENCY_UNIT + ""},
                            {RPCResult::Type::NUM, "denoms_goal", "How many inputs of each denominated amount to target"},
                            {RPCResult::Type::NUM, "denoms_hardcap", "Maximum limit of how many inputs of each denominated amount to create"},
                            {RPCResult::Type::NUM, "queue_size", "How many queues there are currently on the network"},
                            {RPCResult::Type::BOOL, "running", "Whether mixing is currently running"},
                            {RPCResult::Type::ARR, "sessions", "",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::STR_HEX, "protxhash", "The ProTxHash of the masternode"},
                                    {RPCResult::Type::STR_HEX, "outpoint", "The outpoint of the masternode"},
                                    {RPCResult::Type::STR, "service", "The IP address and port of the masternode"},
                                    {RPCResult::Type::NUM, "denomination", "The denomination of the mixing session in " + CURRENCY_UNIT + ""},
                                    {RPCResult::Type::STR_HEX, "state", "Current state of the mixing session"},
                                    {RPCResult::Type::NUM, "entries_count", "The number of entries in the mixing session"},
                                }},
                            }},
                            {RPCResult::Type::NUM, "keys_left", "How many new keys are left since last automatic backup"},
                            {RPCResult::Type::STR, "warnings", "Warnings if any"},
                        }},
                    RPCResult{"for masternodes",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::NUM, "queue_size", "How many queues there are currently on the network"},
                            {RPCResult::Type::NUM, "denomination", "The denomination of the mixing session in " + CURRENCY_UNIT + ""},
                            {RPCResult::Type::STR_HEX, "state", "Current state of the mixing session"},
                            {RPCResult::Type::NUM, "entries_count", "The number of entries in the mixing session"},
                        }},
                },
                RPCExamples{
                    HelpExampleCli("getcoinjoininfo", "")
            + HelpExampleRpc("getcoinjoininfo", "")
                },
            }.Check(request);

    UniValue obj(UniValue::VOBJ);
    const NodeContext& node = EnsureAnyNodeContext(request.context);

    if (fMasternodeMode) {
        node.cj_ctx->server->GetJsonInfo(obj);
        return obj;
    }

#ifdef ENABLE_WALLET
    CCoinJoinClientOptions::GetJsonInfo(obj);

    obj.pushKV("queue_size", node.cj_ctx->queueman->GetQueueSize());

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) {
        return obj;
    }

    auto manager = ::coinJoinClientManagers->Get(*wallet);
    CHECK_NONFATAL(manager != nullptr);
    manager->GetJsonInfo(obj);

    obj.pushKV("keys_left",     wallet->nKeysLeftSinceAutoBackup);
    obj.pushKV("warnings",      wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_WARNING
                                        ? "WARNING: keypool is almost depleted!" : "");
#endif // ENABLE_WALLET

    return obj;
}
void RegisterCoinJoinRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
    { //  category              name                      actor (function)         argNames
        //  --------------------- ------------------------  ---------------------------------
        { "dash",               "getpoolinfo",            &getpoolinfo,            {} },
        { "dash",               "getcoinjoininfo",        &getcoinjoininfo,        {} },
#ifdef ENABLE_WALLET
        { "dash",               "coinjoin",               &coinjoin,               {} },
#endif // ENABLE_WALLET
};
// clang-format on
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
