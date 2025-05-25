// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <httpserver.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <rpc/blockchain.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <util/bip32.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/url.h>
#include <util/vector.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/load.h>
#include <wallet/receive.h>
#include <wallet/rpcwallet.h>
#include <wallet/rpc/util.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>
#include <key_io.h>

#include <coinjoin/client.h>
#include <coinjoin/options.h>
#include <llmq/chainlocks.h>

#include <stdint.h>

#include <univalue.h>

#include <optional>

/** Checks if a CKey is in the given CWallet compressed or otherwise*/
bool HaveKey(const SigningProvider& wallet, const CKey& key)
{
    CKey key2;
    key2.Set(key.begin(), key.end(), !key.IsCompressed());
    return wallet.HaveKey(key.GetPubKey().GetID()) || wallet.HaveKey(key2.GetPubKey().GetID());
}

/**
 * Update coin control with fee estimation based on the given parameters
 *
 * @param[in]     wallet            Wallet reference
 * @param[in,out] cc                Coin control to be updated
 * @param[in]     conf_target       UniValue integer; confirmation target in blocks, values between 1 and 1008 are valid per policy/fees.h;
 * @param[in]     estimate_mode     UniValue string; fee estimation mode, valid values are "unset", "economical" or "conservative";
 * @param[in]     fee_rate          UniValue real; fee rate in sat/B;
 *                                      if present, both conf_target and estimate_mode must either be null, or no-op or "unset"
 * @param[in]     override_min_fee  bool; whether to set fOverrideFeeRate to true to disable minimum fee rate checks and instead
 *                                      verify only that fee_rate is greater than 0
 * @throws a JSONRPCError if conf_target, estimate_mode, or fee_rate contain invalid values or are in conflict
 */
static void SetFeeEstimateMode(const CWallet& wallet, CCoinControl& cc, const UniValue& conf_target, const UniValue& estimate_mode, const UniValue& fee_rate, bool override_min_fee)
{
    if (!fee_rate.isNull()) {
        if (!conf_target.isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both conf_target and fee_rate. Please provide either a confirmation target in blocks for automatic fee estimation, or an explicit fee rate.");
        }
        if (!estimate_mode.isNull() && estimate_mode.get_str() != "unset") {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both estimate_mode and fee_rate");
        }
        // Fee rates in sat/vB cannot represent more than 3 significant digits.
        cc.m_feerate = CFeeRate{AmountFromValue(fee_rate, /* decimals */ 3)};
        if (override_min_fee) cc.fOverrideFeeRate = true;
        return;
    }
    if (!estimate_mode.isNull() && !FeeModeFromString(estimate_mode.get_str(), cc.m_fee_mode)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, InvalidEstimateModeErrorMessage());
    }
    if (!conf_target.isNull()) {
        cc.m_confirm_target = ParseConfirmTarget(conf_target, wallet.chain().estimateMaxBlocks());
    }
}

RPCHelpMan getnewaddress()
{
    return RPCHelpMan{"getnewaddress",
        "\nReturns a new Dash address for receiving payments.\n"
        "If 'label' is specified, it is added to the address book \n"
        "so payments received with the address will be associated with 'label'.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Default{""}, "The label name for the address to be linked to. It can also be set to the empty string \"\" to represent the default label. The label does not need to exist, it will be created if there is no label by the given name."},
        },
        RPCResult{
    RPCResult::Type::STR, "address", "The new Dash address"
        },
        RPCExamples{
            HelpExampleCli("getnewaddress", "")
    + HelpExampleRpc("getnewaddress", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    if (!pwallet->CanGetAddresses()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    // Parse the label first so we don't generate a key if there's an error
    std::string label;
    if (!request.params[0].isNull())
        label = LabelFromValue(request.params[0]);

    CTxDestination dest;
    bilingual_str error;
    if (!pwallet->GetNewDestination(label, dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error.original);
    }
    return EncodeDestination(dest);
},
    };
}

RPCHelpMan getrawchangeaddress()
{
    return RPCHelpMan{"getrawchangeaddress",
        "\nReturns a new Dash address, for receiving change.\n"
        "This is for use with raw transactions, NOT normal use.\n",
        {},
        RPCResult{
            RPCResult::Type::STR, "address", "The address"
        },
        RPCExamples{
            HelpExampleCli("getrawchangeaddress", "")
    + HelpExampleRpc("getrawchangeaddress", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    if (!pwallet->CanGetAddresses(true)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    CTxDestination dest;
    bilingual_str error;
    if (!pwallet->GetNewChangeDestination(dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error.original);
    }
    return EncodeDestination(dest);
},
    };
}


static RPCHelpMan setlabel()
{
    return RPCHelpMan{"setlabel",
        "\nSets the label associated with the given address.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address to be associated with a label."},
            {"label", RPCArg::Type::STR, RPCArg::Optional::NO, "The label to assign to the address."},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("setlabel", "\"" + EXAMPLE_ADDRESS[0] + "\" \"tabby\"")
    + HelpExampleRpc("setlabel", "\"" + EXAMPLE_ADDRESS[0] + "\", \"tabby\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");
    }

    std::string label = LabelFromValue(request.params[1]);

    if (pwallet->IsMine(dest)) {
        pwallet->SetAddressBook(dest, label, "receive");
    } else {
        pwallet->SetAddressBook(dest, label, "send");
    }

    return NullUniValue;
},
    };
}

void ParseRecipients(const UniValue& address_amounts, const UniValue& subtract_fee_outputs, std::vector<CRecipient> &recipients) {
    std::set<CTxDestination> destinations;
    int i = 0;
    for (const std::string& address: address_amounts.getKeys()) {
        CTxDestination dest = DecodeDestination(address);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + address);
        }

        if (destinations.count(dest)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + address);
        }
        destinations.insert(dest);

        CScript script_pub_key = GetScriptForDestination(dest);
        CAmount amount = AmountFromValue(address_amounts[i++]);

        bool subtract_fee = false;
        for (unsigned int idx = 0; idx < subtract_fee_outputs.size(); idx++) {
            const UniValue& addr = subtract_fee_outputs[idx];
            if (addr.get_str() == address) {
                subtract_fee = true;
            }
        }

        CRecipient recipient = {script_pub_key, amount, subtract_fee};
        recipients.push_back(recipient);
    }
}

UniValue SendMoney(CWallet& wallet, const CCoinControl &coin_control, std::vector<CRecipient> &recipients, mapValue_t map_value, bool verbose)
{
    EnsureWalletIsUnlocked(wallet);

    // This function is only used by sendtoaddress and sendmany.
    // This should always try to sign, if we don't have private keys, don't try to do anything here.
    if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Private keys are disabled for this wallet");
    }

    if (coin_control.IsUsingCoinJoin()) {
        map_value["DS"] = "1";
    }

    // Send
    CAmount nFeeRequired = 0;
    int nChangePosRet = -1;
    bilingual_str error;
    CTransactionRef tx;
    FeeCalculation fee_calc_out;
    const bool fCreated = CreateTransaction(wallet, recipients, tx, nFeeRequired, nChangePosRet, error, coin_control, fee_calc_out, true);
    if (!fCreated) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, error.original);
    }
    wallet.CommitTransaction(tx, std::move(map_value), {} /* orderForm */);
    if (verbose) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", tx->GetHash().GetHex());
        entry.pushKV("fee_reason", StringForFeeReason(fee_calc_out.reason));
        return entry;
    }
    return tx->GetHash().GetHex();
}

static RPCHelpMan sendtoaddress()
{
    return RPCHelpMan{"sendtoaddress",
        "\nSend an amount to a given address." +
                HELP_REQUIRING_PASSPHRASE,
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address to send to."},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1"},
            {"comment", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A comment used to store what the transaction is for.\n"
                "This is not part of the transaction, just kept in your wallet."},
            {"comment_to", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A comment to store the name of the person or organization\n"
                "to which you're sending the transaction. This is not part of the \n"
                "transaction, just kept in your wallet."},
            {"subtractfeefromamount", RPCArg::Type::BOOL, RPCArg::Default{false}, "The fee will be deducted from the amount being sent.\n"
                "The recipient will receive less amount of Dash than you enter in the amount field."},
            {"use_is", RPCArg::Type::BOOL, RPCArg::Default{false}, "Deprecated and ignored"},
            {"use_cj", RPCArg::Type::BOOL, RPCArg::Default{false}, "Use CoinJoin funds only"},
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
            "       \"" + FeeModes("\"\n\"") + "\""},
            {"avoid_reuse", RPCArg::Type::BOOL, RPCArg::Default{true}, "(only available if avoid_reuse wallet flag is set) Avoid spending from dirty addresses; addresses are considered\n"
    "                             dirty if they have previously been used in a transaction."},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, return extra information about the transaction."},
        },
                {
                    RPCResult{"if verbose is not set or set to false",
                        RPCResult::Type::STR_HEX, "txid", "The transaction id."
                    },
                    RPCResult{"if verbose is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id."},
                            {RPCResult::Type::STR, "fee_reason", "The transaction fee reason."}
                        },
                    },
                },
                RPCExamples{
                    "\nSend 0.1 Dash\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1") +
                    "\nSend 0.1 Dash with a confirmation target of 6 blocks in economical fee estimate mode using positional arguments\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1 \"donation\" \"sean's outpost\" false false false 6 economical") +
                    "\nSend 0.1 Dash with a fee rate of 1.1 " + CURRENCY_ATOM + "/B, subtract fee from amount, use CoinJoin funds only, using positional arguments\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1 \"drinks\" \"room77\" true false true 0 \"\" 1.1") +
                    "\nSend 0.2 Dash with a confirmation target of 6 blocks in economical fee estimate mode using named arguments\n"
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.2 conf_target=6 estimate_mode=\"economical\"") +
                    "\nSend 0.5 Dash with a fee rate of 25 " + CURRENCY_ATOM + "/B using named arguments\n"
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.5 fee_rate=25")
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.5 fee_rate=25 subtractfeefromamount=false avoid_reuse=true comment=\"2 pizzas\" comment_to=\"jeremy\" verbose=true")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    // Wallet comments
    mapValue_t mapValue;
    if (!request.params[2].isNull() && !request.params[2].get_str().empty())
        mapValue["comment"] = request.params[2].get_str();
    if (!request.params[3].isNull() && !request.params[3].get_str().empty())
        mapValue["to"] = request.params[3].get_str();

    bool fSubtractFeeFromAmount = false;
    if (!request.params[4].isNull()) {
        fSubtractFeeFromAmount = request.params[4].get_bool();
    }

    CCoinControl coin_control;

    if (!request.params[6].isNull()) {
        coin_control.UseCoinJoin(request.params[6].get_bool());
    }

    coin_control.m_avoid_address_reuse = GetAvoidReuseFlag(*pwallet, request.params[9]);
    // We also enable partial spend avoidance if reuse avoidance is set.
    coin_control.m_avoid_partial_spends |= coin_control.m_avoid_address_reuse;

    SetFeeEstimateMode(*pwallet, coin_control, /* conf_target */ request.params[7], /* estimate_mode */ request.params[8], /* fee_rate */ request.params[10], /* override_min_fee */ false);

    EnsureWalletIsUnlocked(*pwallet);

    UniValue address_amounts(UniValue::VOBJ);
    const std::string address = request.params[0].get_str();
    address_amounts.pushKV(address, request.params[1]);
    UniValue subtractFeeFromAmount(UniValue::VARR);
    if (fSubtractFeeFromAmount) {
        subtractFeeFromAmount.push_back(address);
    }

    std::vector<CRecipient> recipients;
    ParseRecipients(address_amounts, subtractFeeFromAmount, recipients);
    const bool verbose{request.params[11].isNull() ? false : request.params[11].get_bool()};

    return SendMoney(*pwallet, coin_control, recipients, mapValue, verbose);
},
    };
}

