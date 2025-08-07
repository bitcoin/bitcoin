// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <httpserver.h>
#include <policy/policy.h>
#include <rpc/blockchain.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/bip32.h>
#include <util/fees.h>
#include <util/translation.h>
#include <util/url.h>
#include <util/vector.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/rpc/wallet.h>
#include <wallet/rpc/util.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#include <key_io.h>

#include <coinjoin/client.h>
#include <coinjoin/options.h>

#include <optional>

#include <univalue.h>

namespace wallet {
/** Checks if a CKey is in the given CWallet compressed or otherwise*/
bool HaveKey(const SigningProvider& wallet, const CKey& key)
{
    CKey key2;
    key2.Set(key.begin(), key.end(), !key.IsCompressed());
    return wallet.HaveKey(key.GetPubKey().GetID()) || wallet.HaveKey(key2.GetPubKey().GetID());
}

static RPCHelpMan listaddressbalances()
{
    return RPCHelpMan{"listaddressbalances",
        "\nLists addresses of this wallet and their balances\n",
        {
            {"minamount", RPCArg::Type::NUM, RPCArg::Default{0}, "Minimum balance in " + CURRENCY_UNIT + " an address should have to be shown in the list"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR_AMOUNT, "amount", "The Dash address and the amount in " + CURRENCY_UNIT},
            }
        },
        RPCExamples{
            HelpExampleCli("listaddressbalances", "")
    + HelpExampleCli("listaddressbalances", "10")
    + HelpExampleRpc("listaddressbalances", "")
    + HelpExampleRpc("listaddressbalances", "10")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    CAmount nMinAmount = 0;
    if (!request.params[0].isNull())
        nMinAmount = AmountFromValue(request.params[0]);

    if (nMinAmount < 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    UniValue jsonBalances(UniValue::VOBJ);
    std::map<CTxDestination, CAmount> balances = GetAddressBalances(*pwallet);
    for (auto& balance : balances)
        if (balance.second >= nMinAmount)
            jsonBalances.pushKV(EncodeDestination(balance.first), ValueFromAmount(balance.second));

    return jsonBalances;
},
    };
}


static RPCHelpMan setcoinjoinrounds()
{
    return RPCHelpMan{"setcoinjoinrounds",
        "\nSet the number of rounds for CoinJoin.\n",
        {
            {"rounds", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "The default number of rounds is " + ToString(DEFAULT_COINJOIN_ROUNDS) +
                " Cannot be more than " + ToString(MAX_COINJOIN_ROUNDS) + " nor less than " + ToString(MIN_COINJOIN_ROUNDS)},
        },
        RPCResults{},
        RPCExamples{
            HelpExampleCli("setcoinjoinrounds", "4")
    + HelpExampleRpc("setcoinjoinrounds", "16")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    int nRounds = request.params[0].getInt<int>();

    if (nRounds > MAX_COINJOIN_ROUNDS || nRounds < MIN_COINJOIN_ROUNDS)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid number of rounds");

    CCoinJoinClientOptions::SetRounds(nRounds);

    return NullUniValue;
},
    };
}

static RPCHelpMan setcoinjoinamount()
{
    return RPCHelpMan{"setcoinjoinamount",
        "\nSet the goal amount in " + CURRENCY_UNIT + " for CoinJoin.\n",
        {
            {"amount", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "The default amount is " + ToString(DEFAULT_COINJOIN_AMOUNT) +
                " Cannot be more than " + ToString(MAX_COINJOIN_AMOUNT) + " nor less than " + ToString(MIN_COINJOIN_AMOUNT)},
        },
        RPCResults{},
        RPCExamples{
            HelpExampleCli("setcoinjoinamount", "500")
    + HelpExampleRpc("setcoinjoinamount", "208")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    int nAmount = request.params[0].getInt<int>();

    if (nAmount > MAX_COINJOIN_AMOUNT || nAmount < MIN_COINJOIN_AMOUNT)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount of " + CURRENCY_UNIT + " as mixing goal amount");

    CCoinJoinClientOptions::SetAmount(nAmount);

    return NullUniValue;
},
    };
}

static RPCHelpMan getwalletinfo()
{
    return RPCHelpMan{"getwalletinfo",
                "Returns an object containing various wallet state info.\n",
                {},
                RPCResult{
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "walletname", "the wallet name"},
                            {RPCResult::Type::NUM, "walletversion", "the wallet version"},
                            {RPCResult::Type::STR, "format", "the database format (bdb or sqlite)"},
                            {RPCResult::Type::NUM, "balance", "DEPRECATED. Identical to getbalances().mine.trusted"},
                            {RPCResult::Type::NUM, "coinjoin_balance", "DEPRECATED. Identical to getbalances().mine.coinjoin"},
                            {RPCResult::Type::NUM, "unconfirmed_balance", "DEPRECATED. Identical to getbalances().mine.untrusted_pending"},
                            {RPCResult::Type::NUM, "immature_balance", "DEPRECATED. Identical to getbalances().mine.immature"},
                            {RPCResult::Type::NUM, "txcount", "the total number of transactions in the wallet"},
                            {RPCResult::Type::NUM_TIME, "timefirstkey", "the " + UNIX_EPOCH_TIME + " of the oldest known key in the wallet"},
                            {RPCResult::Type::NUM_TIME, "keypoololdest", /* optional */ true, "the " + UNIX_EPOCH_TIME + " of the oldest pre-generated key in the key pool. Legacy wallets only"},
                            {RPCResult::Type::NUM, "keypoolsize", "how many new keys are pre-generated (only counts external keys)"},
                            {RPCResult::Type::NUM, "keypoolsize_hd_internal", /* optional */ true, "how many new keys are pre-generated for internal use (used for change outputs, only appears if the wallet is using this feature, otherwise external keys are used)"},
                            {RPCResult::Type::NUM, "keys_left", "how many new keys are left since last automatic backup"},
                            {RPCResult::Type::NUM_TIME, "unlocked_until", /* optional */ true, "the " + UNIX_EPOCH_TIME + " until which the wallet is unlocked for transfers, or 0 if the wallet is locked (only present for passphrase-encrypted wallets)"},
                            {RPCResult::Type::STR_AMOUNT, "paytxfee", "the transaction fee configuration, set in " + CURRENCY_UNIT + "/kB"},
                            {RPCResult::Type::STR_HEX, "hdchainid", "the ID of the HD chain"},
                            {RPCResult::Type::NUM, "hdaccountcount", "how many accounts of the HD chain are in this wallet"},
                            {RPCResult::Type::ARR, "hdaccounts", "",
                                {
                                {RPCResult::Type::OBJ, "", "",
                                    {
                                        {RPCResult::Type::NUM, "hdaccountindex", "the index of the account"},
                                        {RPCResult::Type::NUM, "hdexternalkeyindex", "current external childkey index"},
                                        {RPCResult::Type::NUM, "hdinternalkeyindex", "current internal childkey index"},
                                }},
                            }},
                            {RPCResult::Type::BOOL, "private_keys_enabled", "false if privatekeys are disabled for this wallet (enforced watch-only wallet)"},
                            {RPCResult::Type::BOOL, "avoid_reuse", "whether this wallet tracks clean/dirty coins in terms of reuse"},
                            {RPCResult::Type::OBJ, "scanning", "current scanning details, or false if no scan is in progress",
                                {
                                     {RPCResult::Type::NUM, "duration", "elapsed seconds since scan start"},
                                     {RPCResult::Type::NUM, "progress", "scanning progress percentage [0.0, 1.0]"},
                                }},
                            {RPCResult::Type::BOOL, "descriptors", "whether this wallet uses descriptors for scriptPubKey management"},
                        },
                },
                RPCExamples{
                    HelpExampleCli("getwalletinfo", "")
            + HelpExampleRpc("getwalletinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    LegacyScriptPubKeyMan* spk_man = pwallet->GetLegacyScriptPubKeyMan();
    CHDChain hdChainCurrent;
    bool fHDEnabled = spk_man && spk_man->GetHDChain(hdChainCurrent);
    UniValue obj(UniValue::VOBJ);

    const auto bal = GetBalance(*pwallet);
    obj.pushKV("walletname", pwallet->GetName());
    obj.pushKV("walletversion", pwallet->GetVersion());
    obj.pushKV("format", pwallet->GetDatabase().Format());
    obj.pushKV("balance", ValueFromAmount(bal.m_mine_trusted));
    obj.pushKV("coinjoin_balance",       ValueFromAmount(bal.m_anonymized));
    obj.pushKV("unconfirmed_balance", ValueFromAmount(bal.m_mine_untrusted_pending));
    obj.pushKV("immature_balance", ValueFromAmount(bal.m_mine_immature));
    obj.pushKV("txcount",       (int)pwallet->mapWallet.size());
    // TODO: implement timefirstkey for Descriptor KeyMan or explain why it's not provided
    if (spk_man) {
        obj.pushKV("timefirstkey", spk_man->GetTimeFirstKey());
    }
    const auto kp_oldest = pwallet->GetOldestKeyPoolTime();
    if (kp_oldest.has_value()) {
        obj.pushKV("keypoololdest", kp_oldest.value());
    }
    size_t kpExternalSize = pwallet->KeypoolCountExternalKeys();
    obj.pushKV("keypoolsize", kpExternalSize);
    obj.pushKV("keypoolsize_hd_internal", pwallet->GetKeyPoolSize() - kpExternalSize);
    obj.pushKV("keys_left", pwallet->nKeysLeftSinceAutoBackup);
    if (pwallet->IsCrypted()) {
        obj.pushKV("unlocked_until", pwallet->nRelockTime);
    }
    obj.pushKV("paytxfee", ValueFromAmount(pwallet->m_pay_tx_fee.GetFeePerK()));
    if (fHDEnabled) {
        obj.pushKV("hdchainid", hdChainCurrent.GetID().GetHex());
        obj.pushKV("hdaccountcount", hdChainCurrent.CountAccounts());
        UniValue accounts(UniValue::VARR);
        for (size_t i = 0; i < hdChainCurrent.CountAccounts(); ++i)
        {
            CHDAccount acc;
            UniValue account(UniValue::VOBJ);
            account.pushKV("hdaccountindex", i);
            if(hdChainCurrent.GetAccount(i, acc)) {
                account.pushKV("hdexternalkeyindex", acc.nExternalChainCounter);
                account.pushKV("hdinternalkeyindex", acc.nInternalChainCounter);
            } else {
                account.pushKV("error", strprintf("account %d is missing", i));
            }
            accounts.push_back(account);
        }
        obj.pushKV("hdaccounts", accounts);
    }
    obj.pushKV("private_keys_enabled", !pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    obj.pushKV("avoid_reuse", pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE));
    if (pwallet->IsScanning()) {
        UniValue scanning(UniValue::VOBJ);
        scanning.pushKV("duration", pwallet->ScanningDuration() / 1000);
        scanning.pushKV("progress", pwallet->ScanningProgress());
        obj.pushKV("scanning", scanning);
    } else {
        obj.pushKV("scanning", false);
    }
    obj.pushKV("descriptors", pwallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    return obj;
},
    };
}

static RPCHelpMan listwalletdir()
{
    return RPCHelpMan{"listwalletdir",
        "Returns a list of wallets in the wallet directory.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "wallets", "",
                        {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "name", "The wallet name"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listwalletdir", "")
    + HelpExampleRpc("listwalletdir", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue wallets(UniValue::VARR);
    for (const auto& path : ListDatabases(GetWalletDir())) {
        UniValue wallet(UniValue::VOBJ);
        wallet.pushKV("name", path.utf8string());
        wallets.push_back(wallet);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("wallets", wallets);
    return result;
},
    };
}

static RPCHelpMan listwallets()
{
    return RPCHelpMan{"listwallets",
        "Returns a list of currently loaded wallets.\n"
        "For full information on the wallet, use \"getwalletinfo\"\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "walletname", "the wallet name"},
            }
        },
        RPCExamples{
            HelpExampleCli("listwallets", "")
    + HelpExampleRpc("listwallets", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue obj(UniValue::VARR);

    WalletContext& context = EnsureWalletContext(request.context);
    for (const std::shared_ptr<CWallet>& wallet : GetWallets(context)) {
        LOCK(wallet->cs_wallet);
        obj.push_back(wallet->GetName());
    }

    return obj;
},
    };
}

static RPCHelpMan upgradetohd()
{
    return RPCHelpMan{"upgradetohd",
        "\nUpgrades non-HD wallets to HD.\n"
        "\nIf your wallet is encrypted, the wallet passphrase must be supplied. Supplying an incorrect"
        "\npassphrase may result in your wallet getting locked.\n"
        "\nWarning: You will need to make a new backup of your wallet after setting the HD wallet mnemonic.\n",
        {
            {"mnemonic", RPCArg::Type::STR, RPCArg::Default{""}, "Mnemonic as defined in BIP39 to use for the new HD wallet. Use an empty string \"\" to generate a new random mnemonic."},
            {"mnemonicpassphrase", RPCArg::Type::STR, RPCArg::Default{""}, "Optional mnemonic passphrase as defined in BIP39"},
            {"walletpassphrase", RPCArg::Type::STR, RPCArg::Default{""}, "If your wallet is encrypted you must have your wallet passphrase here. If your wallet is not encrypted, specifying wallet passphrase will trigger wallet encryption."},
            {"rescan", RPCArg::Type::BOOL, RPCArg::DefaultHint{"false if mnemonic is empty"}, "Whether to rescan the blockchain for missing transactions or not"},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "true if successful"
        },
        RPCExamples{
            HelpExampleCli("upgradetohd", "")
    + HelpExampleCli("upgradetohd", "\"mnemonicword1 ... mnemonicwordN\"")
    + HelpExampleCli("upgradetohd", "\"mnemonicword1 ... mnemonicwordN\" \"mnemonicpassphrase\"")
    + HelpExampleCli("upgradetohd", "\"mnemonicword1 ... mnemonicwordN\" \"\" \"walletpassphrase\"")
    + HelpExampleCli("upgradetohd", "\"mnemonicword1 ... mnemonicwordN\" \"mnemonicpassphrase\" \"walletpassphrase\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    bool generate_mnemonic = request.params[0].isNull() || request.params[0].get_str().empty();

    {
        LOCK(pwallet->cs_wallet);

        SecureString wallet_passphrase;
        wallet_passphrase.reserve(100);

        if (request.params[2].isNull()) {
            if (pwallet->IsCrypted()) {
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet encrypted but passphrase not supplied to RPC.");
            }
        } else {
            wallet_passphrase = std::string_view{request.params[2].get_str()};
        }

        SecureString mnemonic;
        mnemonic.reserve(256);
        if (!generate_mnemonic) {
            mnemonic = std::string_view{request.params[0].get_str()};
        }

        SecureString mnemonic_passphrase;
        mnemonic_passphrase.reserve(256);
        if (!request.params[1].isNull()) {
            mnemonic_passphrase = std::string_view{request.params[1].get_str()};
        }

        // TODO: breaking changes kept for v21!
        // instead upgradetohd let's use more straightforward 'sethdseed'
        constexpr bool is_v21 = false;
        const int previous_version{pwallet->GetVersion()};
        if (is_v21 && previous_version >= FEATURE_HD) {
            return JSONRPCError(RPC_WALLET_ERROR, "Already at latest version. Wallet version unchanged.");
        }

        // Do not do anything to HD wallets
        if (pwallet->IsHDEnabled()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Cannot upgrade a wallet to HD if it is already upgraded to HD");
        }

        if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Private keys are disabled for this wallet");
        }

        pwallet->WalletLogPrintf("Upgrading wallet to HD\n");
        pwallet->SetMinVersion(FEATURE_HD);

        if (pwallet->IsCrypted()) {
            if (wallet_passphrase.empty()) {
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: Wallet encrypted but supplied empty wallet passphrase");
            }

            // We are intentionally re-locking the wallet so we can validate passphrase
            // by verifying if it can unlock the wallet
            pwallet->Lock();

            // Unlock the wallet
            if (!pwallet->Unlock(wallet_passphrase)) {
                // Check if the passphrase has a null character (see bitcoin#27067 for details)
                if (wallet_passphrase.find('\0') == std::string::npos) {
                    throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
                } else {
                    throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered is incorrect. "
                                                                        "It contains a null character (ie - a zero byte). "
                                                                        "If the passphrase was set with a version of this software prior to 23.0, "
                                                                        "please try again with only the characters up to — but not including — "
                                                                        "the first null character. If this is successful, please set a new "
                                                                        "passphrase to avoid this issue in the future.");
                }
            }
        }

        if (pwallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            pwallet->SetupDescriptorScriptPubKeyMans(mnemonic, mnemonic_passphrase);
        } else {
            auto spk_man = pwallet->GetLegacyScriptPubKeyMan();
            if (!spk_man) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error: Legacy ScriptPubKeyMan is not available");
            }

            if (pwallet->IsCrypted()) {
                pwallet->WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                        spk_man->GenerateNewHDChain(mnemonic, mnemonic_passphrase, encryption_key);
                        return true;
                    });
            } else {
                spk_man->GenerateNewHDChain(mnemonic, mnemonic_passphrase);
            }
        }

        if (pwallet->IsCrypted()) {
            // Relock encrypted wallet
            pwallet->Lock();
        } else if (!wallet_passphrase.empty()) {
            // Encrypt non-encrypted wallet
            if (!pwallet->EncryptWallet(wallet_passphrase)) {
                throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Failed to encrypt HD wallet");
            }
        }
    } // pwallet->cs_wallet

