// Copyright (c) 2019-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>
#include <validation.h>
#include <coinjoin/context.h>
#include <coinjoin/server.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <util/check.h>
#include <rpc/util.h>
#include <util/strencodings.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#include <coinjoin/options.h>
#include <interfaces/coinjoin.h>
#include <wallet/rpcwallet.h>
#endif // ENABLE_WALLET

#include <univalue.h>

#ifdef ENABLE_WALLET
namespace {
void ValidateCoinJoinArguments()
{
    /* If CoinJoin is enabled, everything is working as expected, we can bail */
    if (CCoinJoinClientOptions::IsEnabled())
        return;

    /* CoinJoin is on by default, unless a command line argument says otherwise */
    if (!gArgs.GetBoolArg("-enablecoinjoin", true)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing is disabled via -enablecoinjoin=0 command line option, remove it to enable mixing again");
    }

    /* Most likely something bad happened and we disabled it while running the wallet */
    throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing is disabled due to an internal error");
}
} // anonymous namespace

static RPCHelpMan coinjoin()
{
    return RPCHelpMan{"coinjoin",
        "\nAvailable commands:\n"
        "  start       - Start mixing\n"
        "  stop        - Stop mixing\n"
        "  reset       - Reset mixing",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan coinjoin_reset()
{
    return RPCHelpMan{"coinjoin reset",
        "\nReset CoinJoin mixing\n",
        {},
        RPCResult{
            RPCResult::Type::STR, "", "Status of request"
        },
        RPCExamples{
            HelpExampleCli("coinjoin reset", "")
          + HelpExampleRpc("coinjoin reset", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    if (node.mn_activeman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Client-side mixing is not supported on masternodes");
    }

    ValidateCoinJoinArguments();

    auto cj_clientman = CHECK_NONFATAL(node.coinjoin_loader)->walletman().Get(wallet->GetName());
    CHECK_NONFATAL(cj_clientman)->ResetPool();

    return "Mixing was reset";
},
    };
}

static RPCHelpMan coinjoin_start()
{
    return RPCHelpMan{"coinjoin start",
        "\nStart CoinJoin mixing\n"
        "Wallet must be unlocked for mixing\n",
        {},
        RPCResult{
            RPCResult::Type::STR, "", "Status of request"
        },
        RPCExamples{
            HelpExampleCli("coinjoin start", "")
          + HelpExampleRpc("coinjoin start", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    if (node.mn_activeman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Client-side mixing is not supported on masternodes");
    }

    ValidateCoinJoinArguments();

    {
        LOCK(wallet->cs_wallet);
        if (wallet->IsLocked(true))
            throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please unlock wallet for mixing with walletpassphrase first.");
    }

    auto cj_clientman = CHECK_NONFATAL(CHECK_NONFATAL(node.coinjoin_loader)->walletman().Get(wallet->GetName()));
    if (!cj_clientman->StartMixing()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Mixing has been started already.");
    }

    ChainstateManager& chainman = EnsureChainman(node);
    CTxMemPool& mempool = EnsureMemPool(node);
    CConnman& connman = EnsureConnman(node);
    bool result = cj_clientman->DoAutomaticDenominating(chainman, connman, mempool);
    return "Mixing " + (result ? "started successfully" : ("start failed: " + cj_clientman->GetStatuses().original + ", will retry"));
},
    };
}

static RPCHelpMan coinjoin_stop()
{
    return RPCHelpMan{"coinjoin stop",
        "\nStop CoinJoin mixing\n",
        {},
        RPCResult{
            RPCResult::Type::STR, "", "Status of request"
        },
        RPCExamples{
            HelpExampleCli("coinjoin stop", "")
          + HelpExampleRpc("coinjoin stop", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    if (node.mn_activeman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Client-side mixing is not supported on masternodes");
    }

    ValidateCoinJoinArguments();

    CHECK_NONFATAL(node.coinjoin_loader);
    auto cj_clientman = node.coinjoin_loader->walletman().Get(wallet->GetName());

    CHECK_NONFATAL(cj_clientman);
    if (!cj_clientman->IsMixing()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No mix session to stop");
    }
    cj_clientman->StopMixing();

    return "Mixing was stopped";
},
    };
}

static RPCHelpMan coinjoinsalt()
{
    return RPCHelpMan{"coinjoinsalt",
        "\nAvailable commands:\n"
        "  generate  - Generate new CoinJoin salt\n"
        "  get       - Fetch existing CoinJoin salt\n"
        "  set       - Set new CoinJoin salt\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan coinjoinsalt_generate()
{
    return RPCHelpMan{"coinjoinsalt generate",
        "\nGenerate new CoinJoin salt and store it in the wallet database\n"
        "Cannot generate new salt if CoinJoin mixing is in process or wallet has private keys disabled.\n",
        {
            {"overwrite", RPCArg::Type::BOOL, RPCArg::Default{false}, "Generate new salt even if there is an existing salt and/or there is CoinJoin balance"},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "Status of CoinJoin salt generation and commitment"
        },
        RPCExamples{
            HelpExampleCli("coinjoinsalt generate", "")
          + HelpExampleRpc("coinjoinsalt generate", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const auto str_wallet = wallet->GetName();
    if (wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           strprintf("Wallet \"%s\" has private keys disabled, cannot perform CoinJoin!", str_wallet));
    }

    bool enable_overwrite{false}; // Default value
    if (!request.params[0].isNull()) {
        enable_overwrite = ParseBoolV(request.params[0], "overwrite");
    }

    if (!enable_overwrite && !wallet->GetCoinJoinSalt().IsNull()) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           strprintf("Wallet \"%s\" already has set CoinJoin salt!", str_wallet));
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (node.coinjoin_loader != nullptr) {
        auto cj_clientman = node.coinjoin_loader->walletman().Get(wallet->GetName());
        if (cj_clientman != nullptr && cj_clientman->IsMixing()) {
            throw JSONRPCError(RPC_WALLET_ERROR,
                               strprintf("Wallet \"%s\" is currently mixing, cannot change salt!", str_wallet));
        }
    }

    const auto wallet_balance{wallet->GetBalance()};
    const bool has_balance{(wallet_balance.m_anonymized
                          + wallet_balance.m_denominated_trusted
                          + wallet_balance.m_denominated_untrusted_pending) > 0};
    if (!enable_overwrite && has_balance) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("Wallet \"%s\" has CoinJoin balance, cannot continue!", str_wallet));
    }

    if (!wallet->SetCoinJoinSalt(GetRandHash())) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           strprintf("Unable to set new CoinJoin salt for wallet \"%s\"!", str_wallet));
    }

    wallet->ClearCoinJoinRoundsCache();

    return true;
},
    };
}

static RPCHelpMan coinjoinsalt_get()
{
    return RPCHelpMan{"coinjoinsalt get",
        "\nFetch existing CoinJoin salt\n"
        "Cannot fetch salt if wallet has private keys disabled.\n",
        {},
        RPCResult{
            RPCResult::Type::STR_HEX, "", "CoinJoin salt"
        },
        RPCExamples{
            HelpExampleCli("coinjoinsalt get", "")
          + HelpExampleRpc("coinjoinsalt get", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const auto str_wallet = wallet->GetName();
    if (wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           strprintf("Wallet \"%s\" has private keys disabled, cannot perform CoinJoin!", str_wallet));
    }

    const auto salt{wallet->GetCoinJoinSalt()};
    if (salt.IsNull()) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("Wallet \"%s\" has no CoinJoin salt!", str_wallet));
    }
    return salt.GetHex();
},
    };
}