static RPCHelpMan listaddressgroupings()
{
    return RPCHelpMan{"listaddressgroupings",
        "\nLists groups of addresses which have had their common ownership\n"
        "made public by common use as inputs or as the resulting change\n"
        "in past transactions\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::ARR_FIXED, "", "",
                    {
                        {RPCResult::Type::STR, "address", "The Dash address"},
                        {RPCResult::Type::STR_AMOUNT, "amount", "The amount in " + CURRENCY_UNIT},
                        {RPCResult::Type::STR, "label", /* optional */ true, "The label"},
                    }},
                }},
            }},
        RPCExamples{
            HelpExampleCli("listaddressgroupings", "")
    + HelpExampleRpc("listaddressgroupings", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    UniValue jsonGroupings(UniValue::VARR);
    std::map<CTxDestination, CAmount> balances = GetAddressBalances(*pwallet);
    for (const std::set<CTxDestination>& grouping : GetAddressGroupings(*pwallet)) {
        UniValue jsonGrouping(UniValue::VARR);
        for (const CTxDestination& address : grouping)
        {
            UniValue addressInfo(UniValue::VARR);
            addressInfo.push_back(EncodeDestination(address));
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                const auto* address_book_entry = pwallet->FindAddressBookEntry(address);
                if (address_book_entry) {
                    addressInfo.push_back(address_book_entry->GetLabel());
                }
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
},
    };
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


static CAmount GetReceived(const CWallet& wallet, const UniValue& params, bool by_label) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    std::vector<CTxDestination> address_vector;

    if (by_label) {
        // Get the set of addresses assigned to label
        std::string label = LabelFromValue(params[0]);
        address_vector = wallet.ListAddrBookAddresses(CWallet::AddrBookFilter{label});
    } else {
        // Get the address
        CTxDestination dest = DecodeDestination(params[0].get_str());
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");
        }
        CScript script_pub_key = GetScriptForDestination(dest);
        if (!wallet.IsMine(script_pub_key)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Address not found in wallet");
        }
        address_vector.emplace_back(dest);
    }

    // Minimum confirmations
    int min_depth = 1;
    if (!params[1].isNull())
        min_depth = params[1].get_int();

    bool fAddLocked = (!params[2].isNull() && params[2].get_bool());
    const bool include_immature_coinbase{params[3].isNull() ? false : params[3].get_bool()};

    // Excluding coinbase outputs is deprecated
    // It can be enabled by setting deprecatedrpc=exclude_coinbase
    const bool include_coinbase{!wallet.chain().rpcEnableDeprecated("exclude_coinbase")};

    if (include_immature_coinbase && !include_coinbase) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "include_immature_coinbase is incompatible with deprecated exclude_coinbase");
    }

    // Tally
    CAmount amount = 0;
    for (const std::pair<const uint256, CWalletTx>& wtx_pair : wallet.mapWallet) {
        const CWalletTx& wtx = wtx_pair.second;
        int depth{wallet.GetTxDepthInMainChain(wtx)};
        if (// Coinbase with less than 1 confirmation is no longer in the main chain
            (wtx.IsCoinBase() && (depth < 1 || !include_coinbase))
            || (wallet.IsTxImmatureCoinBase(wtx) && !include_immature_coinbase))
        {
            continue;
        }
        if (depth < min_depth && !(fAddLocked && wallet.IsTxLockedByInstantSend(wtx))) continue;

        for (const CTxOut& txout : wtx.tx->vout) {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && wallet.IsMine(address) && std::find(address_vector.begin(), address_vector.end(), address) != address_vector.end()) {
                amount += txout.nValue;
            }
        }
    }

    return amount;
}


static RPCHelpMan getreceivedbyaddress()
{
    return RPCHelpMan{"getreceivedbyaddress",
        "\nReturns the total amount received by the given address in transactions with at least minconf confirmations.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address for transactions."},
            {"minconf", RPCArg::Type::NUM, RPCArg::Default{1}, "Only include transactions confirmed at least this many times."},
            {"addlocked", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether to include transactions locked via InstantSend."},
            {"include_immature_coinbase", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include immature coinbase transactions."},
        },
        RPCResult{
            RPCResult::Type::STR_AMOUNT, "amount", "The total amount in " + CURRENCY_UNIT + " received at this address."
        },
        RPCExamples{
    "\nThe amount from transactions with at least 1 confirmation\n"
    + HelpExampleCli("getreceivedbyaddress", "\"" + EXAMPLE_ADDRESS[0] + "\"") +
    "\nThe amount including unconfirmed transactions, zero confirmations\n"
    + HelpExampleCli("getreceivedbyaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0") +
    "\nThe amount with at least 6 confirmations\n"
    + HelpExampleCli("getreceivedbyaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 6") +
    "\nThe amount with at least 6 confirmations including immature coinbase outputs\n"
    + HelpExampleCli("getreceivedbyaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 6 true true") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("getreceivedbyaddress", "\"" + EXAMPLE_ADDRESS[0] + "\", 6")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    return ValueFromAmount(GetReceived(*pwallet, request.params, /* by_label */ false));
},
    };
}


static RPCHelpMan getreceivedbylabel()
{
    return RPCHelpMan{"getreceivedbylabel",
        "\nReturns the total amount received by addresses with <label> in transactions with specified minimum number of confirmations.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Optional::NO, "The selected label, may be the default label using \"\"."},
            {"minconf", RPCArg::Type::NUM, RPCArg::Default{1}, "Only include transactions confirmed at least this many times."},
            {"addlocked", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether to include transactions locked via InstantSend."},
            {"include_immature_coinbase", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include immature coinbase transactions."},
        },
        RPCResult{
            RPCResult::Type::STR_AMOUNT, "amount", "The total amount in " + CURRENCY_UNIT + " received for this label."
        },
        RPCExamples{
    "\nAmount received by the default label with at least 1 confirmation\n"
    + HelpExampleCli("getreceivedbylabel", "\"\"") +
    "\nAmount received at the tabby label including unconfirmed amounts with zero confirmations\n"
    + HelpExampleCli("getreceivedbylabel", "\"tabby\" 0") +
    "\nThe amount with at least 6 confirmations\n"
    + HelpExampleCli("getreceivedbylabel", "\"tabby\" 6") +
    "\nThe amount with at least 6 confirmations including immature coinbase outputs\n"
    + HelpExampleCli("getreceivedbylabel", "\"tabby\" 6 true true") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("getreceivedbylabel", "\"tabby\", 6")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    return ValueFromAmount(GetReceived(*pwallet, request.params, /* by_label */ true));
},
    };
}


static RPCHelpMan getbalance()
{
    return RPCHelpMan{"getbalance",
        "\nReturns the total available balance.\n"
        "The available balance is what the wallet considers currently spendable, and is\n"
        "thus affected by options which limit spendability such as -spendzeroconfchange.\n",
        {
            {"dummy", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Remains for backward compatibility. Must be excluded or set to \"*\"."},
            {"minconf", RPCArg::Type::NUM, RPCArg::Default{0}, "Only include transactions confirmed at least this many times."},
            {"addlocked", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether to include transactions locked via InstantSend in the wallet's balance."},
            {"include_watchonly", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also include balance in watch-only addresses (see 'importaddress')"},
            {"avoid_reuse", RPCArg::Type::BOOL, RPCArg::Default{true}, "(only available if avoid_reuse wallet flag is set) Do not include balance in dirty outputs; addresses are considered dirty if they have previously been used in a transaction. If true, this also activates avoidpartialspends, grouping outputs by their addresses."},
        },
        RPCResult{
            RPCResult::Type::STR_AMOUNT, "amount", "The total amount in " + CURRENCY_UNIT + " received for this wallet."
        },
        RPCExamples{
    "\nThe total amount in the wallet with 0 or more confirmations\n"
    + HelpExampleCli("getbalance", "") +
    "\nThe total amount in the wallet with at least 6 confirmations\n"
    + HelpExampleCli("getbalance", "\"*\" 6") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("getbalance", "\"*\", 6")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    const UniValue& dummy_value = request.params[0];
    if (!dummy_value.isNull() && dummy_value.get_str() != "*") {
        throw JSONRPCError(RPC_METHOD_DEPRECATED, "dummy first argument must be excluded or set to \"*\".");
    }

    int min_depth = 0;
    if (!request.params[1].isNull()) {
        min_depth = request.params[1].get_int();
    }

    const UniValue& addlocked = request.params[2];
    bool fAddLocked = false;
    if (!addlocked.isNull()) {
        fAddLocked = addlocked.get_bool();
    }

    bool include_watchonly = ParseIncludeWatchonly(request.params[3], *pwallet);

    bool avoid_reuse = GetAvoidReuseFlag(*pwallet, request.params[4]);
    const auto bal = GetBalance(*pwallet, min_depth, avoid_reuse, fAddLocked);

    return ValueFromAmount(bal.m_mine_trusted + (include_watchonly ? bal.m_watchonly_trusted : 0));
},
    };
}

static RPCHelpMan getunconfirmedbalance()
{
    return RPCHelpMan{"getunconfirmedbalance",
        "DEPRECATED\nIdentical to getbalances().mine.untrusted_pending\n",
        {},
        RPCResult{RPCResult::Type::NUM, "", "The balance"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    return ValueFromAmount(GetBalance(*pwallet).m_mine_untrusted_pending);
},
    };
}


static RPCHelpMan sendmany()
{
    return RPCHelpMan{"sendmany",
                "\nSend multiple times. Amounts are double-precision floating point numbers." +
                        HELP_REQUIRING_PASSPHRASE,
                {
                    {"dummy", RPCArg::Type::STR, RPCArg::Optional::NO, "Must be set to \"\" for backwards compatibility.", "\"\""},
                    {"amounts", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::NO, "The addresses and amounts",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The Dash address is the key, the numeric amount (can be string) in " + CURRENCY_UNIT + " is the value"},
                        },
                    },
                    {"minconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Ignored dummy value"},
                    {"addlocked", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Ignored dummy value"},
                    {"comment", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A comment"},
                    {"subtractfeefrom", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "The addresses.\n"
                        "The fee will be equally deducted from the amount of each selected address.\n"
                        "Those recipients will receive less Dash than you enter in their corresponding amount field.\n"
                        "If no addresses are specified here, the sender pays the fee.",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Subtract fee from this address"},
                        },
                    },
                    {"use_is", RPCArg::Type::BOOL, RPCArg::Default{false}, "Deprecated and ignored"},
                    {"use_cj", RPCArg::Type::BOOL, RPCArg::Default{false}, "Use CoinJoin funds only"},
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
            "       \"" + FeeModes("\"\n\"") + "\""},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
                    {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, return extra infomration about the transaction."},
                },
                {
                    RPCResult{"if verbose is not set or set to false",
                        RPCResult::Type::STR_HEX, "txid", "The transaction id for the send. Only 1 transaction is created regardless of\n"
                "the number of addresses."
                    },
                    RPCResult{"if verbose is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id for the send. Only 1 transaction is created regardless of\n"
                "the number of addresses."},
                            {RPCResult::Type::STR, "fee_reason", "The transaction fee reason."}
                        },
                    },
                },
                RPCExamples{
            "\nSend two amounts to two different addresses:\n"
            + HelpExampleCli("sendmany", "\"\" \"{\\\"" + EXAMPLE_ADDRESS[0] + "\\\":0.01,\\\"" + EXAMPLE_ADDRESS[1] + "\\\":0.02}\"") +
            "\nSend two amounts to two different addresses setting the confirmation and comment:\n"
            + HelpExampleCli("sendmany", "\"\" \"{\\\"" + EXAMPLE_ADDRESS[0] + "\\\":0.01,\\\"" + EXAMPLE_ADDRESS[1] + "\\\":0.02}\" 6 false \"testing\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("sendmany", "\"\", \"{\\\"" + EXAMPLE_ADDRESS[0] + "\\\":0.01,\\\"" + EXAMPLE_ADDRESS[1] + "\\\":0.02}\", 6, false, \"testing\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    if (!request.params[0].isNull() && !request.params[0].get_str().empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Dummy value must be set to \"\"");
    }
    UniValue sendTo = request.params[1].get_obj();
    mapValue_t mapValue;
    if (!request.params[4].isNull() && !request.params[4].get_str().empty())
        mapValue["comment"] = request.params[4].get_str();

    UniValue subtractFeeFromAmount(UniValue::VARR);
    if (!request.params[5].isNull())
        subtractFeeFromAmount = request.params[5].get_array();

    // request.params[6] ("use_is") is deprecated and not used here

    CCoinControl coin_control;

    if (!request.params[7].isNull()) {
        coin_control.UseCoinJoin(request.params[7].get_bool());
    }

    SetFeeEstimateMode(*pwallet, coin_control, /* conf_target */ request.params[8], /* estimate_mode */ request.params[9], /* fee_rate */ request.params[10], /* override_min_fee */ false);

    std::vector<CRecipient> recipients;
    ParseRecipients(sendTo, subtractFeeFromAmount, recipients);
    const bool verbose{request.params[11].isNull() ? false : request.params[11].get_bool()};

    return SendMoney(*pwallet, coin_control, recipients, std::move(mapValue), verbose);
},
    };
}