    // If you are generating new mnemonic it is assumed that the addresses have never gotten a transaction before, so you don't need to rescan for transactions
    bool rescan = request.params[3].isNull() ? !generate_mnemonic : request.params[3].get_bool();
    if (rescan) {
        WalletRescanReserver reserver(*pwallet);
        if (!reserver.reserve()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
        }
        CWallet::ScanResult result = pwallet->ScanForWalletTransactions(pwallet->chain().getBlockHash(0), 0, {}, reserver, /*fUpdate=*/true, /*save_progress=*/false);
        switch (result.status) {
        case CWallet::ScanResult::SUCCESS:
            break;
        case CWallet::ScanResult::FAILURE:
            throw JSONRPCError(RPC_MISC_ERROR, "Rescan failed. Potentially corrupted data files.");
        case CWallet::ScanResult::USER_ABORT:
            throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted.");
            // no default case, so the compiler can warn about missing cases
        }
    }

    return true;
},
    };
}

static RPCHelpMan loadwallet()
{
    return RPCHelpMan{"loadwallet",
        "\nLoads a wallet from a wallet file or directory."
        "\nNote that all wallet command-line options used when starting dashd will be"
        "\napplied to the new wallet (eg, rescan, etc).\n",
        {
            {"filename", RPCArg::Type::STR, RPCArg::Optional::NO, "The wallet directory or .dat file."},
            {"load_on_startup", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Save wallet name to persistent settings and load on startup. True to add wallet to startup list, false to remove, null to leave unchanged."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "name", "The wallet name if loaded successfully."},
                {RPCResult::Type::STR, "warning", "Warning message if wallet was not loaded cleanly."},
            }
        },
        RPCExamples{
            HelpExampleCli("loadwallet", "\"test.dat\"")
    + HelpExampleRpc("loadwallet", "\"test.dat\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    WalletContext& context = EnsureWalletContext(request.context);
    const std::string name(request.params[0].get_str());

    DatabaseOptions options;
    DatabaseStatus status;
    ReadDatabaseArgs(*context.args, options);
    options.require_existing = true;
    bilingual_str error;
    std::vector<bilingual_str> warnings;
    std::optional<bool> load_on_start = request.params[1].isNull() ? std::nullopt : std::optional<bool>(request.params[1].get_bool());
    std::shared_ptr<CWallet> const wallet = LoadWallet(context, name, load_on_start, options, status, error, warnings);

    HandleWalletError(wallet, status, error);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("name", wallet->GetName());
    obj.pushKV("warning", Join(warnings, Untranslated("\n")).original);

    return obj;
},
    };
}

static RPCHelpMan setwalletflag()
{
    std::string flags;
    for (auto& it : WALLET_FLAG_MAP)
        if (it.second & MUTABLE_WALLET_FLAGS)
            flags += (flags == "" ? "" : ", ") + it.first;

    return RPCHelpMan{"setwalletflag",
        "\nChange the state of the given wallet flag for a wallet.\n",
        {
            {"flag", RPCArg::Type::STR, RPCArg::Optional::NO, "The name of the flag to change. Current available flags: " + flags},
            {"value", RPCArg::Type::BOOL, RPCArg::Default{true}, "The new state."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "flag_name", "The name of the flag that was modified"},
                {RPCResult::Type::BOOL, "flag_state", "The new state of the flag"},
                {RPCResult::Type::STR, "warnings", "Any warnings associated with the change"},
            }
        },
        RPCExamples{
            HelpExampleCli("setwalletflag", "avoid_reuse")
      + HelpExampleRpc("setwalletflag", "\"avoid_reuse\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;


    std::string flag_str = request.params[0].get_str();
    bool value = request.params[1].isNull() || request.params[1].get_bool();

    if (!WALLET_FLAG_MAP.count(flag_str)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown wallet flag: %s", flag_str));
    }

    auto flag = WALLET_FLAG_MAP.at(flag_str);

    if (!(flag & MUTABLE_WALLET_FLAGS)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Wallet flag is immutable: %s", flag_str));
    }

    UniValue res(UniValue::VOBJ);

    if (pwallet->IsWalletFlagSet(flag) == value) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Wallet flag is already set to %s: %s", value ? "true" : "false", flag_str));
    }

    res.pushKV("flag_name", flag_str);
    res.pushKV("flag_state", value);

    if (value) {
        pwallet->SetWalletFlag(flag);
    } else {
        pwallet->UnsetWalletFlag(flag);
    }

    if (flag && value && WALLET_FLAG_CAVEATS.count(flag)) {
        res.pushKV("warnings", WALLET_FLAG_CAVEATS.at(flag));
    }

    return res;
},
    };
}