static RPCHelpMan coinjoinsalt_set()
{
    return RPCHelpMan{"coinjoinsalt set",
        "\nSet new CoinJoin salt\n"
        "Cannot set salt if CoinJoin mixing is in process or wallet has private keys disabled.\n"
        "Will overwrite existing salt. The presence of a CoinJoin balance will cause the wallet to rescan.\n",
        {
            {"salt", RPCArg::Type::STR, RPCArg::Optional::NO, "Desired CoinJoin salt value for the wallet"},
            {"overwrite", RPCArg::Type::BOOL, RPCArg::Default{false}, "Overwrite salt even if CoinJoin balance present"},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "Status of CoinJoin salt change request"
        },
        RPCExamples{
            HelpExampleCli("coinjoinsalt set", "f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16")
          + HelpExampleRpc("coinjoinsalt set", "f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const auto salt{ParseHashV(request.params[0], "salt")};
    if (salt == uint256::ZERO) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid CoinJoin salt value");
    }

    bool enable_overwrite{false}; // Default value
    if (!request.params[1].isNull()) {
        enable_overwrite = ParseBoolV(request.params[1], "overwrite");
    }

    const auto str_wallet = wallet->GetName();
    if (wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           strprintf("Wallet \"%s\" has private keys disabled, cannot perform CoinJoin!", str_wallet));
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (node.coinjoin_loader != nullptr) {
        auto cj_clientman = node.coinjoin_loader->walletman().Get(wallet->GetName());
        if (cj_clientman != nullptr && cj_clientman->IsMixing()) {
            throw JSONRPCError(RPC_WALLET_ERROR,
                               strprintf("Wallet \"%s\" is currently mixing, cannot change salt!", str_wallet));
        }
    }

    const auto wallet_balance{wallet->GetBalance()};
    const bool has_balance{(wallet_balance.m_anonymized
                          + wallet_balance.m_denominated_trusted
                          + wallet_balance.m_denominated_untrusted_pending) > 0};
    if (has_balance && !enable_overwrite) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("Wallet \"%s\" has CoinJoin balance, cannot continue!", str_wallet));
    }

    if (!wallet->SetCoinJoinSalt(salt)) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("Unable to set new CoinJoin salt for wallet \"%s\"!", str_wallet));
    }

    wallet->ClearCoinJoinRoundsCache();

    return true;
},
    };
}
#endif // ENABLE_WALLET