RPCHelpMan addmultisigaddress()
{
    return RPCHelpMan{"addmultisigaddress",
        "\nAdd an nrequired-to-sign multisignature address to the wallet. Requires a new wallet backup.\n"
        "Each key is a Dash address or hex-encoded public key.\n"
        "This functionality is only intended for use with non-watchonly addresses.\n"
        "See `importaddress` for watchonly p2sh address support.\n"
        "If 'label' is specified, assign address to that label.\n",
        {
            {"nrequired", RPCArg::Type::NUM, RPCArg::Optional::NO, "The number of required signatures out of the n keys or addresses."},
            {"keys", RPCArg::Type::ARR, RPCArg::Optional::NO, "The Dash addresses or hex-encoded public keys",
                {
                    {"key", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Dash address or hex-encoded public key"},
                },
                },
            {"label", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A label to assign the addresses to."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "address", "The value of the new multisig address"},
                {RPCResult::Type::STR_HEX, "redeemScript", "The string value of the hex-encoded redemption script"},
                {RPCResult::Type::STR, "descriptor", "The descriptor for this multisig."},
            }
        },
        RPCExamples{
    "\nAdd a multisig address from 2 addresses\n"
    + HelpExampleCli("addmultisigaddress", "2 \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("addmultisigaddress", "2, \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    std::string label;
    if (!request.params[2].isNull())
        label = LabelFromValue(request.params[2]);

    int required = request.params[0].get_int();

    // Get the public keys
    const UniValue& keys_or_addrs = request.params[1].get_array();
    std::vector<CPubKey> pubkeys;
    for (unsigned int i = 0; i < keys_or_addrs.size(); ++i) {
        if (IsHex(keys_or_addrs[i].get_str()) && (keys_or_addrs[i].get_str().length() == 66 || keys_or_addrs[i].get_str().length() == 130)) {
            pubkeys.push_back(HexToPubKey(keys_or_addrs[i].get_str()));
        } else {
            pubkeys.push_back(AddrToPubKey(spk_man, keys_or_addrs[i].get_str()));
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner;
    CTxDestination dest = AddAndGetMultisigDestination(required, pubkeys, spk_man, inner);
    pwallet->SetAddressBook(dest, label, "send");

    // Make the descriptor
    std::unique_ptr<Descriptor> descriptor = InferDescriptor(GetScriptForDestination(dest), spk_man);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", EncodeDestination(dest));
    result.pushKV("redeemScript", HexStr(inner));
    result.pushKV("descriptor", descriptor->ToString());
    return result;
},
    };
}


static RPCHelpMan keypoolrefill()
{
    return RPCHelpMan{"keypoolrefill",
        "\nFills the keypool."+
                HELP_REQUIRING_PASSPHRASE,
        {
            {"newsize", RPCArg::Type::NUM, RPCArg::DefaultHint{strprintf("%u, or as set by -keypool", DEFAULT_KEYPOOL_SIZE)}, "The new keypool size"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("keypoolrefill", "")
    + HelpExampleRpc("keypoolrefill", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    if (pwallet->IsLegacy() && pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Private keys are disabled for this wallet");
    }

    LOCK(pwallet->cs_wallet);

    // 0 is interpreted by TopUpKeyPool() as the default keypool size given by -keypool
    unsigned int kpSize = 0;
    if (!request.params[0].isNull()) {
        if (request.params[0].get_int() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size.");
        kpSize = (unsigned int)request.params[0].get_int();
    }

    EnsureWalletIsUnlocked(*pwallet);
    pwallet->TopUpKeyPool(kpSize);

    if (pwallet->GetKeyPoolSize() < (pwallet->IsHDEnabled() ? kpSize * 2 : kpSize)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");
    }

    return NullUniValue;
},
    };
}


static RPCHelpMan newkeypool()
{
    return RPCHelpMan{"newkeypool",
                "\nEntirely clears and refills the keypool.\n"
                "WARNING: On non-HD wallets, this will require a new backup immediately, to include the new keys.\n"
                "When restoring a backup of an HD wallet created before the newkeypool command is run, funds received to\n"
                "new addresses may not appear automatically. They have not been lost, but the wallet may not find them.\n"
                "This can be fixed by running the newkeypool command on the backup and then rescanning, so the wallet\n"
                "re-generates the required keys." +
            HELP_REQUIRING_PASSPHRASE,
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            HelpExampleCli("newkeypool", "")
            + HelpExampleRpc("newkeypool", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet, true);
    spk_man.NewKeyPool();

    return NullUniValue;
},
    };
}


static RPCHelpMan lockunspent()
{
    return RPCHelpMan{"lockunspent",
        "\nUpdates list of temporarily unspendable outputs.\n"
        "Temporarily lock (unlock=false) or unlock (unlock=true) specified transaction outputs.\n"
        "If no transaction outputs are specified when unlocking then all current locked transaction outputs are unlocked.\n"
        "A locked transaction output will not be chosen by automatic coin selection, when spending Dash.\n"
        "Manually selected coins are automatically unlocked.\n"
        "Locks are stored in memory only, unless persistent=true, in which case they will be written to the\n"
        "wallet database and loaded on node start. Unwritten (persistent=false) locks are always cleared\n"
        "(by virtue of process exit) when a node stops or fails. Unlocking will clear both persistent and not.\n"
        "Also see the listunspent call\n",
        {
            {"unlock", RPCArg::Type::BOOL, RPCArg::Optional::NO, "Whether to unlock (true) or lock (false) the specified transactions"},
            {"transactions", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The transaction outputs and within each, the txid (string) vout (numeric).",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                        },
                    },
                },
            },
            {"persistent", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether to write/erase this lock in the wallet database, or keep the change in memory only. Ignored for unlocking."},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "Whether the command was successful or not"
        },
        RPCExamples{
    "\nList the unspent transactions\n"
    + HelpExampleCli("listunspent", "") +
    "\nLock an unspent transaction\n"
    + HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
    "\nList the locked transactions\n"
    + HelpExampleCli("listlockunspent", "") +
    "\nUnlock the transaction again\n"
    + HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
    "\nLock the transaction persistently in the wallet database\n"
    + HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\" true") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("lockunspent", "false, \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    RPCTypeCheckArgument(request.params[0], UniValue::VBOOL);

    bool fUnlock = request.params[0].get_bool();

    const bool persistent{request.params[2].isNull() ? false : request.params[2].get_bool()};

    if (request.params[1].isNull()) {
        if (fUnlock) {
            if (!pwallet->UnlockAllCoins())
                throw JSONRPCError(RPC_WALLET_ERROR, "Unlocking coins failed");
        }
        return true;
    }

    RPCTypeCheckArgument(request.params[1], UniValue::VARR);

    const UniValue& output_params = request.params[1];

    // Create and validate the COutPoints first.

    std::vector<COutPoint> outputs;
    outputs.reserve(output_params.size());

    for (unsigned int idx = 0; idx < output_params.size(); idx++) {
        const UniValue& o = output_params[idx].get_obj();

        RPCTypeCheckObj(o,
            {
                {"txid", UniValueType(UniValue::VSTR)},
                {"vout", UniValueType(UniValue::VNUM)},
            });

        const uint256 txid(ParseHashO(o, "txid"));
        const int nOutput = find_value(o, "vout").get_int();
        if (nOutput < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout cannot be negative");
        }

        const COutPoint outpt(txid, nOutput);

        const auto it = pwallet->mapWallet.find(outpt.hash);
        if (it == pwallet->mapWallet.end()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, unknown transaction");
        }

        const CWalletTx& trans = it->second;

        if (outpt.n >= trans.tx->vout.size()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout index out of bounds");
        }

        if (pwallet->IsSpent(outpt.hash, outpt.n)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected unspent output");
        }

        const bool is_locked = pwallet->IsLockedCoin(outpt.hash, outpt.n);

        if (fUnlock && !is_locked) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected locked output");
        }

        if (!fUnlock && is_locked && !persistent) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, output already locked");
        }

        outputs.push_back(outpt);
    }

    std::unique_ptr<WalletBatch> batch = nullptr;
    // Unlock is always persistent
    if (fUnlock || persistent) batch = std::make_unique<WalletBatch>(pwallet->GetDatabase());

    // Atomically set (un)locked status for the outputs.
    for (const COutPoint& outpt : outputs) {
        if (fUnlock) {
            if (!pwallet->UnlockCoin(outpt, batch.get())) throw JSONRPCError(RPC_WALLET_ERROR, "Unlocking coin failed");
        } else {
            if (!pwallet->LockCoin(outpt, batch.get())) throw JSONRPCError(RPC_WALLET_ERROR, "Locking coin failed");
        }
    }

    return true;
},
    };
}

static RPCHelpMan listlockunspent()
{
    return RPCHelpMan{"listlockunspent",
        "\nReturns list of temporarily unspendable outputs.\n"
        "See the lockunspent call to lock and unlock transactions for spending.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "The transaction id locked"},
                    {RPCResult::Type::NUM, "vout", "The vout value"},
                }},
            }
        },
        RPCExamples{
    "\nList the unspent transactions\n"
    + HelpExampleCli("listunspent", "") +
    "\nLock an unspent transaction\n"
    + HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
    "\nList the locked transactions\n"
    + HelpExampleCli("listlockunspent", "") +
    "\nUnlock the transaction again\n"
    + HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("listlockunspent", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    UniValue ret(UniValue::VARR);
    for (const COutPoint& outpt : pwallet->ListLockedCoins()) {
        UniValue o(UniValue::VOBJ);
        o.pushKV("txid", outpt.hash.GetHex());
        o.pushKV("vout", (int)outpt.n);
        ret.push_back(o);
    }

    return ret;
},
    };
}