static RPCHelpMan createwallet()
{
    return RPCHelpMan{
        "createwallet",
        "\nCreates and loads a new wallet.\n",
        {
            {"wallet_name", RPCArg::Type::STR, RPCArg::Optional::NO, "The name for the new wallet. If this is a path, the wallet will be created at the path location."},
            {"disable_private_keys", RPCArg::Type::BOOL, RPCArg::Default{false}, "Disable the possibility of private keys (only watchonlys are possible in this mode)."},
            {"blank", RPCArg::Type::BOOL, RPCArg::Default{false}, "Create a blank wallet. A blank wallet has no keys or HD seed. One can be set using upgradetohd (by mnemonic) or sethdseed (WIF private key)."},
            {"passphrase", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Encrypt the wallet with this passphrase."},
            {"avoid_reuse", RPCArg::Type::BOOL, RPCArg::Default{false}, "Keep track of coin reuse, and treat dirty and clean coins differently with privacy considerations in mind."},
            {"descriptors", RPCArg::Type::BOOL, RPCArg::Default{false}, "Create a native descriptor wallet. The wallet will use descriptors internally to handle address creation. This feature is well-tested but still considered experimental."},
            {"load_on_startup", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Save wallet name to persistent settings and load on startup. True to add wallet to startup list, false to remove, null to leave unchanged."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "name", "The wallet name if created successfully. If the wallet was created using a full path, the wallet_name will be the full path."},
                {RPCResult::Type::STR, "warning", "Warning message if wallet was not loaded cleanly."},
            }
        },
        RPCExamples{
            HelpExampleCli("createwallet", "\"testwallet\"")
            + HelpExampleRpc("createwallet", "\"testwallet\"")
            + HelpExampleCliNamed("createwallet", {{"wallet_name", "descriptors"}, {"avoid_reuse", true}, {"descriptors", true}, {"load_on_startup", true}})
            + HelpExampleRpcNamed("createwallet", {{"wallet_name", "descriptors"}, {"avoid_reuse", true}, {"descriptors", true}, {"load_on_startup", true}})
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    WalletContext& context = EnsureWalletContext(request.context);
    uint64_t flags = 0;
    if (!request.params[1].isNull() && request.params[1].get_bool()) {
        flags |= WALLET_FLAG_DISABLE_PRIVATE_KEYS;
    }

    if (!request.params[2].isNull() && request.params[2].get_bool()) {
        flags |= WALLET_FLAG_BLANK_WALLET;
    }
    SecureString passphrase;
    passphrase.reserve(100);
    std::vector<bilingual_str> warnings;
    if (!request.params[3].isNull()) {
        passphrase = std::string_view{request.params[3].get_str()};
        if (passphrase.empty()) {
            // Empty string means unencrypted
            warnings.emplace_back(Untranslated("Empty string given as passphrase, wallet will not be encrypted."));
        }
    }

    if (!request.params[4].isNull() && request.params[4].get_bool()) {
        flags |= WALLET_FLAG_AVOID_REUSE;
    }
    if (!request.params[5].isNull() && request.params[5].get_bool()) {
#ifndef USE_SQLITE
        throw JSONRPCError(RPC_WALLET_ERROR, "Compiled without sqlite support (required for descriptor wallets)");
#endif
        if (request.params[6].isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "The createwallet RPC requires specifying the 'load_on_startup' flag when creating descriptor wallets. Dash Core v21 introduced this requirement due to breaking changes in the createwallet RPC.");
        }
        flags |= WALLET_FLAG_DESCRIPTORS;
        warnings.emplace_back(Untranslated("Wallet is an experimental descriptor wallet"));
    }

#ifndef USE_BDB
    if (!(flags & WALLET_FLAG_DESCRIPTORS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Compiled without bdb support (required for legacy wallets)");
    }
#endif
    DatabaseOptions options;
    DatabaseStatus status;
    ReadDatabaseArgs(*context.args, options);
    options.require_create = true;
    options.create_flags = flags;
    options.create_passphrase = passphrase;
    bilingual_str error;
    std::optional<bool> load_on_start = request.params[6].isNull() ? std::nullopt : std::optional<bool>(request.params[6].get_bool());
    const std::shared_ptr<CWallet> wallet = CreateWallet(context, request.params[0].get_str(), load_on_start, options, status, error, warnings);
    if (!wallet) {
        RPCErrorCode code = status == DatabaseStatus::FAILED_ENCRYPT ? RPC_WALLET_ENCRYPTION_FAILED : RPC_WALLET_ERROR;
        throw JSONRPCError(code, error.original);
    }
    wallet->SetupLegacyScriptPubKeyMan();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("name", wallet->GetName());
    obj.pushKV("warning", Join(warnings, Untranslated("\n")).original);

    return obj;
},
    };
}

static RPCHelpMan unloadwallet()
{
    return RPCHelpMan{"unloadwallet",
        "Unloads the wallet referenced by the request endpoint otherwise unloads the wallet specified in the argument.\n"
        "Specifying the wallet name on a wallet endpoint is invalid.",
        {
            {"wallet_name", RPCArg::Type::STR, RPCArg::DefaultHint{"the wallet name from the RPC endpoint"}, "The name of the wallet to unload. If provided both here and in the RPC endpoint, the two must be identical."},
            {"load_on_startup", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Save wallet name to persistent settings and load on startup. True to add wallet to startup list, false to remove, null to leave unchanged."},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::STR, "warning", "Warning message if wallet was not unloaded cleanly."},
        }},
        RPCExamples{
            HelpExampleCli("unloadwallet", "wallet_name")
    + HelpExampleRpc("unloadwallet", "wallet_name")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string wallet_name;
    if (GetWalletNameFromJSONRPCRequest(request, wallet_name)) {
        if (!(request.params[0].isNull() || request.params[0].get_str() == wallet_name)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "RPC endpoint wallet and wallet_name parameter specify different wallets");
        }
    } else {
        wallet_name = request.params[0].get_str();
    }

    WalletContext& context = EnsureWalletContext(request.context);
    std::shared_ptr<CWallet> wallet = GetWallet(context, wallet_name);
    if (!wallet) {
        throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Requested wallet does not exist or is not loaded");
    }

    std::vector<bilingual_str> warnings;
    {
        WalletRescanReserver reserver(*wallet);
        if (!reserver.reserve()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
        }

        // Release the "main" shared pointer and prevent further notifications.
        // Note that any attempt to load the same wallet would fail until the wallet
        // is destroyed (see CheckUniqueFileid).
        std::optional<bool> load_on_start = request.params[1].isNull() ? std::nullopt : std::optional<bool>(request.params[1].get_bool());
        if (!RemoveWallet(context, wallet, load_on_start, warnings)) {
            throw JSONRPCError(RPC_MISC_ERROR, "Requested wallet already unloaded");
        }
    }

    UnloadWallet(std::move(wallet));

    UniValue result(UniValue::VOBJ);
    result.pushKV("warning", Join(warnings, Untranslated("\n")).original);
    return result;
},
    };
}