// TODO: remove it completely
static RPCHelpMan getpoolinfo()
{
    return RPCHelpMan{"getpoolinfo",
                "DEPRECATED. Please use getcoinjoininfo instead.\n",
                {},
                RPCResults{},
                RPCExamples{""},
                [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
        throw JSONRPCError(RPC_METHOD_DEPRECATED, "Please use getcoinjoininfo instead");
},
    };
}

static RPCHelpMan getcoinjoininfo()
{
            return RPCHelpMan{"getcoinjoininfo",
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
                            {RPCResult::Type::NUM, "keys_left", /* optional */ true, "How many new keys are left since last automatic backup (if applicable)"},
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
                [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue obj(UniValue::VOBJ);

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (node.mn_activeman) {
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

    auto* manager = CHECK_NONFATAL(node.coinjoin_loader->walletman().Get(wallet->GetName()));
    manager->GetJsonInfo(obj);

    std::string warning_msg{""};
    if (wallet->IsLegacy()) {
        obj.pushKV("keys_left", wallet->nKeysLeftSinceAutoBackup);
        if (wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_WARNING) {
            warning_msg = "WARNING: keypool is almost depleted!";
        }
    }
    obj.pushKV("warnings", warning_msg);
#endif // ENABLE_WALLET

    return obj;
},
    };
}

void RegisterCoinJoinRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category               actor (function)
  //  ---------------------  -----------------------
    { "dash",                &getcoinjoininfo,        },
#ifdef ENABLE_WALLET
    { "dash",                &coinjoin,               },
    { "dash",                &coinjoin_reset,         },
    { "dash",                &coinjoin_start,         },
    { "dash",                &coinjoin_stop,          },
    { "dash",                &coinjoinsalt,           },
    { "dash",                &coinjoinsalt_generate,  },
    { "dash",                &coinjoinsalt_get,       },
    { "dash",                &coinjoinsalt_set,       },

    { "hidden",              &getpoolinfo,            },
#endif // ENABLE_WALLET
};
// clang-format on
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