static RPCHelpMan settxfee()
{
    return RPCHelpMan{"settxfee",
        "\nSet the transaction fee rate in " + CURRENCY_UNIT + "/kB for this wallet. Overrides the global -paytxfee command line parameter.\n"
        "Can be deactivated by passing 0 as the fee. In that case automatic fee selection will be used by default.\n",
        {
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The transaction fee rate in " + CURRENCY_UNIT + "/kB"},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "Returns true if successful"
        },
        RPCExamples{
            HelpExampleCli("settxfee", "0.00001")
    + HelpExampleRpc("settxfee", "0.00001")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    CAmount nAmount = AmountFromValue(request.params[0]);
    CFeeRate tx_fee_rate(nAmount, 1000);
    CFeeRate max_tx_fee_rate(pwallet->m_default_max_tx_fee, 1000);
    if (tx_fee_rate == CFeeRate(0)) {
        // automatic selection
    } else if (tx_fee_rate < pwallet->chain().relayMinFee()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("txfee cannot be less than min relay tx fee (%s)", pwallet->chain().relayMinFee().ToString()));
    } else if (tx_fee_rate < pwallet->m_min_fee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("txfee cannot be less than wallet min fee (%s)", pwallet->m_min_fee.ToString()));
    } else if (tx_fee_rate > max_tx_fee_rate) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("txfee cannot be more than wallet max tx fee (%s)", max_tx_fee_rate.ToString()));
    }

    pwallet->m_pay_tx_fee = tx_fee_rate;
    return true;
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

    int nRounds = request.params[0].get_int();

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

    int nAmount = request.params[0].get_int();

    if (nAmount > MAX_COINJOIN_AMOUNT || nAmount < MIN_COINJOIN_AMOUNT)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount of " + CURRENCY_UNIT + " as mixing goal amount");

    CCoinJoinClientOptions::SetAmount(nAmount);

    return NullUniValue;
},
    };
}

static RPCHelpMan getbalances()
{
    return RPCHelpMan{"getbalances",
                "Returns an object with all balances in " + CURRENCY_UNIT + ".\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ, "mine", "balances from outputs that the wallet can sign",
                            {
                                {RPCResult::Type::STR_AMOUNT, "trusted", " trusted balance (outputs created by the wallet or confirmed outputs)"},
                                {RPCResult::Type::STR_AMOUNT, "untrusted_pending", " untrusted pending balance (outputs created by others that are in the mempool)"},
                                {RPCResult::Type::STR_AMOUNT, "immature", " balance from immature coinbase outputs"},
                                {RPCResult::Type::STR_AMOUNT, "used", /* optional */ true, "(only present if avoid_reuse is set) balance from coins sent to addresses that were previously spent from (potentially privacy violating)"},
                                {RPCResult::Type::STR_AMOUNT, "coinjoin", " CoinJoin balance (outputs with enough rounds created by the wallet via mixing)"},
                        }},
                        {RPCResult::Type::OBJ, "watchonly", /* optional */ true, "watchonly balances (not present if wallet does not watch anything)",
                            {
                                {RPCResult::Type::STR_AMOUNT, "trusted", " trusted balance (outputs created by the wallet or confirmed outputs)"},
                                {RPCResult::Type::STR_AMOUNT, "untrusted_pending", " untrusted pending balance (outputs created by others that are in the mempool)"},
                                {RPCResult::Type::STR_AMOUNT, "immature", " balance from immature coinbase outputs"},
                        }},
                    },
                },
                RPCExamples{
                    HelpExampleCli("getbalances", "") +
                    HelpExampleRpc("getbalances", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> rpc_wallet = GetWalletForJSONRPCRequest(request);
    if (!rpc_wallet) return NullUniValue;
    const CWallet& wallet = *rpc_wallet;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    LOCK(wallet.cs_wallet);

    const auto bal = GetBalance(wallet);
    UniValue balances{UniValue::VOBJ};
    {
        UniValue balances_mine{UniValue::VOBJ};
        balances_mine.pushKV("trusted", ValueFromAmount(bal.m_mine_trusted));
        balances_mine.pushKV("untrusted_pending", ValueFromAmount(bal.m_mine_untrusted_pending));
        balances_mine.pushKV("immature", ValueFromAmount(bal.m_mine_immature));
        if (wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
            // If the AVOID_REUSE flag is set, bal has been set to just the un-reused address balance. Get
            // the total balance, and then subtract bal to get the reused address balance.
            const auto full_bal = GetBalance(wallet, 0, false);
            balances_mine.pushKV("used", ValueFromAmount(full_bal.m_mine_trusted + full_bal.m_mine_untrusted_pending - bal.m_mine_trusted - bal.m_mine_untrusted_pending));
        }
        balances_mine.pushKV("coinjoin", ValueFromAmount(bal.m_anonymized));
        balances.pushKV("mine", balances_mine);
    }
    auto spk_man = wallet.GetLegacyScriptPubKeyMan();
    if (spk_man && spk_man->HaveWatchOnly()) {
        UniValue balances_watchonly{UniValue::VOBJ};
        balances_watchonly.pushKV("trusted", ValueFromAmount(bal.m_watchonly_trusted));
        balances_watchonly.pushKV("untrusted_pending", ValueFromAmount(bal.m_watchonly_untrusted_pending));
        balances_watchonly.pushKV("immature", ValueFromAmount(bal.m_watchonly_immature));
        balances.pushKV("watchonly", balances_watchonly);
    }
    return balances;
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
    obj.pushKV("keypoolsize",   (int64_t)kpExternalSize);
    obj.pushKV("keypoolsize_hd_internal",   (int64_t)(pwallet->GetKeyPoolSize() - kpExternalSize));
    obj.pushKV("keys_left",     pwallet->nKeysLeftSinceAutoBackup);
    if (pwallet->IsCrypted())
        obj.pushKV("unlocked_until", pwallet->nRelockTime);
    obj.pushKV("paytxfee",      ValueFromAmount(pwallet->m_pay_tx_fee.GetFeePerK()));
    if (fHDEnabled) {
        obj.pushKV("hdchainid", hdChainCurrent.GetID().GetHex());
        obj.pushKV("hdaccountcount", (int64_t)hdChainCurrent.CountAccounts());
        UniValue accounts(UniValue::VARR);
        for (size_t i = 0; i < hdChainCurrent.CountAccounts(); ++i)
        {
            CHDAccount acc;
            UniValue account(UniValue::VOBJ);
            account.pushKV("hdaccountindex", (int64_t)i);
            if(hdChainCurrent.GetAccount(i, acc)) {
                account.pushKV("hdexternalkeyindex", (int64_t)acc.nExternalChainCounter);
                account.pushKV("hdinternalkeyindex", (int64_t)acc.nInternalChainCounter);
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
    SecureString secureWalletPassphrase;
    secureWalletPassphrase.reserve(100);

    if (request.params[2].isNull()) {
        if (pwallet->IsCrypted()) {
            throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet encrypted but passphrase not supplied to RPC.");
        }
    } else {
        // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
        // Alternately, find a way to make request.params[0] mlock()'d to begin with.
        secureWalletPassphrase = request.params[2].get_str().c_str();
    }

    SecureString secureMnemonic;
    secureMnemonic.reserve(256);
    if (!generate_mnemonic) {
        secureMnemonic = request.params[0].get_str().c_str();
    }

    SecureString secureMnemonicPassphrase;
    secureMnemonicPassphrase.reserve(256);
    if (!request.params[1].isNull()) {
        secureMnemonicPassphrase = request.params[1].get_str().c_str();
    }

    // TODO: breaking changes kept for v21!
    // instead upgradetohd let's use more straightforward 'sethdseed'
    constexpr bool is_v21 = false;
    const int previous_version{pwallet->GetVersion()};
    if (is_v21 && previous_version >= FEATURE_HD) {
        return JSONRPCError(RPC_WALLET_ERROR, "Already at latest version. Wallet version unchanged.");
    }

    bilingual_str error;
    const bool wallet_upgraded{pwallet->UpgradeToHD(secureMnemonic, secureMnemonicPassphrase, secureWalletPassphrase, error)};

    if (!secureWalletPassphrase.empty() && !pwallet->IsCrypted()) {
        if (!pwallet->EncryptWallet(secureWalletPassphrase)) {
            throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Failed to encrypt HD wallet");
        }
    }

    if (!wallet_upgraded) {
        throw JSONRPCError(RPC_WALLET_ERROR, error.original);
    }

    // If you are generating new mnemonic it is assumed that the addresses have never gotten a transaction before, so you don't need to rescan for transactions
    bool rescan = request.params[3].isNull() ? !generate_mnemonic : request.params[3].get_bool();
    if (rescan) {
        WalletRescanReserver reserver(*pwallet);
        if (!reserver.reserve()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
        }
        CWallet::ScanResult result = pwallet->ScanForWalletTransactions(pwallet->chain().getBlockHash(0), 0, {}, reserver, true);
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
    std::string flags = "";
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
        passphrase = request.params[3].get_str().c_str();
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

    // Release the "main" shared pointer and prevent further notifications.
    // Note that any attempt to load the same wallet would fail until the wallet
    // is destroyed (see CheckUniqueFileid).
    std::vector<bilingual_str> warnings;
    std::optional<bool> load_on_start = request.params[1].isNull() ? std::nullopt : std::optional<bool>(request.params[1].get_bool());
    if (!RemoveWallet(context, wallet, load_on_start, warnings)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Requested wallet already unloaded");
    }

    UnloadWallet(std::move(wallet));

    UniValue result(UniValue::VOBJ);
    result.pushKV("warning", Join(warnings, Untranslated("\n")).original);
    return result;
},
    };
}

static RPCHelpMan listunspent()
{
    return RPCHelpMan{"listunspent",
        "\nReturns array of unspent transaction outputs\n"
        "with between minconf and maxconf (inclusive) confirmations.\n"
        "Optionally filter to only include txouts paid to specified addresses.\n",
        {
            {"minconf", RPCArg::Type::NUM, RPCArg::Default{1}, "The minimum confirmations to filter"},
            {"maxconf", RPCArg::Type::NUM, RPCArg::Default{9999999}, "The maximum confirmations to filter"},
            {"addresses", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The Dash addresses to filter",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Dash address"},
                },
            },
            {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{true}, "Include outputs that are not safe to spend\n"
                "See description of \"safe\" attribute below."},
            {"query_options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "JSON with query options",
                {
                    {"minimumAmount", RPCArg::Type::AMOUNT, RPCArg::Default{0}, "Minimum value of each UTXO in " + CURRENCY_UNIT + ""},
                    {"maximumAmount", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"unlimited"}, "Maximum value of each UTXO in " + CURRENCY_UNIT + ""},
                    {"maximumCount", RPCArg::Type::NUM, RPCArg::DefaultHint{"unlimited"}, "Maximum number of UTXOs"},
                    {"minimumSumAmount", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"unlimited"}, "Minimum sum value of all UTXOs in " + CURRENCY_UNIT + ""},
                    {"coinType", RPCArg::Type::NUM, RPCArg::Default{0}, "Filter coinTypes as follows:\n"
                        "0=ALL_COINS, 1=ONLY_FULLY_MIXED, 2=ONLY_READY_TO_MIX, 3=ONLY_NONDENOMINATED,\n"
                        "4=ONLY_MASTERNODE_COLLATERAL, 5=ONLY_COINJOIN_COLLATERAL" },
                },
                "query_options"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "the transaction id"},
                    {RPCResult::Type::NUM, "vout", "the vout value"},
                    {RPCResult::Type::STR, "address", /* optional */ "the Dash address"},
                    {RPCResult::Type::STR, "label", /* optional */ "The associated label, or \"\" for the default label"},
                    {RPCResult::Type::STR, "scriptPubKey", "the script key"},
                    {RPCResult::Type::STR_AMOUNT, "amount", "the transaction output amount in " + CURRENCY_UNIT},
                    {RPCResult::Type::NUM, "confirmations", "The number of confirmations"},
                    {RPCResult::Type::NUM, "ancestorcount", /* optional */ true, "The number of in-mempool ancestor transactions, including this one (if transaction is in the mempool)"},
                    {RPCResult::Type::NUM, "ancestorsize", /* optional */ true, "The virtual transaction size of in-mempool ancestors, including this one (if transaction is in the mempool)"},
                    {RPCResult::Type::STR_AMOUNT, "ancestorfees", /* optional */ true, "The total fees of in-mempool ancestors (including this one) with fee deltas used for mining priority in " + CURRENCY_ATOM + " (if transaction is in the mempool)"},
                    {RPCResult::Type::STR_HEX, "redeemScript", /* optional */ true, "The redeemScript if scriptPubKey is P2SH"},
                    {RPCResult::Type::BOOL, "spendable", "Whether we have the private keys to spend this output"},
                    {RPCResult::Type::BOOL, "solvable", "Whether we know how to spend this output, ignoring the lack of keys"},
                    {RPCResult::Type::STR, "desc", /* optional */ true, "(only when solvable) A descriptor for spending this output"},
                    {RPCResult::Type::BOOL, "reused", /* optional*/ true, "(only present if avoid_reuse is set) Whether this output is reused/dirty (sent to an address that was previously spent from)"},
                    {RPCResult::Type::BOOL, "safe", "Whether this output is considered safe to spend. Unconfirmed transactions"
                                                    "from outside keys and unconfirmed replacement transactions are considered unsafe\n"
                                                    "and are not eligible for spending by fundrawtransaction and sendtoaddress."},
                    {RPCResult::Type::NUM, "coinjoin_rounds", "The number of CoinJoin rounds"},
                }},
            }},
        RPCExamples{
            HelpExampleCli("listunspent", "")
    + HelpExampleCli("listunspent", "6 9999999 \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"")
    + HelpExampleRpc("listunspent", "6, 9999999 \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"")
    + HelpExampleCli("listunspent", "6 9999999 '[]' true '{ \"minimumAmount\": 0.005 }'")
    + HelpExampleRpc("listunspent", "6, 9999999, [] , true, { \"minimumAmount\": 0.005 } ")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    int nMinDepth = 1;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VNUM);
        nMinDepth = request.params[0].get_int();
    }

    int nMaxDepth = 9999999;
    if (!request.params[1].isNull()) {
        RPCTypeCheckArgument(request.params[1], UniValue::VNUM);
        nMaxDepth = request.params[1].get_int();
    }

    std::set<CTxDestination> destinations;
    if (!request.params[2].isNull()) {
        RPCTypeCheckArgument(request.params[2], UniValue::VARR);
        UniValue inputs = request.params[2].get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CTxDestination dest = DecodeDestination(input.get_str());
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + input.get_str());
            }
            if (!destinations.insert(dest).second) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + input.get_str());
            }
        }
    }

    bool include_unsafe = true;
    if (!request.params[3].isNull()) {
        RPCTypeCheckArgument(request.params[3], UniValue::VBOOL);
        include_unsafe = request.params[3].get_bool();
    }

    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 0;
    CCoinControl coinControl;
    coinControl.nCoinType = CoinType::ALL_COINS;

    if (!request.params[4].isNull()) {
        const UniValue& options = request.params[4].get_obj();

        // Note: Keep this vector up to date with the options processed below
        const std::vector<std::string> vecOptions {
            "minimumAmount",
            "maximumAmount",
            "minimumSumAmount",
            "maximumCount",
            "coinType"
        };

        for (const auto& key : options.getKeys()) {
            if (std::find(vecOptions.begin(), vecOptions.end(), key) == vecOptions.end()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid key used in query_options JSON object: ") + key);
            }
        }

        if (options.exists("minimumAmount"))
            nMinimumAmount = AmountFromValue(options["minimumAmount"]);

        if (options.exists("maximumAmount"))
            nMaximumAmount = AmountFromValue(options["maximumAmount"]);

        if (options.exists("minimumSumAmount"))
            nMinimumSumAmount = AmountFromValue(options["minimumSumAmount"]);

        if (options.exists("maximumCount"))
            nMaximumCount = options["maximumCount"].get_int64();

        if (options.exists("coinType")) {
            int64_t nCoinType = options["coinType"].get_int64();

            if (nCoinType < static_cast<int64_t>(CoinType::MIN_COIN_TYPE) || nCoinType > static_cast<int64_t>(CoinType::MAX_COIN_TYPE)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid coinType selected. Available range: %d - %d", static_cast<int64_t>(CoinType::MIN_COIN_TYPE), static_cast<int64_t>(CoinType::MAX_COIN_TYPE)));
            }

            coinControl.nCoinType = static_cast<CoinType>(nCoinType);
        }
    }

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue results(UniValue::VARR);
    std::vector<COutput> vecOutputs;
    {
        coinControl.m_avoid_address_reuse = false;
        coinControl.m_min_depth = nMinDepth;
        coinControl.m_max_depth = nMaxDepth;
        coinControl.m_include_unsafe_inputs = include_unsafe;

        LOCK(pwallet->cs_wallet);
        AvailableCoins(*pwallet, vecOutputs, &coinControl, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount);
    }

    LOCK(pwallet->cs_wallet);

    const bool avoid_reuse = pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);

    for (const COutput& out : vecOutputs) {
        CTxDestination address;
        const CScript& scriptPubKey = out.txout.scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);
        bool reused = avoid_reuse && pwallet->IsSpentKey(out.outpoint.hash, out.outpoint.n);

        if (destinations.size() && (!fValidAddress || !destinations.count(address)))
            continue;

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", out.outpoint.hash.GetHex());
        entry.pushKV("vout", (int)out.outpoint.n);

        if (fValidAddress) {
            entry.pushKV("address", EncodeDestination(address));

            const auto* address_book_entry = pwallet->FindAddressBookEntry(address);
            if (address_book_entry) {
                entry.pushKV("label", address_book_entry->GetLabel());
            }

            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);
            if (provider) {
                if (scriptPubKey.IsPayToScriptHash()) {
                    const CScriptID& hash = CScriptID(std::get<ScriptHash>(address));
                    CScript redeemScript;
                    if (provider->GetCScript(hash, redeemScript)) {
                        entry.pushKV("redeemScript", HexStr(redeemScript));
                    }
                }
            }
        }

        entry.pushKV("scriptPubKey", HexStr(scriptPubKey));
        entry.pushKV("amount", ValueFromAmount(out.txout.nValue));
        entry.pushKV("confirmations", out.depth);
        if (!out.depth) {
            size_t ancestor_count, descendant_count, ancestor_size;
            CAmount ancestor_fees;
            pwallet->chain().getTransactionAncestry(out.outpoint.hash, ancestor_count, descendant_count, &ancestor_size, &ancestor_fees);
            if (ancestor_count) {
                entry.pushKV("ancestorcount", uint64_t(ancestor_count));
                entry.pushKV("ancestorsize", uint64_t(ancestor_size));
                entry.pushKV("ancestorfees", uint64_t(ancestor_fees));
            }
        }
        entry.pushKV("spendable", out.spendable);
        entry.pushKV("solvable", out.solvable);
        if (out.solvable) {
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);
            if (provider) {
                auto descriptor = InferDescriptor(scriptPubKey, *provider);
                entry.pushKV("desc", descriptor->ToString());
            }
        }
        if (avoid_reuse) entry.pushKV("reused", reused);
        entry.pushKV("safe", out.safe);
        entry.pushKV("coinjoin_rounds", pwallet->GetRealOutpointCoinJoinRounds(out.outpoint));
        results.push_back(entry);
    }

    return results;
},
    };
}