static RPCHelpMan wipewallettxes()
{
    return RPCHelpMan{"wipewallettxes",
        "\nWipe wallet transactions.\n"
        "Note: Use \"rescanblockchain\" to initiate the scanning progress and recover wallet transactions.\n",
        {
            {"keep_confirmed", RPCArg::Type::BOOL, RPCArg::Default{false}, "Do not wipe confirmed transactions"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("wipewallettxes", "")
    + HelpExampleRpc("wipewallettxes", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();

    WalletRescanReserver reserver(*pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort rescan or wait.");
    }

    LOCK(pwallet->cs_wallet);

    bool keep_confirmed{false};
    if (!request.params[0].isNull()) {
        keep_confirmed = request.params[0].get_bool();
    }

    const size_t WALLET_SIZE{pwallet->mapWallet.size()};
    const size_t STEPS{20};
    const size_t BATCH_SIZE = std::max(WALLET_SIZE / STEPS, size_t(1000));

    pwallet->ShowProgress(strprintf("%s " + _("Wiping wallet transactions…").translated, pwallet->GetDisplayName()), 0);

    for (size_t progress = 0; progress < STEPS; ++progress) {
        std::vector<uint256> vHashIn;
        std::vector<uint256> vHashOut;
        size_t count{0};

        for (auto& [txid, wtx] : pwallet->mapWallet) {
            if (progress < STEPS - 1 && ++count > BATCH_SIZE) break;
            if (keep_confirmed && wtx.isConfirmed()) continue;
            vHashIn.push_back(txid);
        }

        if (vHashIn.size() > 0 && pwallet->ZapSelectTx(vHashIn, vHashOut) != DBErrors::LOAD_OK) {
            pwallet->ShowProgress(strprintf("%s " + _("Wiping wallet transactions…").translated, pwallet->GetDisplayName()), 100);
            throw JSONRPCError(RPC_WALLET_ERROR, "Could not properly delete transactions.");
        }

        CHECK_NONFATAL(vHashOut.size() == vHashIn.size());

        if (pwallet->IsAbortingRescan() || pwallet->chain().shutdownRequested()) {
            pwallet->ShowProgress(strprintf("%s " + _("Wiping wallet transactions…").translated, pwallet->GetDisplayName()), 100);
            throw JSONRPCError(RPC_MISC_ERROR, "Wiping was aborted by user.");
        }

        pwallet->ShowProgress(strprintf("%s " + _("Wiping wallet transactions…").translated, pwallet->GetDisplayName()), std::max(1, std::min(99, int(progress * 100 / STEPS))));
    }

    pwallet->ShowProgress(strprintf("%s " + _("Wiping wallet transactions…").translated, pwallet->GetDisplayName()), 100);

    return NullUniValue;
},
    };
}

static RPCHelpMan sethdseed()
{
    return RPCHelpMan{"sethdseed",
                "\nSet or generate a new HD wallet seed. Non-HD wallets will not be upgraded to being a HD wallet. Wallets that are already\n"
                "HD can not be updated to a new HD seed.\n"
                "\nNote that you will need to MAKE A NEW BACKUP of your wallet after setting the HD wallet seed." + HELP_REQUIRING_PASSPHRASE +
                "Note: This command is only compatible with legacy wallets.\n",
                {
                    {"newkeypool", RPCArg::Type::BOOL, RPCArg::Default{true}, "Whether to flush old unused addresses, including change addresses, from the keypool and regenerate it.\n"
                                         "If true, the next address from getnewaddress and change address from getrawchangeaddress will be from this new seed.\n"
                                         "If false, addresses from the existing keypool will be used until it has been depleted."},
                    {"seed", RPCArg::Type::STR, RPCArg::DefaultHint{"random seed"}, "The WIF private key to use as the new HD seed.\n"
                                         "The seed value can be retrieved using the dumpwallet command. It is the private key marked hdseed=1"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("sethdseed", "")
            + HelpExampleCli("sethdseed", "false")
            + HelpExampleCli("sethdseed", "true \"wifkey\"")
            + HelpExampleRpc("sethdseed", "true, \"wifkey\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // TODO: add mnemonic feature to sethdseed or remove it in favour of upgradetohd
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet, true);

    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Cannot set a HD seed to a wallet with private keys disabled");
    }

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    // Do not do anything to non-HD wallets
    if (!pwallet->CanSupportFeature(FEATURE_HD)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Cannot set a HD seed on a non-HD wallet. Use the upgradewallet RPC in order to upgrade a non-HD wallet to HD");
    }
    if (pwallet->IsHDEnabled()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Cannot set a HD seed. The wallet already has a seed");
    }

    EnsureWalletIsUnlocked(*pwallet);

    bool flush_key_pool = true;
    if (!request.params[0].isNull()) {
        flush_key_pool = request.params[0].get_bool();
    }

    if (request.params[1].isNull()) {
        spk_man.GenerateNewHDChain("", "");
    } else {
        CKey key = DecodeSecret(request.params[1].get_str());
        if (!key.IsValid()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
        }
        if (HaveKey(spk_man, key)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Already have this key (either as an HD seed or as a loose private key)");
        }
        CHDChain newHdChain;
        if (!newHdChain.SetSeed(SecureVector(key.begin(), key.end()), true)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key: SetSeed failed");
        }
        if (!spk_man.AddHDChainSingle(newHdChain)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key: AddHDChainSingle failed");
        }
        // add default account
        newHdChain.AddAccount();
    }

    if (flush_key_pool) spk_man.NewKeyPool();

    return NullUniValue;
},
    };
}

static RPCHelpMan upgradewallet()
{
    return RPCHelpMan{"upgradewallet",
        "\nUpgrade the wallet. Upgrades to the latest version if no version number is specified.\n"
        "New keys may be generated and a new wallet backup will need to be made.",
        {
            {"version", RPCArg::Type::NUM, RPCArg::Default{int{FEATURE_LATEST}}, "The version number to upgrade to. Default is the latest wallet version."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "wallet_name", "Name of wallet this operation was performed on"},
                {RPCResult::Type::NUM, "previous_version", "Version of wallet before this operation"},
                {RPCResult::Type::NUM, "current_version", "Version of wallet after this operation"},
                {RPCResult::Type::STR, "result", /* optional */ true, "Description of result, if no error"},
                {RPCResult::Type::STR, "error", /* optional */ true, "Error message (if there is one)"}
            },
        },
        RPCExamples{
            HelpExampleCli("upgradewallet", "120200")
            + HelpExampleRpc("upgradewallet", "120200")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    RPCTypeCheck(request.params, {UniValue::VNUM}, true);

    EnsureWalletIsUnlocked(*pwallet);

    int version = 0;
    if (!request.params[0].isNull()) {
        version = request.params[0].getInt<int>();
    }
    bilingual_str error;
    const int previous_version{pwallet->GetVersion()};
    const bool wallet_upgraded{pwallet->UpgradeWallet(version, error)};
    const int current_version{pwallet->GetVersion()};
    std::string result;

    if (wallet_upgraded) {
        if (previous_version == current_version) {
            result = "Already at latest version. Wallet version unchanged.";
        } else {
            result = strprintf("Wallet upgraded successfully from version %i to version %i.", previous_version, current_version);
        }
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("wallet_name", pwallet->GetName());
    obj.pushKV("previous_version", previous_version);
    obj.pushKV("current_version", current_version);
    if (!result.empty()) {
        obj.pushKV("result", result);
    } else {
        CHECK_NONFATAL(!error.empty());
        obj.pushKV("error", error.original);
    }
    return obj;
},
    };
}

RPCHelpMan simulaterawtransaction()
{
    return RPCHelpMan{"simulaterawtransaction",
        "\nCalculate the balance change resulting in the signing and broadcasting of the given transaction(s).\n",
        {
            {"rawtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "An array of hex strings of raw transactions.\n",
                {
                    {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                },
            },
            {"options", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED_NAMED_ARG, "Options",
                {
                    {"include_watchonly", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Whether to include watch-only addresses (see RPC importaddress)"},
                },
            },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_AMOUNT, "balance_change", "The wallet balance change (negative means decrease)."},
            }
        },
        RPCExamples{
            HelpExampleCli("simulaterawtransaction", "[\"myhex\"]")
            + HelpExampleRpc("simulaterawtransaction", "[\"myhex\"]")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> rpc_wallet = GetWalletForJSONRPCRequest(request);
    if (!rpc_wallet) return UniValue::VNULL;
    const CWallet& wallet = *rpc_wallet;

    RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VOBJ}, true);

    LOCK(wallet.cs_wallet);

    UniValue include_watchonly(UniValue::VNULL);
    if (request.params[1].isObject()) {
        UniValue options = request.params[1];
        RPCTypeCheckObj(options,
            {
                {"include_watchonly", UniValueType(UniValue::VBOOL)},
            },
            true, true);

        include_watchonly = options["include_watchonly"];
    }

    isminefilter filter = ISMINE_SPENDABLE;
    if (ParseIncludeWatchonly(include_watchonly, wallet)) {
        filter |= ISMINE_WATCH_ONLY;
    }

    const auto& txs = request.params[0].get_array();
    CAmount changes{0};
    std::map<COutPoint, CAmount> new_utxos; // UTXO:s that were made available in transaction array
    std::set<COutPoint> spent;

    for (size_t i = 0; i < txs.size(); ++i) {
        CMutableTransaction mtx;
        if (!DecodeHexTx(mtx, txs[i].get_str())) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Transaction hex string decoding failure.");
        }

        // Fetch previous transactions (inputs)
        std::map<COutPoint, Coin> coins;
        for (const CTxIn& txin : mtx.vin) {
            coins[txin.prevout]; // Create empty map entry keyed by prevout.
        }
        wallet.chain().findCoins(coins);

        // Fetch debit; we are *spending* these; if the transaction is signed and
        // broadcast, we will lose everything in these
        for (const auto& txin : mtx.vin) {
            const auto& outpoint = txin.prevout;
            if (spent.count(outpoint)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Transaction(s) are spending the same output more than once");
            }
            if (new_utxos.count(outpoint)) {
                changes -= new_utxos.at(outpoint);
                new_utxos.erase(outpoint);
            } else {
                if (coins.at(outpoint).IsSpent()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "One or more transaction inputs are missing or have been spent already");
                }
                changes -= wallet.GetDebit(txin, filter);
            }
            spent.insert(outpoint);
        }

        // Iterate over outputs; we are *receiving* these, if the wallet considers
        // them "mine"; if the transaction is signed and broadcast, we will receive
        // everything in these
        // Also populate new_utxos in case these are spent in later transactions

        const auto& hash = mtx.GetHash();
        for (size_t i = 0; i < mtx.vout.size(); ++i) {
            const auto& txout = mtx.vout[i];
            bool is_mine = 0 < (wallet.IsMine(txout) & filter);
            changes += new_utxos[COutPoint(hash, i)] = is_mine ? txout.nValue : 0;
        }
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("balance_change", ValueFromAmount(changes));

    return result;
}
    };
}

