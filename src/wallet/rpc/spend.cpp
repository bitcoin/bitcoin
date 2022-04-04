// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <policy/policy.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/util.h>
#include <util/fees.h>
#include <util/translation.h>
#include <util/vector.h>
#include <wallet/coincontrol.h>
#include <wallet/rpc/util.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <map>

namespace wallet {
static void ParseRecipients(const UniValue& address_amounts, const UniValue& subtract_fee_outputs, std::vector<CRecipient> &recipients) {
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
    auto res = CreateTransaction(wallet, recipients, RANDOM_CHANGE_POSITION, coin_control, /*sign=*/true);
    if (!res) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, util::ErrorString(res).original);
    }
    const CTransactionRef& tx = res->tx;
    wallet.CommitTransaction(tx, std::move(map_value), {} /* orderForm */);
    if (verbose) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", tx->GetHash().GetHex());
        entry.pushKV("fee_reason", StringForFeeReason(res->fee_calc.reason));
        return entry;
    }
    return tx->GetHash().GetHex();
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
        cc.m_feerate = CFeeRate{AmountFromValue(fee_rate, /*decimals=*/3)};
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

RPCHelpMan sendtoaddress()
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

    SetFeeEstimateMode(*pwallet, coin_control, /*conf_target=*/request.params[7], /*estimate_mode=*/request.params[8], /*fee_rate=*/request.params[10], /*override_min_fee=*/false);

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

RPCHelpMan sendmany()
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

    SetFeeEstimateMode(*pwallet, coin_control, /*conf_target=*/request.params[8], /*estimate_mode=*/request.params[9], /*fee_rate=*/request.params[10], /*override_min_fee=*/false);

    std::vector<CRecipient> recipients;
    ParseRecipients(sendTo, subtractFeeFromAmount, recipients);
    const bool verbose{request.params[11].isNull() ? false : request.params[11].get_bool()};

    return SendMoney(*pwallet, coin_control, recipients, std::move(mapValue), verbose);
},
    };
}