void FundTransaction(CWallet& wallet, CMutableTransaction& tx, CAmount& fee_out, int& change_position, const UniValue& options, CCoinControl& coinControl, bool override_min_fee)
{
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    change_position = -1;
    bool lockUnspents = false;
    UniValue subtractFeeFromOutputs;
    std::set<int> setSubtractFeeFromOutputs;

    if (!options.isNull()) {
      if (options.type() == UniValue::VBOOL) {
        // backward compatibility bool only fallback
        coinControl.fAllowWatchOnly = options.get_bool();
      }
      else {
        RPCTypeCheckArgument(options, UniValue::VOBJ);
        RPCTypeCheckObj(options,
            {
                {"add_inputs", UniValueType(UniValue::VBOOL)},
                {"include_unsafe", UniValueType(UniValue::VBOOL)},
                {"add_to_wallet", UniValueType(UniValue::VBOOL)},
                {"changeAddress", UniValueType(UniValue::VSTR)},
                {"change_address", UniValueType(UniValue::VSTR)},
                {"changePosition", UniValueType(UniValue::VNUM)},
                {"change_position", UniValueType(UniValue::VNUM)},
                {"includeWatching", UniValueType(UniValue::VBOOL)},
                {"include_watching", UniValueType(UniValue::VBOOL)},
                {"inputs", UniValueType(UniValue::VARR)},
                {"lockUnspents", UniValueType(UniValue::VBOOL)},
                {"lock_unspents", UniValueType(UniValue::VBOOL)},
                {"locktime", UniValueType(UniValue::VNUM)},
                {"fee_rate", UniValueType()}, // will be checked by AmountFromValue() in SetFeeEstimateMode()
                {"feeRate", UniValueType()}, // will be checked by AmountFromValue() below
                {"psbt", UniValueType(UniValue::VBOOL)},
                {"subtractFeeFromOutputs", UniValueType(UniValue::VARR)},
                {"subtract_fee_from_outputs", UniValueType(UniValue::VARR)},
                {"conf_target", UniValueType(UniValue::VNUM)},
                {"estimate_mode", UniValueType(UniValue::VSTR)},
            },
            true, true);

        if (options.exists("add_inputs") ) {
            coinControl.m_add_inputs = options["add_inputs"].get_bool();
        }

        if (options.exists("changeAddress") || options.exists("change_address")) {
            const std::string change_address_str = (options.exists("change_address") ? options["change_address"] : options["changeAddress"]).get_str();
            CTxDestination dest = DecodeDestination(change_address_str);

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Change address must be a valid Dash address");
            }

            coinControl.destChange = dest;
        }

        if (options.exists("changePosition") || options.exists("change_position")) {
            change_position = (options.exists("change_position") ? options["change_position"] : options["changePosition"]).get_int();
        }

        const UniValue include_watching_option = options.exists("include_watching") ? options["include_watching"] : options["includeWatching"];
        coinControl.fAllowWatchOnly = ParseIncludeWatchonly(include_watching_option, wallet);

        if (options.exists("lockUnspents") || options.exists("lock_unspents")) {
            lockUnspents = (options.exists("lock_unspents") ? options["lock_unspents"] : options["lockUnspents"]).get_bool();
        }

        if (options.exists("include_unsafe")) {
            coinControl.m_include_unsafe_inputs = options["include_unsafe"].get_bool();
        }

        if (options.exists("feeRate")) {
            if (options.exists("fee_rate")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both fee_rate (" + CURRENCY_ATOM + "/B) and feeRate (" + CURRENCY_UNIT + "/kB)");
            }
            if (options.exists("conf_target")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both conf_target and feeRate. Please provide either a confirmation target in blocks for automatic fee estimation, or an explicit fee rate.");
            }
            if (options.exists("estimate_mode")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both estimate_mode and feeRate");
            }
            coinControl.m_feerate = CFeeRate(AmountFromValue(options["feeRate"]));
            coinControl.fOverrideFeeRate = true;
        }

        if (options.exists("subtractFeeFromOutputs") || options.exists("subtract_fee_from_outputs") )
            subtractFeeFromOutputs = (options.exists("subtract_fee_from_outputs") ? options["subtract_fee_from_outputs"] : options["subtractFeeFromOutputs"]).get_array();

        SetFeeEstimateMode(wallet, coinControl, options["conf_target"], options["estimate_mode"], options["fee_rate"], override_min_fee);
      }
    } else {
        // if options is null and not a bool
        coinControl.fAllowWatchOnly = ParseIncludeWatchonly(NullUniValue, wallet);
    }

    if (tx.vout.size() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "TX must have at least one output");

    if (change_position != -1 && (change_position < 0 || (unsigned int)change_position > tx.vout.size()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "changePosition out of bounds");

    for (unsigned int idx = 0; idx < subtractFeeFromOutputs.size(); idx++) {
        int pos = subtractFeeFromOutputs[idx].get_int();
        if (setSubtractFeeFromOutputs.count(pos))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, duplicated position: %d", pos));
        if (pos < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, negative position: %d", pos));
        if (pos >= int(tx.vout.size()))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, position too large: %d", pos));
        setSubtractFeeFromOutputs.insert(pos);
    }

    bilingual_str error;

    if (!FundTransaction(wallet, tx, fee_out, change_position, error, lockUnspents, setSubtractFeeFromOutputs, coinControl)) {
        throw JSONRPCError(RPC_WALLET_ERROR, error.original);
    }
}

static RPCHelpMan fundrawtransaction()
{
    return RPCHelpMan{"fundrawtransaction",
                "\nIf the transaction has no inputs, they will be automatically selected to meet its out value.\n"
                "It will add at most one change output to the outputs.\n"
                "No existing outputs will be modified unless \"subtractFeeFromOutputs\" is specified.\n"
                "Note that inputs which were signed may need to be resigned after completion since in/outputs have been added.\n"
                "The inputs added will not be signed, use signrawtransactionwithkey\n"
                " or signrawtransactionwithwallet for that.\n"
                "Note that all existing inputs must have their previous output transaction be in the wallet.\n"
                "Note that all inputs selected must be of standard form and P2SH scripts must be\n"
                "in the wallet using importaddress or addmultisigaddress (to calculate fees).\n"
                "You can see whether this is the case by checking the \"solvable\" field in the listunspent output.\n"
                "Only pay-to-pubkey, multisig, and P2SH versions thereof are currently supported for watch-only\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the raw transaction"},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "for backward compatibility: passing in a true instead of an object will result in {\"includeWatching\":true}",
                        {
                            {"add_inputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "For a transaction with existing inputs, automatically include more if they are not enough."},
                            {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                          "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                          "If that happens, you will need to fund the transaction with different inputs and republish it."},
                            {"changeAddress", RPCArg::Type::STR, RPCArg::DefaultHint{"pool address"}, "The Dash address to receive the change"},
                            {"changePosition", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                            {"includeWatching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only.\n"
                                                          "Only solvable inputs can be used. Watch-only destinations are solvable if the public key and/or output script was imported,\n"
                                                          "e.g. with 'importpubkey' or 'importmulti' with the 'pubkeys' or 'desc' field."},
                            {"lockUnspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
                            {"feeRate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_UNIT + "/kB."},
                            {"subtractFeeFromOutputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The integers.\n"
                                "The fee will be equally deducted from the amount of each specified output.\n"
                                "Those recipients will receive less Dash than you enter in their corresponding amount field.\n"
                                "If no outputs are specified here, the sender pays the fee.",
                                {
                                    {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                                },
                            },
                            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
                            "       \"" + FeeModes("\"\n\"") + "\""},
                        },
                        "options"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "hex", "The resulting raw transaction (hex-encoded string)"},
                        {RPCResult::Type::STR_AMOUNT, "fee", "Fee in " + CURRENCY_UNIT + " the resulting transaction pays"},
                        {RPCResult::Type::NUM, "changepos", "The position of the added change output, or -1"},
                    }
                                },
                                RPCExamples{
                            "\nCreate a transaction with no inputs\n"
                            + HelpExampleCli("createrawtransaction", "\"[]\" \"{\\\"myaddress\\\":0.01}\"") +
                            "\nAdd sufficient unsigned inputs to meet the output value\n"
                            + HelpExampleCli("fundrawtransaction", "\"rawtransactionhex\"") +
                            "\nSign the transaction\n"
                            + HelpExampleCli("signrawtransaction", "\"fundedtransactionhex\"") +
                            "\nSend the transaction\n"
                            + HelpExampleCli("sendrawtransaction", "\"signedtransactionhex\"")
                                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValueType(), UniValue::VBOOL});

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // parse hex string from parameter
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    CAmount fee;
    int change_position;
    CCoinControl coin_control;
    // Automatically select (additional) coins. Can be overridden by options.add_inputs.
    coin_control.m_add_inputs = true;
    FundTransaction(*pwallet, tx, fee, change_position, request.params[1], coin_control, /* override_min_fee */ true);

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(CTransaction(tx)));
    result.pushKV("fee", ValueFromAmount(fee));
    result.pushKV("changepos", change_position);

    return result;
},
    };
}