// addresses
RPCHelpMan getaddressinfo();
RPCHelpMan getnewaddress();
RPCHelpMan getrawchangeaddress();
RPCHelpMan setlabel();
RPCHelpMan listaddressgroupings();
RPCHelpMan addmultisigaddress();
RPCHelpMan keypoolrefill();
RPCHelpMan newkeypool();
RPCHelpMan getaddressesbylabel();
RPCHelpMan listlabels();

// backup
RPCHelpMan dumpprivkey();
RPCHelpMan importprivkey();
RPCHelpMan importaddress();
RPCHelpMan importpubkey();
RPCHelpMan dumpwallet();
RPCHelpMan importwallet();
RPCHelpMan importprunedfunds();
RPCHelpMan removeprunedfunds();
RPCHelpMan importmulti();
RPCHelpMan importdescriptors();
RPCHelpMan listdescriptors();
RPCHelpMan backupwallet();
RPCHelpMan restorewallet();
RPCHelpMan dumphdinfo();
RPCHelpMan importelectrumwallet();

// coins
RPCHelpMan getreceivedbyaddress();
RPCHelpMan getreceivedbylabel();
RPCHelpMan getbalance();
RPCHelpMan getunconfirmedbalance();
RPCHelpMan lockunspent();
RPCHelpMan listlockunspent();
RPCHelpMan getbalances();
RPCHelpMan listunspent();