RPCHelpMan settxfee()
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
                {"solving_data", UniValueType(UniValue::VOBJ)},
                {"subtractFeeFromOutputs", UniValueType(UniValue::VARR)},
                {"subtract_fee_from_outputs", UniValueType(UniValue::VARR)},
                {"conf_target", UniValueType(UniValue::VNUM)},
                {"estimate_mode", UniValueType(UniValue::VSTR)},
            },
            true, true);

        if (options.exists("add_inputs")) {
            coinControl.m_allow_other_inputs = options["add_inputs"].get_bool();
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
            change_position = (options.exists("change_position") ? options["change_position"] : options["changePosition"]).getInt<int>();
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

    if (options.exists("solving_data")) {
        const UniValue solving_data = options["solving_data"].get_obj();
        if (solving_data.exists("pubkeys")) {
            for (const UniValue& pk_univ : solving_data["pubkeys"].get_array().getValues()) {
                const std::string& pk_str = pk_univ.get_str();
                if (!IsHex(pk_str)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("'%s' is not hex", pk_str));
                }
                const std::vector<unsigned char> data(ParseHex(pk_str));
                const CPubKey pubkey(data.begin(), data.end());
                if (!pubkey.IsFullyValid()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("'%s' is not a valid public key", pk_str));
                }
                coinControl.m_external_provider.pubkeys.emplace(pubkey.GetID(), pubkey);
                // Add script for pubkeys
                const CScript pk_script = GetScriptForDestination(PKHash(pubkey));
                coinControl.m_external_provider.scripts.emplace(CScriptID(pk_script), pk_script);
            }
        }

        if (solving_data.exists("scripts")) {
            for (const UniValue& script_univ : solving_data["scripts"].get_array().getValues()) {
                const std::string& script_str = script_univ.get_str();
                if (!IsHex(script_str)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("'%s' is not hex", script_str));
                }
                std::vector<unsigned char> script_data(ParseHex(script_str));
                const CScript script(script_data.begin(), script_data.end());
                coinControl.m_external_provider.scripts.emplace(CScriptID(script), script);
            }
        }

        if (solving_data.exists("descriptors")) {
            for (const UniValue& desc_univ : solving_data["descriptors"].get_array().getValues()) {
                const std::string& desc_str  = desc_univ.get_str();
                FlatSigningProvider desc_out;
                std::string error;
                std::vector<CScript> scripts_temp;
                std::unique_ptr<Descriptor> desc = Parse(desc_str, desc_out, error, true);
                if (!desc) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unable to parse descriptor '%s': %s", desc_str, error));
                }
                desc->Expand(0, desc_out, scripts_temp, desc_out);
                coinControl.m_external_provider = Merge(coinControl.m_external_provider, desc_out);
            }
        }
    }

    if (tx.vout.size() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "TX must have at least one output");

    if (change_position != -1 && (change_position < 0 || (unsigned int)change_position > tx.vout.size()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "changePosition out of bounds");

    for (unsigned int idx = 0; idx < subtractFeeFromOutputs.size(); idx++) {
        int pos = subtractFeeFromOutputs[idx].getInt<int>();
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

RPCHelpMan fundrawtransaction()
{
    return RPCHelpMan{"fundrawtransaction",
                "\nIf the transaction has no inputs, they will be automatically selected to meet its out value.\n"
                "It will add at most one change output to the outputs.\n"
                "No existing outputs will be modified unless \"subtractFeeFromOutputs\" is specified.\n"
                "Note that inputs which were signed may need to be resigned after completion since in/outputs have been added.\n"
                "The inputs added will not be signed, use signrawtransactionwithkey\n"
                "or signrawtransactionwithwallet for that.\n"
                "All existing inputs must either have their previous output transaction be in the wallet\n"
                "or be in the UTXO set. Solving data must be provided for non-wallet inputs.\n"
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
                            {"solving_data", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "Keys and scripts needed for producing a final transaction with a dummy signature.\n"
                                "Used for fee estimation during coin selection.",
                                {
                                    {"pubkeys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Public keys involved in this transaction.",
                                        {
                                            {"pubkey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A public key"},
                                        },
                                    },
                                    {"scripts", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Scripts involved in this transaction.",
                                        {
                                            {"script", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A script"},
                                        },
                                    },
                                    {"descriptors", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Descriptors that provide solving data for this transaction.",
                                        {
                                            {"descriptor", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A descriptor"},
                                        },
                                    }
                                }
                            },
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
    coin_control.m_allow_other_inputs = true;
    FundTransaction(*pwallet, tx, fee, change_position, request.params[1], coin_control, /*override_min_fee=*/true);

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
                {RPCResult::Type::ARR, "errors", /*optional=*/true, "Script verification errors (if there are any)",
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

RPCHelpMan send()
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
                    {"solving_data", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "Keys and scripts needed for producing a final transaction with a dummy signature.\n"
                        "Used for fee estimation during coin selection.",
                        {
                            {"pubkeys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Public keys involved in this transaction.",
                                {
                                    {"pubkey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A public key"},
                                },
                            },
                            {"scripts", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Scripts involved in this transaction.",
                                {
                                    {"script", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A script"},
                                },
                            },
                            {"descriptors", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Descriptors that provide solving data for this transaction.",
                                {
                                    {"descriptor", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A descriptor"},
                                },
                            }
                        }
                    },
                },
                "options"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                    {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id for the send. Only 1 transaction is created regardless of the number of addresses."},
                    {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "If add_to_wallet is false, the hex-encoded raw transaction with signature(s)"},
                    {RPCResult::Type::STR, "psbt", /*optional=*/true, "If more signatures are needed, or if add_to_wallet is false, the base64-encoded (partially) signed transaction"}
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
    coin_control.m_allow_other_inputs = rawTx.vin.size() == 0;
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

RPCHelpMan walletcreatefundedpsbt()
{
    return RPCHelpMan{"walletcreatefundedpsbt",
        "\nCreates and funds a transaction in the Partially Signed Transaction format.\n"
        "Implements the Creator and Updater roles.\n"
        "All existing inputs must either have their previous output transaction be in the wallet\n"
        "or be in the UTXO set. Solving data must be provided for non-wallet inputs.\n",
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
                    {"solving_data", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED_NAMED_ARG, "Keys and scripts needed for producing a final transaction with a dummy signature.\n"
                        "Used for fee estimation during coin selection.",
                        {
                            {"pubkeys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Public keys involved in this transaction.",
                                {
                                    {"pubkey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A public key"},
                                },
                            },
                            {"scripts", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Scripts involved in this transaction.",
                                {
                                    {"script", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A script"},
                                },
                            },
                            {"descriptors", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Descriptors that provide solving data for this transaction.",
                                {
                                    {"descriptor", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A descriptor"},
                                },
                            }
                        }
                    },
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
    coin_control.m_allow_other_inputs = rawTx.vin.size() == 0;
    FundTransaction(*pwallet, rawTx, fee, change_position, request.params[3], coin_control, /*override_min_fee=*/true);

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
} // namespace wallet