RPCHelpMan signrawtransactionwithwallet()
{
    return RPCHelpMan{"signrawtransactionwithwallet",
        "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
        "The second optional argument (may be null) is an array of previous transaction outputs that\n"
        "this transaction depends on but may not yet be in the block chain." +
                HELP_REQUIRING_PASSPHRASE,
        {
            {"hexstring", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction hex string"},
            {"prevtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "The previous dependent transaction outputs",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"scriptPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "script key"},
                            {"redeemScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2SH or P2WSH)"},
                            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount spent"},
                        },
                    },
                },
            },
            {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"ALL"}, "The signature hash type. Must be one of\n"
    "       \"ALL\"\n"
    "       \"NONE\"\n"
    "       \"SINGLE\"\n"
    "       \"ALL|ANYONECANPAY\"\n"
    "       \"NONE|ANYONECANPAY\"\n"
    "       \"SINGLE|ANYONECANPAY\""},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hex", "The hex-encoded raw transaction with signature(s)"},
                {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                {RPCResult::Type::ARR, "errors", /* optional */ true, "Script verification errors (if there are any)",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "txid", "The hash of the referenced, previous transaction"},
                        {RPCResult::Type::NUM, "vout", "The index of the output to spent and used as input"},
                        {RPCResult::Type::STR_HEX, "scriptSig", "The hex-encoded signature script"},
                        {RPCResult::Type::NUM, "sequence", "Script sequence number"},
                        {RPCResult::Type::STR, "error", "Verification or signing error related to the input"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"")
    + HelpExampleRpc("signrawtransactionwithwallet", "\"myhex\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VSTR}, true);

    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }

    // Sign the transaction
    LOCK(pwallet->cs_wallet);
    EnsureWalletIsUnlocked(*pwallet);

    // Fetch previous transactions (inputs):
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : mtx.vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    pwallet->chain().findCoins(coins);

    // Parse the prevtxs array
    ParsePrevouts(request.params[1], nullptr, coins);

    int nHashType = ParseSighashString(request.params[2]);

    // Script verification errors
    std::map<int, bilingual_str> input_errors;

    bool complete = pwallet->SignTransaction(mtx, coins, nHashType, input_errors);
    UniValue result(UniValue::VOBJ);
    SignTransactionResultToJSON(mtx, complete, coins, input_errors, result);
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
            if (keep_confirmed && wtx.m_confirm.status == CWalletTx::CONFIRMED) continue;
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

class DescribeWalletAddressVisitor
{
public:
    const SigningProvider * const provider;

    explicit DescribeWalletAddressVisitor(const SigningProvider * const _provider) : provider(_provider) {}

    UniValue operator()(const CNoDestination &dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(const PKHash& pkhash) const {
        CKeyID keyID{ToKeyID(pkhash)};
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        if (provider && provider->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }

    UniValue operator()(const ScriptHash& scriptHash) const {
        CScriptID scriptID(scriptHash);
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        if (provider && provider->GetCScript(scriptID, subscript)) {
            // Always present: script type and redeemscript
            std::vector<std::vector<unsigned char>> solutions_data;
            TxoutType whichType = Solver(subscript, solutions_data);
            obj.pushKV("script", GetTxnOutputType(whichType));
            obj.pushKV("hex", HexStr(subscript));
            if (whichType == TxoutType::MULTISIG) {
                // Also report some information on multisig scripts (which do not have a corresponding address).
                UniValue pubkeys(UniValue::VARR);
                UniValue addresses(UniValue::VARR);
                for (size_t i = 1; i < solutions_data.size() - 1; ++i) {
                    CPubKey pubkey(solutions_data[i]);
                    pubkeys.push_back(HexStr(pubkey));
                    addresses.push_back(EncodeDestination(PKHash(pubkey)));
                }
                obj.pushKV("pubkeys", std::move(pubkeys));
                obj.pushKV("addresses", std::move(addresses));
                obj.pushKV("sigsrequired", solutions_data[0][0]);
            }
        }
        return obj;
    }
};

static UniValue DescribeWalletAddress(const CWallet& wallet, const CTxDestination& dest)
{
    UniValue ret(UniValue::VOBJ);
    UniValue detail = DescribeAddress(dest);
    CScript script = GetScriptForDestination(dest);
    std::unique_ptr<SigningProvider> provider = nullptr;
    provider = wallet.GetSolvingProvider(script);
    ret.pushKVs(detail);
    ret.pushKVs(std::visit(DescribeWalletAddressVisitor(provider.get()), dest));
    return ret;
}

RPCHelpMan getaddressinfo()
{
    return RPCHelpMan{"getaddressinfo",
        "\nReturn information about the given Dash address.\n"
        "Some of the information will only be present if the address is in the active wallet.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address for which to get information."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "address", "The Dash address validated."},
                {RPCResult::Type::STR_HEX, "scriptPubKey", "The hex-encoded scriptPubKey generated by the address."},
                {RPCResult::Type::BOOL, "ismine", "If the address is yours."},
                {RPCResult::Type::BOOL, "iswatchonly", "If the address is watchonly."},
                {RPCResult::Type::BOOL, "solvable", "If we know how to spend coins sent to this address, ignoring the possible lack of private keys."},
                {RPCResult::Type::STR, "desc", /* optional */ true, "A descriptor for spending coins sent to this address (only when solvable)."},
                {RPCResult::Type::STR, "parent_desc", /* optional */ true, "The descriptor used to derive this address if this is a descriptor wallet"},
                {RPCResult::Type::BOOL, "isscript", "If the key is a script."},
                {RPCResult::Type::BOOL, "ischange", "If the address was used for change output."},
                {RPCResult::Type::STR, "script", /* optional */ true, "The output script type. Only if isscript is true and the redeemscript is known. Possible types: nonstandard, pubkey, pubkeyhash, scripthash, multisig, nulldata"},
                {RPCResult::Type::STR_HEX, "hex", /* optional */ true, "The redeemscript for the p2sh address."},
                {RPCResult::Type::ARR, "pubkeys", /* optional */ true, "Array of pubkeys associated with the known redeemscript (only if script is multisig).",
                {
                    {RPCResult::Type::STR, "pubkey", ""},
                }},
                {RPCResult::Type::ARR, "addresses", /* optional */ true, "Array of addresses associated with the known redeemscript (only if script is multisig).",
                {
                    {RPCResult::Type::STR, "address", ""},
                }},
                {RPCResult::Type::NUM, "sigsrequired", /* optional */ true, "The number of signatures required to spend multisig output (only if script is multisig)."},
                {RPCResult::Type::STR_HEX, "pubkey", /* optional */ true, "The hex value of the raw public key, for single-key addresses."},
                {RPCResult::Type::BOOL, "iscompressed", /* optional */ true, "If the pubkey is compressed."},
                {RPCResult::Type::NUM_TIME, "timestamp", /* optional */ true, "The creation time of the key, if available, expressed in " + UNIX_EPOCH_TIME + "."},
                {RPCResult::Type::STR_HEX, "hdchainid", /* optional */ true, "The ID of the HD chain."},
                {RPCResult::Type::STR, "hdkeypath", /* optional */ true, "The HD keypath, if the key is HD and available."},
                {RPCResult::Type::STR_HEX, "hdseedid", /* optional */ true, "The Hash160 of the HD seed."},
                {RPCResult::Type::STR_HEX, "hdmasterfingerprint", /* optional */ true, "The fingerprint of the master key."},
                {RPCResult::Type::ARR, "labels", "An array of labels associated with the address. Currently limited to one label but returned as an array to keep the API stable if multiple labels are enabled in the future.",
                {
                    {RPCResult::Type::STR, "label name", "Label name (defaults to \"\")."},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getaddressinfo", EXAMPLE_ADDRESS[0])
    + HelpExampleRpc("getaddressinfo", EXAMPLE_ADDRESS[0])
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string error_msg;
    CTxDestination dest = DecodeDestination(request.params[0].get_str(), error_msg);

    // Make sure the destination is valid
    if (!IsValidDestination(dest)) {
        // Set generic error message in case 'DecodeDestination' didn't set it
        if (error_msg.empty()) error_msg = "Invalid address";

        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, error_msg);
    }

    UniValue ret(UniValue::VOBJ);

    std::string currentAddress = EncodeDestination(dest);
    ret.pushKV("address", currentAddress);

    CScript scriptPubKey = GetScriptForDestination(dest);
    ret.pushKV("scriptPubKey", HexStr(scriptPubKey));

    std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);

    isminetype mine = pwallet->IsMine(dest);
    ret.pushKV("ismine", bool(mine & ISMINE_SPENDABLE));

    bool solvable = provider && IsSolvable(*provider, scriptPubKey);
    ret.pushKV("solvable", solvable);

    if (solvable) {
        ret.pushKV("desc", InferDescriptor(scriptPubKey, *provider)->ToString());
    }

    DescriptorScriptPubKeyMan* desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(pwallet->GetScriptPubKeyMan(scriptPubKey));
    if (desc_spk_man) {
        std::string desc_str;
        if (desc_spk_man->GetDescriptorString(desc_str, /* priv */ false)) {
            ret.pushKV("parent_desc", desc_str);
        }
    }

    ret.pushKV("iswatchonly", bool(mine & ISMINE_WATCH_ONLY));

    UniValue detail = DescribeWalletAddress(*pwallet, dest);
    ret.pushKVs(detail);

    ret.pushKV("ischange", ScriptIsChange(*pwallet, scriptPubKey));

    ScriptPubKeyMan* spk_man = pwallet->GetScriptPubKeyMan(scriptPubKey);
    if (spk_man) {
        if (const std::unique_ptr<CKeyMetadata> meta = spk_man->GetMetadata(dest)) {
            ret.pushKV("timestamp", meta->nCreateTime);
            CHDChain hdChainCurrent;
            LegacyScriptPubKeyMan* legacy_spk_man = pwallet->GetLegacyScriptPubKeyMan();
            if (legacy_spk_man != nullptr) {
                const PKHash *pkhash = std::get_if<PKHash>(&dest);
                // TODO: refactor to use hd_seed_id from `meta`
                if (pkhash && legacy_spk_man->HaveHDKey(ToKeyID(*pkhash), hdChainCurrent)) {
                    ret.pushKV("hdchainid", hdChainCurrent.GetID().GetHex());
                }
            }
            if (meta->has_key_origin) {
                ret.pushKV("hdkeypath", WriteHDKeypath(meta->key_origin.path));
                ret.pushKV("hdmasterfingerprint", HexStr(meta->key_origin.fingerprint));
            }
        }
    }

    // Return a `labels` array containing the label associated with the address,
    // equivalent to the `label` field above. Currently only one label can be
    // associated with an address, but we return an array so the API remains
    // stable if we allow multiple labels to be associated with an address in
    // the future.
    UniValue labels(UniValue::VARR);
    const auto* address_book_entry = pwallet->FindAddressBookEntry(dest);
    if (address_book_entry) {
        labels.push_back(address_book_entry->GetLabel());
    }
    ret.pushKV("labels", std::move(labels));

    return ret;
},
    };
}

static RPCHelpMan getaddressesbylabel()
{
    return RPCHelpMan{"getaddressesbylabel",
        "\nReturns the list of addresses assigned the specified label.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Optional::NO, "The label."},
        },
        RPCResult{
            RPCResult::Type::OBJ_DYN, "", "json object with addresses as keys",
            {
                {RPCResult::Type::OBJ, "address", "json object with information about address",
                {
                    {RPCResult::Type::STR, "purpose", "Purpose of address (\"send\" for sending address, \"receive\" for receiving address)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getaddressesbylabel", "\"tabby\"")
    + HelpExampleRpc("getaddressesbylabel", "\"tabby\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string label = LabelFromValue(request.params[0]);

    // Find all addresses that have the given label
    UniValue ret(UniValue::VOBJ);
    std::set<std::string> addresses;
    pwallet->ForEachAddrBookEntry([&](const CTxDestination& _dest, const std::string& _label, const std::string& _purpose, bool _is_change) {
        if (_is_change) return;
        if (_label == label) {
            std::string address = EncodeDestination(_dest);
            // CWallet::m_address_book is not expected to contain duplicate
            // address strings, but build a separate set as a precaution just in
            // case it does.
            bool unique = addresses.emplace(address).second;
            CHECK_NONFATAL(unique);
            // UniValue::pushKV checks if the key exists in O(N)
            // and since duplicate addresses are unexpected (checked with
            // std::set in O(log(N))), UniValue::__pushKV is used instead,
            // which currently is O(1).
            UniValue value(UniValue::VOBJ);
            value.pushKV("purpose", _purpose);
            ret.__pushKV(address, value);
        }
    });

    if (ret.empty()) {
        throw JSONRPCError(RPC_WALLET_INVALID_LABEL_NAME, std::string("No addresses with label " + label));
    }

    return ret;
},
    };
}

static RPCHelpMan listlabels()
{
    return RPCHelpMan{"listlabels",
        "\nReturns the list of all labels, or labels that are assigned to addresses with a specific purpose.\n",
        {
            {"purpose", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Address purpose to list labels for ('send','receive'). An empty string is the same as not providing this argument."},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "label", "Label name"},
            }
        },
        RPCExamples{
    "\nList all labels\n"
    + HelpExampleCli("listlabels", "") +
    "\nList labels that have receiving addresses\n"
    + HelpExampleCli("listlabels", "receive") +
    "\nList labels that have sending addresses\n"
    + HelpExampleCli("listlabels", "send") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("listlabels", "receive")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string purpose;
    if (!request.params[0].isNull()) {
        purpose = request.params[0].get_str();
    }

    // Add to a set to sort by label name, then insert into Univalue array
    std::set<std::string> label_set = pwallet->ListAddrBookLabels(purpose);

    UniValue ret(UniValue::VARR);
    for (const std::string& name : label_set) {
        ret.push_back(name);
    }

    return ret;
},
    };
}

static RPCHelpMan send()
{
    return RPCHelpMan{"send",
        "\nEXPERIMENTAL warning: this call may be changed in future releases.\n"
        "\nSend a transaction.\n",
        {
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs (key-value pairs), where none of the keys are duplicated.\n"
                    "That is, each address can only appear once and there can only be one 'data' object.\n"
                    "For convenience, a dictionary, which holds the key-value pairs directly, is also accepted.",
                {
                    {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the Dash address, the value (float or string) is the amount in " + CURRENCY_UNIT + ""},
                        },
                        },
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
                        },
                    },
                },
            },
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
                        "       \"" + FeeModes("\"\n\"") + "\""},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
            {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                {
                    {"add_inputs", RPCArg::Type::BOOL, RPCArg::Default{false}, "If inputs are specified, automatically include more if they are not enough."},
                    {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                          "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                          "If that happens, you will need to fund the transaction with different inputs and republish it."},
                    {"add_to_wallet", RPCArg::Type::BOOL, RPCArg::Default{true}, "When false, returns a serialized transaction which will not be added to the wallet or broadcast"},
                    {"change_address", RPCArg::Type::STR_HEX, RPCArg::DefaultHint{"pool address"}, "The Dash address to receive the change"},
                    {"change_position", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
            "       \"" + FeeModes("\"\n\"") + "\""},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
                    {"include_watching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only.\n"
                                          "Only solvable inputs can be used. Watch-only destinations are solvable if the public key and/or output script was imported,\n"
                                          "e.g. with 'importpubkey' or 'importmulti' with the 'pubkeys' or 'desc' field."},
                    {"inputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Specify inputs instead of adding them automatically. A JSON array of JSON objects",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"sequence", RPCArg::Type::NUM, RPCArg::Optional::NO, "The sequence number"},
                        },
                    },
                    {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
                    {"lock_unspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                    {"psbt", RPCArg::Type::BOOL,  RPCArg::DefaultHint{"automatic"}, "Always return a PSBT, implies add_to_wallet=false."},
                    {"subtract_fee_from_outputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Outputs to subtract the fee from, specified as integer indices.\n"
                    "The fee will be equally deducted from the amount of each specified output.\n"
                    "Those recipients will receive less funds than you enter in their corresponding amount field.\n"
                    "If no outputs are specified here, the sender pays the fee.",
                        {
                            {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                        },
                    },
                },
                "options"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                    {RPCResult::Type::STR_HEX, "txid", /* optional */ true, "The transaction id for the send. Only 1 transaction is created regardless of the number of addresses."},
                    {RPCResult::Type::STR_HEX, "hex", /* optional */ true, "If add_to_wallet is false, the hex-encoded raw transaction with signature(s)"},
                    {RPCResult::Type::STR, "psbt", /* optional */ true, "If more signatures are needed, or if add_to_wallet is false, the base64-encoded (partially) signed transaction"}
                }
        },
        RPCExamples{""
        "\nSend 0.1 Dash with a confirmation target of 6 blocks in economical fee estimate mode\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.1}' 6 economical\n") +
        "Send 0.2 Dash with a fee rate of 1.1 " + CURRENCY_ATOM + "/B using positional arguments\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.2}' null \"unset\" 1.1\n") +
        "Send 0.2 Dash with a fee rate of 1 " + CURRENCY_ATOM + "/B using the options argument\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.2}' null \"unset\" null '{\"fee_rate\": 1}'\n") +
        "Send 0.3 Dash with a fee rate of 25 " + CURRENCY_ATOM + "/B using named arguments\n"
        + HelpExampleCli("-named send", "outputs='{\"" + EXAMPLE_ADDRESS[0] + "\": 0.3}' fee_rate=25\n") +
        "Create a transaction that should confirm the next block, with a specific input, and return result without adding to wallet or broadcasting to the network\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.1}' 1 economical '{\"add_to_wallet\": false, \"inputs\": [{\"txid\":\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\", \"vout\":1}]}'")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {
                UniValueType(), // outputs (ARR or OBJ, checked later)
                UniValue::VNUM, // conf_target
                UniValue::VSTR, // estimate_mode
                UniValueType(), // fee_rate, will be checked by AmountFromValue() in SetFeeEstimateMode()
                UniValue::VOBJ, // options
                }, true
            );

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    UniValue options{request.params[4].isNull() ? UniValue::VOBJ : request.params[4]};
    if (options.exists("estimate_mode") || options.exists("conf_target")) {
        if (!request.params[1].isNull() || !request.params[2].isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Pass conf_target and estimate_mode either as arguments or in the options object, but not both");
        }
    } else {
        options.pushKV("conf_target", request.params[1]);
        options.pushKV("estimate_mode", request.params[2]);
    }
    if (options.exists("fee_rate")) {
        if (!request.params[3].isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Pass the fee_rate either as an argument, or in the options object, but not both");
        }
    } else {
        options.pushKV("fee_rate", request.params[3]);
    }
    if (!options["conf_target"].isNull() && (options["estimate_mode"].isNull() || (options["estimate_mode"].get_str() == "unset"))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Specify estimate_mode");
    }
    if (options.exists("feeRate")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use fee_rate (" + CURRENCY_ATOM + "/B) instead of feeRate");
    }
    if (options.exists("changeAddress")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use change_address");
    }
    if (options.exists("changePosition")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use change_position");
    }
    if (options.exists("includeWatching")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use include_watching");
    }
    if (options.exists("lockUnspents")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use lock_unspents");
    }
    if (options.exists("subtractFeeFromOutputs")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use subtract_fee_from_outputs");
    }

    const bool psbt_opt_in = options.exists("psbt") && options["psbt"].get_bool();

    CAmount fee;
    int change_position;
    CMutableTransaction rawTx = ConstructTransaction(options["inputs"], request.params[0], options["locktime"]);
    CCoinControl coin_control;
    // Automatically select coins, unless at least one is manually selected. Can
    // be overridden by options.add_inputs.
    coin_control.m_add_inputs = rawTx.vin.size() == 0;
    FundTransaction(*pwallet, rawTx, fee, change_position, options, coin_control, /* override_min_fee */ false);

    bool add_to_wallet = true;
    if (options.exists("add_to_wallet")) {
        add_to_wallet = options["add_to_wallet"].get_bool();
    }

    // Make a blank psbt
    PartiallySignedTransaction psbtx(rawTx);

    // Fill transaction with our data and sign
    bool complete = true;
    const TransactionError err = pwallet->FillPSBT(psbtx, complete, SIGHASH_ALL, true, false);
    if (err != TransactionError::OK) {
        throw JSONRPCTransactionError(err);
    }

    CMutableTransaction mtx;
    complete = FinalizeAndExtractPSBT(psbtx, mtx);

    UniValue result(UniValue::VOBJ);

    if (psbt_opt_in || !complete || !add_to_wallet) {
        // Serialize the PSBT
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << psbtx;
        result.pushKV("psbt", EncodeBase64(ssTx.str()));
    }

    if (complete) {
        std::string err_string;
        std::string hex = EncodeHexTx(CTransaction(mtx));
        CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
        result.pushKV("txid", tx->GetHash().GetHex());
        if (add_to_wallet && !psbt_opt_in) {
            pwallet->CommitTransaction(tx, {}, {} /* orderForm */);
        } else {
            result.pushKV("hex", hex);
        }
    }
    result.pushKV("complete", complete);

    return result;
},
    };
}