// encryption
RPCHelpMan walletpassphrase();
RPCHelpMan walletpassphrasechange();
RPCHelpMan walletlock();
RPCHelpMan encryptwallet();

// spend
RPCHelpMan sendtoaddress();
RPCHelpMan sendmany();
RPCHelpMan settxfee();
RPCHelpMan fundrawtransaction();
RPCHelpMan send();
RPCHelpMan walletprocesspsbt();
RPCHelpMan walletcreatefundedpsbt();
RPCHelpMan signrawtransactionwithwallet();

// signmessage
RPCHelpMan signmessage();

// transactions
RPCHelpMan listreceivedbyaddress();
RPCHelpMan listreceivedbylabel();
RPCHelpMan listtransactions();
RPCHelpMan listsinceblock();
RPCHelpMan gettransaction();
RPCHelpMan abandontransaction();
RPCHelpMan rescanblockchain();
RPCHelpMan abortrescan();

Span<const CRPCCommand> GetWalletRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  ------------------    ------------------------
    { "rawtransactions",    &fundrawtransaction,             },
    { "wallet",             &abandontransaction,             },
    { "wallet",             &abortrescan,                    },
    { "wallet",             &addmultisigaddress,             },
    { "wallet",             &backupwallet,                   },
    { "wallet",             &createwallet,                   },
    { "wallet",             &restorewallet,                  },
    { "wallet",             &dumphdinfo,                     },
    { "wallet",             &dumpprivkey,                    },
    { "wallet",             &dumpwallet,                     },
    { "wallet",             &encryptwallet,                  },
    { "wallet",             &getaddressesbylabel,            },
    { "wallet",             &getaddressinfo,                 },
    { "wallet",             &getbalance,                     },
    { "wallet",             &getnewaddress,                  },
    { "wallet",             &getrawchangeaddress,            },
    { "wallet",             &getreceivedbyaddress,           },
    { "wallet",             &getreceivedbylabel,             },
    { "wallet",             &gettransaction,                 },
    { "wallet",             &getunconfirmedbalance,          },
    { "wallet",             &getbalances,                    },
    { "wallet",             &getwalletinfo,                  },
    { "wallet",             &importaddress,                  },
    { "wallet",             &importelectrumwallet,           },
    { "wallet",             &importdescriptors,              },
    { "wallet",             &importmulti,                    },
    { "wallet",             &importprivkey,                  },
    { "wallet",             &importprunedfunds,              },
    { "wallet",             &importpubkey,                   },
    { "wallet",             &importwallet,                   },
    { "wallet",             &keypoolrefill,                  },
    { "wallet",             &listaddressbalances,            },
    { "wallet",             &listaddressgroupings,           },
    { "wallet",             &listdescriptors,                },
    { "wallet",             &listlabels,                     },
    { "wallet",             &listlockunspent,                },
    { "wallet",             &listreceivedbyaddress,          },
    { "wallet",             &listreceivedbylabel,            },
    { "wallet",             &listsinceblock,                 },
    { "wallet",             &listtransactions,               },
    { "wallet",             &listunspent,                    },
    { "wallet",             &listwalletdir,                  },
    { "wallet",             &listwallets,                    },
    { "wallet",             &loadwallet,                     },
    { "wallet",             &lockunspent,                    },
    { "wallet",             &newkeypool,                     },
    { "wallet",             &removeprunedfunds,              },
    { "wallet",             &rescanblockchain,               },
    { "wallet",             &send,                           },
    { "wallet",             &sendmany,                       },
    { "wallet",             &sendtoaddress,                  },
    { "wallet",             &sethdseed,                      },
    { "wallet",             &setcoinjoinrounds,              },
    { "wallet",             &setcoinjoinamount,              },
    { "wallet",             &setlabel,                       },
    { "wallet",             &settxfee,                       },
    { "wallet",             &setwalletflag,                  },
    { "wallet",             &signmessage,                    },
    { "wallet",             &signrawtransactionwithwallet,   },
    { "wallet",             &simulaterawtransaction,         },
    { "wallet",             &unloadwallet,                   },
    { "wallet",             &upgradewallet,                  },
    { "wallet",             &upgradetohd,                    },
    { "wallet",             &walletlock,                     },
    { "wallet",             &walletpassphrasechange,         },
    { "wallet",             &walletpassphrase,               },
    { "wallet",             &walletprocesspsbt,              },
    { "wallet",             &walletcreatefundedpsbt,         },
    { "wallet",             &wipewallettxes,                 },
};
// clang-format on
    return commands;
}
} // namespace wallet