static RPCHelpMan sethdseed()
{
    return RPCHelpMan{"sethdseed",
                "\nSet or generate a new HD wallet seed. Non-HD wallets will not be upgraded to being a HD wallet. Wallets that are already\n"
                "HD can not be updated to a new HD seed.\n"
                "\nNote that you will need to MAKE A NEW BACKUP of your wallet after setting the HD wallet seed." +
        HELP_REQUIRING_PASSPHRASE,
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

RPCHelpMan walletprocesspsbt()
{
    return RPCHelpMan{"walletprocesspsbt",
        "\nUpdate a PSBT with input information from our wallet and then sign inputs\n"
        "that we can sign for." +
                HELP_REQUIRING_PASSPHRASE,
        {
            {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction base64 string"},
            {"sign", RPCArg::Type::BOOL, RPCArg::Default{true}, "Also sign the transaction when updating (requires wallet to be unlocked)"},
            {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"ALL"}, "The signature hash type to sign with if not specified by the PSBT. Must be one of\n"
    "       \"ALL\"\n"
    "       \"NONE\"\n"
    "       \"SINGLE\"\n"
    "       \"ALL|ANYONECANPAY\"\n"
    "       \"NONE|ANYONECANPAY\"\n"
    "       \"SINGLE|ANYONECANPAY\""},
            {"bip32derivs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Include BIP 32 derivation paths for public keys if we know them"},
            {"finalize", RPCArg::Type::BOOL, RPCArg::Default{true}, "Also finalize inputs if possible"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "psbt", "The base64-encoded partially signed transaction"},
                {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
            }
        },
        RPCExamples{
            HelpExampleCli("walletprocesspsbt", "\"psbt\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR});

    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    const CWallet& wallet{*pwallet};
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    // Unserialize the transaction
    PartiallySignedTransaction psbtx;
    std::string error;
    if (!DecodeBase64PSBT(psbtx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    // Get the sighash type
    int nHashType = ParseSighashString(request.params[2]);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(*psbtx.tx);

    // Fill transaction with our data and also sign
    bool sign = request.params[1].isNull() ? true : request.params[1].get_bool();
    bool bip32derivs = request.params[3].isNull() ? true : request.params[3].get_bool();
    bool finalize = request.params[4].isNull() ? true : request.params[4].get_bool();
    bool complete = true;

    if (sign) EnsureWalletIsUnlocked(*pwallet);

    const TransactionError err{wallet.FillPSBT(psbtx, complete, nHashType, sign, bip32derivs, nullptr, finalize)};
    if (err != TransactionError::OK) {
        throw JSONRPCTransactionError(err);
    }

    UniValue result(UniValue::VOBJ);
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;
    result.pushKV("psbt", EncodeBase64(ssTx.str()));
    result.pushKV("complete", complete);

    return result;
},
    };
}

static RPCHelpMan walletcreatefundedpsbt()
{
    return RPCHelpMan{"walletcreatefundedpsbt",
        "\nCreates and funds a transaction in the Partially Signed Transaction format.\n"
        "Implements the Creator and Updater roles.\n",
        {
            {"inputs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "Leave empty to add inputs automatically. See add_inputs option.",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"sequence", RPCArg::Type::NUM, RPCArg::DefaultHint{"depends on the value of the 'locktime' argument"}, "The sequence number"},
                        },
                    },
                },
            },
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs (key-value pairs), where none of the keys are duplicated.\n"
                    "That is, each address can only appear once and there can only be one 'data' object.\n"
                    "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                    "accepted as second parameter.",
                {
                    {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the Dash address, the value (float or string) is the amount in " + CURRENCY_UNIT + ""},
                        },
                        },
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
                        },
                    },
                },
            },
            {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
            {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                {
                    {"add_inputs", RPCArg::Type::BOOL, RPCArg::Default{false}, "If inputs are specified, automatically include more if they are not enough."},
                    {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                    "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                    "If that happens, you will need to fund the transaction with different inputs and republish it."},
                    {"changeAddress", RPCArg::Type::STR_HEX, RPCArg::DefaultHint{"pool address"}, "The Dash address to receive the change"},
                    {"changePosition", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                    {"includeWatching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only"},
                    {"lockUnspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/B."},
                    {"feeRate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set: makes wallet determine the fee"}, "Set a specific fee rate in " + CURRENCY_UNIT + "/kB"},
                    {"subtractFeeFromOutputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The outputs to subtract the fee from.\n"
                        "The fee will be equally deducted from the amount of each specified output.\n"
                        "Those recipients will receive less Dash than you enter in their corresponding amount field.\n"
                        "If no outputs are specified here, the sender pays the fee.",
                        {
                            {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                        },
                    },
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
                    "         \"" + FeeModes("\"\n\"") + "\""},
                },
                "options"},
            {"bip32derivs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Include BIP 32 derivation paths for public keys if we know them"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "psbt", "The resulting raw transaction (base64-encoded string)"},
                {RPCResult::Type::STR_AMOUNT, "fee", "Fee in " + CURRENCY_UNIT + " the resulting transaction pays"},
                {RPCResult::Type::NUM, "changepos", "The position of the added change output, or -1"},
            }
                        },
                        RPCExamples{
                    "\nCreate a transaction with no inputs\n"
                    + HelpExampleCli("walletcreatefundedpsbt", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
                        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // ARR or OBJ, checked later
        UniValue::VNUM,
        UniValue::VOBJ,
        UniValue::VBOOL,
        }, true
    );

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    CWallet& wallet{*pwallet};
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    CAmount fee;
    int change_position;
    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2]);
    CCoinControl coin_control;
    // Automatically select coins, unless at least one is manually selected. Can
    // be overridden by options.add_inputs.
    coin_control.m_add_inputs = rawTx.vin.size() == 0;
    FundTransaction(*pwallet, rawTx, fee, change_position, request.params[3], coin_control, /* override_min_fee */ true);

    // Make a blank psbt
    PartiallySignedTransaction psbtx{rawTx};

    // Fill transaction with out data but don't sign
    bool bip32derivs = request.params[4].isNull() ? true : request.params[4].get_bool();
    bool complete = true;
    const TransactionError err{wallet.FillPSBT(psbtx, complete, 1, false, bip32derivs)};
    if (err != TransactionError::OK) {
        throw JSONRPCTransactionError(err);
    }

    // Serialize the PSBT
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;

    UniValue result(UniValue::VOBJ);
    result.pushKV("psbt", EncodeBase64(ssTx.str()));
    result.pushKV("fee", ValueFromAmount(fee));
    result.pushKV("changepos", change_position);
    return result;
},
    };
}

static RPCHelpMan upgradewallet()
{
    return RPCHelpMan{"upgradewallet",
        "\nUpgrade the wallet. Upgrades to the latest version if no version number is specified.\n"
        "New keys may be generated and a new wallet backup will need to be made.",
        {
            {"version", RPCArg::Type::NUM, RPCArg::Default{FEATURE_LATEST}, "The version number to upgrade to. Default is the latest wallet version."}
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
        version = request.params[0].get_int();
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

RPCHelpMan abortrescan();
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
RPCHelpMan signmessage();
RPCHelpMan backupwallet();
RPCHelpMan restorewallet();
RPCHelpMan dumphdinfo();
RPCHelpMan importelectrumwallet();

// encryption
RPCHelpMan walletpassphrase();
RPCHelpMan walletpassphrasechange();
RPCHelpMan walletlock();
RPCHelpMan encryptwallet();

// transactions
RPCHelpMan listreceivedbyaddress();
RPCHelpMan listreceivedbylabel();
RPCHelpMan listtransactions();
RPCHelpMan listsinceblock();
RPCHelpMan gettransaction();
RPCHelpMan abandontransaction();
RPCHelpMan rescanblockchain();

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
