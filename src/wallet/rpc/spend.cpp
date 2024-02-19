// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <policy/policy.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/util.h>
#include <script/script.h>
#include <util/fees.h>
#include <util/rbf.h>
#include <util/translation.h>
#include <util/vector.h>
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/fees.h>
#include <wallet/rpc/util.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <univalue.h>


namespace wallet {
std::vector<CRecipient> CreateRecipients(const std::vector<std::pair<CTxDestination, CAmount>>& outputs, const std::set<int>& subtract_fee_outputs)
{
    std::vector<CRecipient> recipients;
    for (size_t i = 0; i < outputs.size(); ++i) {
        const auto& [destination, amount] = outputs.at(i);
        CRecipient recipient{destination, amount, subtract_fee_outputs.contains(i)};
        recipients.push_back(recipient);
    }
    return recipients;
}

static void InterpretFeeEstimationInstructions(const UniValue& conf_target, const UniValue& estimate_mode, const UniValue& fee_rate, UniValue& options)
{
    if (options.exists("conf_target") || options.exists("estimate_mode")) {
        if (!conf_target.isNull() || !estimate_mode.isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Pass conf_target and estimate_mode either as arguments or in the options object, but not both");
        }
    } else {
        options.pushKV("conf_target", conf_target);
        options.pushKV("estimate_mode", estimate_mode);
    }
    if (options.exists("fee_rate")) {
        if (!fee_rate.isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Pass the fee_rate either as an argument, or in the options object, but not both");
        }
    } else {
        options.pushKV("fee_rate", fee_rate);
    }
    if (!options["conf_target"].isNull() && (options["estimate_mode"].isNull() || (options["estimate_mode"].get_str() == "unset"))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Specify estimate_mode");
    }
}

std::set<int> InterpretSubtractFeeFromOutputInstructions(const UniValue& sffo_instructions, const std::vector<std::string>& destinations)
{
    std::set<int> sffo_set;
    if (sffo_instructions.isNull()) return sffo_set;
    if (sffo_instructions.isBool()) {
        if (sffo_instructions.get_bool()) sffo_set.insert(0);
        return sffo_set;
    }
    for (const auto& sffo : sffo_instructions.getValues()) {
        if (sffo.isStr()) {
            for (size_t i = 0; i < destinations.size(); ++i) {
                if (sffo.get_str() == destinations.at(i)) {
                    sffo_set.insert(i);
                    break;
                }
            }
        }
        if (sffo.isNum()) {
            int pos = sffo.getInt<int>();
            if (sffo_set.contains(pos))
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, duplicated position: %d", pos));
            if (pos < 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, negative position: %d", pos));
            if (pos >= int(destinations.size()))
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, position too large: %d", pos));
            sffo_set.insert(pos);
        }
    }
    return sffo_set;
}

static UniValue FinishTransaction(const std::shared_ptr<CWallet> pwallet, const UniValue& options, const CMutableTransaction& rawTx)
{
    // Make a blank psbt
    PartiallySignedTransaction psbtx(rawTx);

    // First fill transaction with our data without signing,
    // so external signers are not asked to sign more than once.
    bool complete;
    pwallet->FillPSBT(psbtx, complete, SIGHASH_DEFAULT, /*sign=*/false, /*bip32derivs=*/true);
    const TransactionError err{pwallet->FillPSBT(psbtx, complete, SIGHASH_DEFAULT, /*sign=*/true, /*bip32derivs=*/false)};
    if (err != TransactionError::OK) {
        throw JSONRPCTransactionError(err);
    }

    CMutableTransaction mtx;
    complete = FinalizeAndExtractPSBT(psbtx, mtx);

    UniValue result(UniValue::VOBJ);

    const bool psbt_opt_in{options.exists("psbt") && options["psbt"].get_bool()};
    bool add_to_wallet{options.exists("add_to_wallet") ? options["add_to_wallet"].get_bool() : true};
    if (psbt_opt_in || !complete || !add_to_wallet) {
        // Serialize the PSBT
        DataStream ssTx{};
        ssTx << psbtx;
        result.pushKV("psbt", EncodeBase64(ssTx.str()));
    }

    if (complete) {
        std::string hex{EncodeHexTx(CTransaction(mtx))};
        CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
        result.pushKV("txid", tx->GetHash().GetHex());
        if (add_to_wallet && !psbt_opt_in) {
            pwallet->CommitTransaction(tx, {}, /*orderForm=*/{});
        } else {
            result.pushKV("hex", hex);
        }
    }
    result.pushKV("complete", complete);

    return result;
}

static void PreventOutdatedOptions(const UniValue& options)
{
    if (options.exists("feeRate")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use fee_rate (" + CURRENCY_ATOM + "/vB) instead of feeRate");
    }
    if (options.exists("changeAddress")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use change_address instead of changeAddress");
    }
    if (options.exists("changePosition")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use change_position instead of changePosition");
    }
    if (options.exists("includeWatching")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use include_watching instead of includeWatching");
    }
    if (options.exists("lockUnspents")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use lock_unspents instead of lockUnspents");
    }
    if (options.exists("subtractFeeFromOutputs")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Use subtract_fee_from_outputs instead of subtractFeeFromOutputs");
    }
}

static void IsSilentPaymentsEnabled(const CWallet &pwallet)
{
    if (pwallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) || pwallet.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER) ) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Silent payments require access to private keys to build transactions.");
    }
    EnsureWalletIsUnlocked(pwallet);
}

UniValue SendMoney(CWallet& wallet, const CCoinControl &coin_control, std::vector<CRecipient> &recipients, mapValue_t map_value, bool verbose)
{
    EnsureWalletIsUnlocked(wallet);

    // This function is only used by sendtoaddress and sendmany.
    // This should always try to sign, if we don't have private keys, don't try to do anything here.
    if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Private keys are disabled for this wallet");
    }

    // Shuffle recipient list
    std::shuffle(recipients.begin(), recipients.end(), FastRandomContext());

    // Send
    auto res = CreateTransaction(wallet, recipients, /*change_pos=*/std::nullopt, coin_control, true);
    if (!res) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, util::ErrorString(res).original);
    }
    const CTransactionRef& tx = res->tx;
    wallet.CommitTransaction(tx, std::move(map_value), /*orderForm=*/{});
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
 * @param[in]     fee_rate          UniValue real; fee rate in sat/vB;
 *                                      if present, both conf_target and estimate_mode must either be null, or "unset"
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
        // Default RBF to true for explicit fee_rate, if unset.
        if (!cc.m_signal_bip125_rbf) cc.m_signal_bip125_rbf = true;
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
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitcoin address to send to."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1"},
                    {"comment", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A comment used to store what the transaction is for.\n"
                                         "This is not part of the transaction, just kept in your wallet."},
                    {"comment_to", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A comment to store the name of the person or organization\n"
                                         "to which you're sending the transaction. This is not part of the \n"
                                         "transaction, just kept in your wallet."},
                    {"subtractfeefromamount", RPCArg::Type::BOOL, RPCArg::Default{false}, "The fee will be deducted from the amount being sent.\n"
                                         "The recipient will receive less bitcoins than you enter in the amount field."},
                    {"replaceable", RPCArg::Type::BOOL, RPCArg::DefaultHint{"wallet default"}, "Signal that this transaction can be replaced by a transaction (BIP 125)"},
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
                     "\"" + FeeModes("\"\n\"") + "\""},
                    {"avoid_reuse", RPCArg::Type::BOOL, RPCArg::Default{true}, "(only available if avoid_reuse wallet flag is set) Avoid spending from dirty addresses; addresses are considered\n"
                                         "dirty if they have previously been used in a transaction. If true, this also activates avoidpartialspends, grouping outputs by their addresses."},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
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
                    "\nSend 0.1 BTC\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1") +
                    "\nSend 0.1 BTC with a confirmation target of 6 blocks in economical fee estimate mode using positional arguments\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1 \"donation\" \"sean's outpost\" false true 6 economical") +
                    "\nSend 0.1 BTC with a fee rate of 1.1 " + CURRENCY_ATOM + "/vB, subtract fee from amount, BIP125-replaceable, using positional arguments\n"
                    + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 0.1 \"drinks\" \"room77\" true true null \"unset\" null 1.1") +
                    "\nSend 0.2 BTC with a confirmation target of 6 blocks in economical fee estimate mode using named arguments\n"
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.2 conf_target=6 estimate_mode=\"economical\"") +
                    "\nSend 0.5 BTC with a fee rate of 25 " + CURRENCY_ATOM + "/vB using named arguments\n"
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.5 fee_rate=25")
                    + HelpExampleCli("-named sendtoaddress", "address=\"" + EXAMPLE_ADDRESS[0] + "\" amount=0.5 fee_rate=25 subtractfeefromamount=false replaceable=true avoid_reuse=true comment=\"2 pizzas\" comment_to=\"jeremy\" verbose=true")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

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

    CCoinControl coin_control;
    if (!request.params[5].isNull()) {
        coin_control.m_signal_bip125_rbf = request.params[5].get_bool();
    }

    coin_control.m_avoid_address_reuse = GetAvoidReuseFlag(*pwallet, request.params[8]);
    // We also enable partial spend avoidance if reuse avoidance is set.
    coin_control.m_avoid_partial_spends |= coin_control.m_avoid_address_reuse;

    SetFeeEstimateMode(*pwallet, coin_control, /*conf_target=*/request.params[6], /*estimate_mode=*/request.params[7], /*fee_rate=*/request.params[9], /*override_min_fee=*/false);

    EnsureWalletIsUnlocked(*pwallet);

    UniValue address_amounts(UniValue::VOBJ);
    const std::string address = request.params[0].get_str();
    address_amounts.pushKV(address, request.params[1]);
    std::vector<CRecipient> recipients = CreateRecipients(
            ParseOutputs(address_amounts),
            InterpretSubtractFeeFromOutputInstructions(request.params[4], address_amounts.getKeys())
    );
    auto it = std::find_if(recipients.begin(), recipients.end(), [](const auto& r) { return std::holds_alternative<V0SilentPaymentDestination>(r.dest); });
    if (it != recipients.end()) {
        IsSilentPaymentsEnabled(*pwallet);
        coin_control.m_silent_payment = true;
    }
    const bool verbose{request.params[10].isNull() ? false : request.params[10].get_bool()};

    return SendMoney(*pwallet, coin_control, recipients, mapValue, verbose);
},
    };
}

RPCHelpMan sendmany()
{
    return RPCHelpMan{"sendmany",
        "Send multiple times. Amounts are double-precision floating point numbers." +
        HELP_REQUIRING_PASSPHRASE,
                {
                    {"dummy", RPCArg::Type::STR, RPCArg::Default{"\"\""}, "Must be set to \"\" for backwards compatibility.",
                     RPCArgOptions{
                         .oneline_description = "\"\"",
                     }},
                    {"amounts", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::NO, "The addresses and amounts",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The bitcoin address is the key, the numeric amount (can be string) in " + CURRENCY_UNIT + " is the value"},
                        },
                    },
                    {"minconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Ignored dummy value"},
                    {"comment", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A comment"},
                    {"subtractfeefrom", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The addresses.\n"
                                       "The fee will be equally deducted from the amount of each selected address.\n"
                                       "Those recipients will receive less bitcoins than you enter in their corresponding amount field.\n"
                                       "If no addresses are specified here, the sender pays the fee.",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Subtract fee from this address"},
                        },
                    },
                    {"replaceable", RPCArg::Type::BOOL, RPCArg::DefaultHint{"wallet default"}, "Signal that this transaction can be replaced by a transaction (BIP 125)"},
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
                     "\"" + FeeModes("\"\n\"") + "\""},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
                    {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, return extra information about the transaction."},
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
            + HelpExampleCli("sendmany", "\"\" \"{\\\"" + EXAMPLE_ADDRESS[0] + "\\\":0.01,\\\"" + EXAMPLE_ADDRESS[1] + "\\\":0.02}\" 6 \"testing\"") +
            "\nSend two amounts to two different addresses, subtract fee from amount:\n"
            + HelpExampleCli("sendmany", "\"\" \"{\\\"" + EXAMPLE_ADDRESS[0] + "\\\":0.01,\\\"" + EXAMPLE_ADDRESS[1] + "\\\":0.02}\" 1 \"\" \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("sendmany", "\"\", {\"" + EXAMPLE_ADDRESS[0] + "\":0.01,\"" + EXAMPLE_ADDRESS[1] + "\":0.02}, 6, \"testing\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    if (!request.params[0].isNull() && !request.params[0].get_str().empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Dummy value must be set to \"\"");
    }
    UniValue sendTo = request.params[1].get_obj();

    mapValue_t mapValue;
    if (!request.params[3].isNull() && !request.params[3].get_str().empty())
        mapValue["comment"] = request.params[3].get_str();

    CCoinControl coin_control;
    if (!request.params[5].isNull()) {
        coin_control.m_signal_bip125_rbf = request.params[5].get_bool();
    }

    SetFeeEstimateMode(*pwallet, coin_control, /*conf_target=*/request.params[6], /*estimate_mode=*/request.params[7], /*fee_rate=*/request.params[8], /*override_min_fee=*/false);

    std::vector<CRecipient> recipients = CreateRecipients(
            ParseOutputs(sendTo),
            InterpretSubtractFeeFromOutputInstructions(request.params[4], sendTo.getKeys())
    );
    auto it = std::find_if(recipients.begin(), recipients.end(), [](const auto& r) { return std::holds_alternative<V0SilentPaymentDestination>(r.dest); });
    if (it != recipients.end()) {
        IsSilentPaymentsEnabled(*pwallet);
        coin_control.m_silent_payment = true;
    }
    const bool verbose{request.params[9].isNull() ? false : request.params[9].get_bool()};

    return SendMoney(*pwallet, coin_control, recipients, std::move(mapValue), verbose);
},
    };
}

RPCHelpMan settxfee()
{
    return RPCHelpMan{"settxfee",
                "\nSet the transaction fee rate in " + CURRENCY_UNIT + "/kvB for this wallet. Overrides the global -paytxfee command line parameter.\n"
                "Can be deactivated by passing 0 as the fee. In that case automatic fee selection will be used by default.\n",
                {
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The transaction fee rate in " + CURRENCY_UNIT + "/kvB"},
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
    if (!pwallet) return UniValue::VNULL;

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


// Only includes key documentation where the key is snake_case in all RPC methods. MixedCase keys can be added later.
static std::vector<RPCArg> FundTxDoc(bool solving_data = true)
{
    std::vector<RPCArg> args = {
        {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks", RPCArgOptions{.also_positional = true}},
        {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
         "\"" + FeeModes("\"\n\"") + "\"", RPCArgOptions{.also_positional = true}},
        {
            "replaceable", RPCArg::Type::BOOL, RPCArg::DefaultHint{"wallet default"}, "Marks this transaction as BIP125-replaceable.\n"
            "Allows this transaction to be replaced by a transaction with higher fees"
        },
    };
    if (solving_data) {
        args.push_back({"solving_data", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "Keys and scripts needed for producing a final transaction with a dummy signature.\n"
        "Used for fee estimation during coin selection.",
            {
                {
                    "pubkeys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Public keys involved in this transaction.",
                    {
                        {"pubkey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A public key"},
                    }
                },
                {
                    "scripts", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Scripts involved in this transaction.",
                    {
                        {"script", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A script"},
                    }
                },
                {
                    "descriptors", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Descriptors that provide solving data for this transaction.",
                    {
                        {"descriptor", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A descriptor"},
                    }
                },
            }
        });
    }
    return args;
}

CreatedTransactionResult FundTransaction(CWallet& wallet, const CMutableTransaction& tx, const std::vector<CRecipient>& recipients, const UniValue& options, CCoinControl& coinControl, bool override_min_fee)
{
    // We want to make sure tx.vout is not used now that we are passing outputs as a vector of recipients.
    // This sets us up to remove tx completely in a future PR in favor of passing the inputs directly.
    CHECK_NONFATAL(tx.vout.empty());
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    std::optional<unsigned int> change_position;
    bool lockUnspents = false;
    if (!options.isNull()) {
      if (options.type() == UniValue::VBOOL) {
        // backward compatibility bool only fallback
        coinControl.fAllowWatchOnly = options.get_bool();
      }
      else {
        RPCTypeCheckObj(options,
            {
                {"add_inputs", UniValueType(UniValue::VBOOL)},
                {"include_unsafe", UniValueType(UniValue::VBOOL)},
                {"add_to_wallet", UniValueType(UniValue::VBOOL)},
                {"changeAddress", UniValueType(UniValue::VSTR)},
                {"change_address", UniValueType(UniValue::VSTR)},
                {"changePosition", UniValueType(UniValue::VNUM)},
                {"change_position", UniValueType(UniValue::VNUM)},
                {"change_type", UniValueType(UniValue::VSTR)},
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
                {"replaceable", UniValueType(UniValue::VBOOL)},
                {"conf_target", UniValueType(UniValue::VNUM)},
                {"estimate_mode", UniValueType(UniValue::VSTR)},
                {"minconf", UniValueType(UniValue::VNUM)},
                {"maxconf", UniValueType(UniValue::VNUM)},
                {"input_weights", UniValueType(UniValue::VARR)},
            },
            true, true);

        if (options.exists("add_inputs")) {
            coinControl.m_allow_other_inputs = options["add_inputs"].get_bool();
        }

        if (options.exists("changeAddress") || options.exists("change_address")) {
            const std::string change_address_str = (options.exists("change_address") ? options["change_address"] : options["changeAddress"]).get_str();
            CTxDestination dest = DecodeDestination(change_address_str);

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Change address must be a valid bitcoin address");
            }

            coinControl.destChange = dest;
        }

        if (options.exists("changePosition") || options.exists("change_position")) {
            int pos = (options.exists("change_position") ? options["change_position"] : options["changePosition"]).getInt<int>();
            if (pos < 0 || (unsigned int)pos > recipients.size()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "changePosition out of bounds");
            }
            change_position = (unsigned int)pos;
        }

        if (options.exists("change_type")) {
            if (options.exists("changeAddress") || options.exists("change_address")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both change address and address type options");
            }
            if (std::optional<OutputType> parsed = ParseOutputType(options["change_type"].get_str())) {
                coinControl.m_change_type.emplace(parsed.value());
            } else {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Unknown change type '%s'", options["change_type"].get_str()));
            }
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
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both fee_rate (" + CURRENCY_ATOM + "/vB) and feeRate (" + CURRENCY_UNIT + "/kvB)");
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

        if (options.exists("replaceable")) {
            coinControl.m_signal_bip125_rbf = options["replaceable"].get_bool();
        }

        if (options.exists("minconf")) {
            coinControl.m_min_depth = options["minconf"].getInt<int>();

            if (coinControl.m_min_depth < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative minconf");
            }
        }

        if (options.exists("maxconf")) {
            coinControl.m_max_depth = options["maxconf"].getInt<int>();

            if (coinControl.m_max_depth < coinControl.m_min_depth) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("maxconf can't be lower than minconf: %d < %d", coinControl.m_max_depth, coinControl.m_min_depth));
            }
        }
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
                // Add witness script for pubkeys
                const CScript wit_script = GetScriptForDestination(WitnessV0KeyHash(pubkey));
                coinControl.m_external_provider.scripts.emplace(CScriptID(wit_script), wit_script);
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
                coinControl.m_external_provider.Merge(std::move(desc_out));
            }
        }
    }

    if (options.exists("input_weights")) {
        for (const UniValue& input : options["input_weights"].get_array().getValues()) {
            Txid txid = Txid::FromUint256(ParseHashO(input, "txid"));

            const UniValue& vout_v = input.find_value("vout");
            if (!vout_v.isNum()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
            }
            int vout = vout_v.getInt<int>();
            if (vout < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout cannot be negative");
            }

            const UniValue& weight_v = input.find_value("weight");
            if (!weight_v.isNum()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing weight key");
            }
            int64_t weight = weight_v.getInt<int64_t>();
            const int64_t min_input_weight = GetTransactionInputWeight(CTxIn());
            CHECK_NONFATAL(min_input_weight == 165);
            if (weight < min_input_weight) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, weight cannot be less than 165 (41 bytes (size of outpoint + sequence + empty scriptSig) * 4 (witness scaling factor)) + 1 (empty witness)");
            }
            if (weight > MAX_STANDARD_TX_WEIGHT) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, weight cannot be greater than the maximum standard tx weight of %d", MAX_STANDARD_TX_WEIGHT));
            }

            coinControl.SetInputWeight(COutPoint(txid, vout), weight);
        }
    }

    if (recipients.empty())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "TX must have at least one output");

    auto txr = FundTransaction(wallet, tx, recipients, change_position, lockUnspents, coinControl);
    if (!txr) {
        throw JSONRPCError(RPC_WALLET_ERROR, ErrorString(txr).original);
    }
    return *txr;
}

static void SetOptionsInputWeights(const UniValue& inputs, UniValue& options)
{
    if (options.exists("input_weights")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Input weights should be specified in inputs rather than in options.");
    }
    if (inputs.size() == 0) {
        return;
    }
    UniValue weights(UniValue::VARR);
    for (const UniValue& input : inputs.getValues()) {
        if (input.exists("weight")) {
            weights.push_back(input);
        }
    }
    options.pushKV("input_weights", weights);
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
                    {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "For backward compatibility: passing in a true instead of an object will result in {\"includeWatching\":true}",
                        Cat<std::vector<RPCArg>>(
                        {
                            {"add_inputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "For a transaction with existing inputs, automatically include more if they are not enough."},
                            {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                          "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                          "If that happens, you will need to fund the transaction with different inputs and republish it."},
                            {"minconf", RPCArg::Type::NUM, RPCArg::Default{0}, "If add_inputs is specified, require inputs with at least this many confirmations."},
                            {"maxconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If add_inputs is specified, require inputs with at most this many confirmations."},
                            {"changeAddress", RPCArg::Type::STR, RPCArg::DefaultHint{"automatic"}, "The bitcoin address to receive the change"},
                            {"changePosition", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                            {"change_type", RPCArg::Type::STR, RPCArg::DefaultHint{"set by -changetype"}, "The output type to use. Only valid if changeAddress is not specified. Options are \"legacy\", \"p2sh-segwit\", \"bech32\", and \"bech32m\"."},
                            {"includeWatching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only.\n"
                                                          "Only solvable inputs can be used. Watch-only destinations are solvable if the public key and/or output script was imported,\n"
                                                          "e.g. with 'importpubkey' or 'importmulti' with the 'pubkeys' or 'desc' field."},
                            {"lockUnspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
                            {"feeRate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_UNIT + "/kvB."},
                            {"subtractFeeFromOutputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The integers.\n"
                                                          "The fee will be equally deducted from the amount of each specified output.\n"
                                                          "Those recipients will receive less bitcoins than you enter in their corresponding amount field.\n"
                                                          "If no outputs are specified here, the sender pays the fee.",
                                {
                                    {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                                },
                            },
                            {"input_weights", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "Inputs and their corresponding weights",
                                {
                                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                        {
                                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output index"},
                                            {"weight", RPCArg::Type::NUM, RPCArg::Optional::NO, "The maximum weight for this input, "
                                                "including the weight of the outpoint and sequence number. "
                                                "Note that serialized signature sizes are not guaranteed to be consistent, "
                                                "so the maximum DER signatures size of 73 bytes should be used when considering ECDSA signatures."
                                                "Remember to convert serialized sizes to weight units when necessary."},
                                        },
                                    },
                                },
                             },
                        },
                        FundTxDoc()),
                        RPCArgOptions{
                            .skip_type_check = true,
                            .oneline_description = "options",
                        }},
                    {"iswitness", RPCArg::Type::BOOL, RPCArg::DefaultHint{"depends on heuristic tests"}, "Whether the transaction hex is a serialized witness transaction.\n"
                        "If iswitness is not present, heuristic tests will be used in decoding.\n"
                        "If true, only witness deserialization will be tried.\n"
                        "If false, only non-witness deserialization will be tried.\n"
                        "This boolean should reflect whether the transaction has inputs\n"
                        "(e.g. fully valid, or on-chain transactions), if known by the caller."
                    },
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
                            + HelpExampleCli("signrawtransactionwithwallet", "\"fundedtransactionhex\"") +
                            "\nSend the transaction\n"
                            + HelpExampleCli("sendrawtransaction", "\"signedtransactionhex\"")
                                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    // parse hex string from parameter
    CMutableTransaction tx;
    bool try_witness = request.params[2].isNull() ? true : request.params[2].get_bool();
    bool try_no_witness = request.params[2].isNull() ? true : !request.params[2].get_bool();
    if (!DecodeHexTx(tx, request.params[0].get_str(), try_no_witness, try_witness)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    UniValue options = request.params[1];
    std::vector<std::pair<CTxDestination, CAmount>> destinations;
    for (const auto& tx_out : tx.vout) {
        CTxDestination dest;
        ExtractDestination(tx_out.scriptPubKey, dest);
        destinations.emplace_back(dest, tx_out.nValue);
    }
    std::vector<std::string> dummy(destinations.size(), "dummy");
    std::vector<CRecipient> recipients = CreateRecipients(
            destinations,
            InterpretSubtractFeeFromOutputInstructions(options["subtractFeeFromOutputs"], dummy)
    );
    CCoinControl coin_control;
    // Automatically select (additional) coins. Can be overridden by options.add_inputs.
    coin_control.m_allow_other_inputs = true;
    // Clear tx.vout since it is not meant to be used now that we are passing outputs directly.
    // This sets us up for a future PR to completely remove tx from the function signature in favor of passing inputs directly
    tx.vout.clear();
    auto txr = FundTransaction(*pwallet, tx, recipients, options, coin_control, /*override_min_fee=*/true);

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(*txr.tx));
    result.pushKV("fee", ValueFromAmount(txr.fee));
    result.pushKV("changepos", txr.change_pos ? (int)*txr.change_pos : -1);

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
                    {"prevtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The previous dependent transaction outputs",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                    {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                                    {"scriptPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "script key"},
                                    {"redeemScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2SH) redeem script"},
                                    {"witnessScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2WSH or P2SH-P2WSH) witness script"},
                                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "(required for Segwit inputs) the amount spent"},
                                },
                            },
                        },
                    },
                    {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"DEFAULT for Taproot, ALL otherwise"}, "The signature hash type. Must be one of\n"
            "       \"DEFAULT\"\n"
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
                                {RPCResult::Type::ARR, "witness", "",
                                {
                                    {RPCResult::Type::STR_HEX, "witness", ""},
                                }},
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
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

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

// Definition of allowed formats of specifying transaction outputs in
// `bumpfee`, `psbtbumpfee`, `send` and `walletcreatefundedpsbt` RPCs.
static std::vector<RPCArg> OutputsDoc()
{
    return
    {
        {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
            {
                {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the bitcoin address,\n"
                         "the value (float or string) is the amount in " + CURRENCY_UNIT + ""},
            },
        },
        {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
            {
                {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
            },
        },
    };
}

static RPCHelpMan bumpfee_helper(std::string method_name)
{
    const bool want_psbt = method_name == "psbtbumpfee";
    const std::string incremental_fee{CFeeRate(DEFAULT_INCREMENTAL_RELAY_FEE).ToString(FeeEstimateMode::SAT_VB)};

    return RPCHelpMan{method_name,
        "\nBumps the fee of an opt-in-RBF transaction T, replacing it with a new transaction B.\n"
        + std::string(want_psbt ? "Returns a PSBT instead of creating and signing a new transaction.\n" : "") +
        "An opt-in RBF transaction with the given txid must be in the wallet.\n"
        "The command will pay the additional fee by reducing change outputs or adding inputs when necessary.\n"
        "It may add a new change output if one does not already exist.\n"
        "All inputs in the original transaction will be included in the replacement transaction.\n"
        "The command will fail if the wallet or mempool contains a transaction that spends one of T's outputs.\n"
        "By default, the new fee will be calculated automatically using the estimatesmartfee RPC.\n"
        "The user can specify a confirmation target for estimatesmartfee.\n"
        "Alternatively, the user can specify a fee rate in " + CURRENCY_ATOM + "/vB for the new transaction.\n"
        "At a minimum, the new fee rate must be high enough to pay an additional new relay fee (incrementalfee\n"
        "returned by getnetworkinfo) to enter the node's mempool.\n"
        "* WARNING: before version 0.21, fee_rate was in " + CURRENCY_UNIT + "/kvB. As of 0.21, fee_rate is in " + CURRENCY_ATOM + "/vB. *\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The txid to be bumped"},
            {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                {
                    {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks\n"},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"},
                             "\nSpecify a fee rate in " + CURRENCY_ATOM + "/vB instead of relying on the built-in fee estimator.\n"
                             "Must be at least " + incremental_fee + " higher than the current transaction fee rate.\n"
                             "WARNING: before version 0.21, fee_rate was in " + CURRENCY_UNIT + "/kvB. As of 0.21, fee_rate is in " + CURRENCY_ATOM + "/vB.\n"},
                    {"replaceable", RPCArg::Type::BOOL, RPCArg::Default{true}, "Whether the new transaction should still be\n"
                             "marked bip-125 replaceable. If true, the sequence numbers in the transaction will\n"
                             "be left unchanged from the original. If false, any input sequence numbers in the\n"
                             "original transaction that were less than 0xfffffffe will be increased to 0xfffffffe\n"
                             "so the new transaction will not be explicitly bip-125 replaceable (though it may\n"
                             "still be replaceable in practice, for example if it has unconfirmed ancestors which\n"
                             "are replaceable).\n"},
                    {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
                             "\"" + FeeModes("\"\n\"") + "\""},
                    {"outputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The outputs specified as key-value pairs.\n"
                             "Each key may only appear once, i.e. there can only be one 'data' output, and no address may be duplicated.\n"
                             "At least one output of either type must be specified.\n"
                             "Cannot be provided if 'original_change_index' is specified.",
                        OutputsDoc(),
                        RPCArgOptions{.skip_type_check = true}},
                    {"original_change_index", RPCArg::Type::NUM, RPCArg::DefaultHint{"not set, detect change automatically"}, "The 0-based index of the change output on the original transaction. "
                                                                                                                            "The indicated output will be recycled into the new change output on the bumped transaction. "
                                                                                                                            "The remainder after paying the recipients and fees will be sent to the output script of the "
                                                                                                                            "original change output. The change output’s amount can increase if bumping the transaction "
                                                                                                                            "adds new inputs, otherwise it will decrease. Cannot be used in combination with the 'outputs' option."},
                },
                RPCArgOptions{.oneline_description="options"}},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "", Cat(
                want_psbt ?
                std::vector<RPCResult>{{RPCResult::Type::STR, "psbt", "The base64-encoded unsigned PSBT of the new transaction."}} :
                std::vector<RPCResult>{{RPCResult::Type::STR_HEX, "txid", "The id of the new transaction."}},
            {
                {RPCResult::Type::STR_AMOUNT, "origfee", "The fee of the replaced transaction."},
                {RPCResult::Type::STR_AMOUNT, "fee", "The fee of the new transaction."},
                {RPCResult::Type::ARR, "errors", "Errors encountered during processing (may be empty).",
                {
                    {RPCResult::Type::STR, "", ""},
                }},
            })
        },
        RPCExamples{
    "\nBump the fee, get the new transaction\'s " + std::string(want_psbt ? "psbt" : "txid") + "\n" +
            HelpExampleCli(method_name, "<txid>")
        },
        [want_psbt](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !pwallet->IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER) && !want_psbt) {
        throw JSONRPCError(RPC_WALLET_ERROR, "bumpfee is not available with wallets that have private keys disabled. Use psbtbumpfee instead.");
    }

    uint256 hash(ParseHashV(request.params[0], "txid"));

    CCoinControl coin_control;
    coin_control.fAllowWatchOnly = pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    // optional parameters
    coin_control.m_signal_bip125_rbf = true;
    std::vector<CTxOut> outputs;

    std::optional<uint32_t> original_change_index;

    if (!request.params[1].isNull()) {
        UniValue options = request.params[1];
        RPCTypeCheckObj(options,
            {
                {"confTarget", UniValueType(UniValue::VNUM)},
                {"conf_target", UniValueType(UniValue::VNUM)},
                {"fee_rate", UniValueType()}, // will be checked by AmountFromValue() in SetFeeEstimateMode()
                {"replaceable", UniValueType(UniValue::VBOOL)},
                {"estimate_mode", UniValueType(UniValue::VSTR)},
                {"outputs", UniValueType()}, // will be checked by AddOutputs()
                {"original_change_index", UniValueType(UniValue::VNUM)},
            },
            true, true);

        if (options.exists("confTarget") && options.exists("conf_target")) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "confTarget and conf_target options should not both be set. Use conf_target (confTarget is deprecated).");
        }

        auto conf_target = options.exists("confTarget") ? options["confTarget"] : options["conf_target"];

        if (options.exists("replaceable")) {
            coin_control.m_signal_bip125_rbf = options["replaceable"].get_bool();
        }
        SetFeeEstimateMode(*pwallet, coin_control, conf_target, options["estimate_mode"], options["fee_rate"], /*override_min_fee=*/false);

        // Prepare new outputs by creating a temporary tx and calling AddOutputs().
        if (!options["outputs"].isNull()) {
            if (options["outputs"].isArray() && options["outputs"].empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, output argument cannot be an empty array");
            }
            CMutableTransaction tempTx;
            AddOutputs(tempTx, options["outputs"]);
            outputs = tempTx.vout;
        }

        if (options.exists("original_change_index")) {
            original_change_index = options["original_change_index"].getInt<uint32_t>();
        }
    }

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    EnsureWalletIsUnlocked(*pwallet);


    std::vector<bilingual_str> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    feebumper::Result res;
    // Targeting feerate bump.
    res = feebumper::CreateRateBumpTransaction(*pwallet, hash, coin_control, errors, old_fee, new_fee, mtx, /*require_mine=*/ !want_psbt, outputs, original_change_index);
    if (res != feebumper::Result::OK) {
        switch(res) {
            case feebumper::Result::INVALID_ADDRESS_OR_KEY:
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errors[0].original);
                break;
            case feebumper::Result::INVALID_REQUEST:
                throw JSONRPCError(RPC_INVALID_REQUEST, errors[0].original);
                break;
            case feebumper::Result::INVALID_PARAMETER:
                throw JSONRPCError(RPC_INVALID_PARAMETER, errors[0].original);
                break;
            case feebumper::Result::WALLET_ERROR:
                throw JSONRPCError(RPC_WALLET_ERROR, errors[0].original);
                break;
            default:
                throw JSONRPCError(RPC_MISC_ERROR, errors[0].original);
                break;
        }
    }

    UniValue result(UniValue::VOBJ);

    // For bumpfee, return the new transaction id.
    // For psbtbumpfee, return the base64-encoded unsigned PSBT of the new transaction.
    if (!want_psbt) {
        if (!feebumper::SignTransaction(*pwallet, mtx)) {
            if (pwallet->IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Transaction incomplete. Try psbtbumpfee instead.");
            }
            throw JSONRPCError(RPC_WALLET_ERROR, "Can't sign transaction.");
        }

        uint256 txid;
        if (feebumper::CommitTransaction(*pwallet, hash, std::move(mtx), errors, txid) != feebumper::Result::OK) {
            throw JSONRPCError(RPC_WALLET_ERROR, errors[0].original);
        }

        result.pushKV("txid", txid.GetHex());
    } else {
        PartiallySignedTransaction psbtx(mtx);
        bool complete = false;
        const TransactionError err = pwallet->FillPSBT(psbtx, complete, SIGHASH_DEFAULT, /*sign=*/false, /*bip32derivs=*/true);
        CHECK_NONFATAL(err == TransactionError::OK);
        CHECK_NONFATAL(!complete);
        DataStream ssTx{};
        ssTx << psbtx;
        result.pushKV("psbt", EncodeBase64(ssTx.str()));
    }

    result.pushKV("origfee", ValueFromAmount(old_fee));
    result.pushKV("fee", ValueFromAmount(new_fee));
    UniValue result_errors(UniValue::VARR);
    for (const bilingual_str& error : errors) {
        result_errors.push_back(error.original);
    }
    result.pushKV("errors", result_errors);

    return result;
},
    };
}

RPCHelpMan bumpfee() { return bumpfee_helper("bumpfee"); }
RPCHelpMan psbtbumpfee() { return bumpfee_helper("psbtbumpfee"); }

RPCHelpMan send()
{
    return RPCHelpMan{"send",
        "\nEXPERIMENTAL warning: this call may be changed in future releases.\n"
        "\nSend a transaction.\n",
        {
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs specified as key-value pairs.\n"
                    "Each key may only appear once, i.e. there can only be one 'data' output, and no address may be duplicated.\n"
                    "At least one output of either type must be specified.\n"
                    "For convenience, a dictionary, which holds the key-value pairs directly, is also accepted.",
                OutputsDoc(),
                RPCArgOptions{.skip_type_check = true}},
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
             "\"" + FeeModes("\"\n\"") + "\""},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
            {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                Cat<std::vector<RPCArg>>(
                {
                    {"add_inputs", RPCArg::Type::BOOL, RPCArg::DefaultHint{"false when \"inputs\" are specified, true otherwise"},"Automatically include coins from the wallet to cover the target amount.\n"},
                    {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                          "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                          "If that happens, you will need to fund the transaction with different inputs and republish it."},
                    {"minconf", RPCArg::Type::NUM, RPCArg::Default{0}, "If add_inputs is specified, require inputs with at least this many confirmations."},
                    {"maxconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If add_inputs is specified, require inputs with at most this many confirmations."},
                    {"add_to_wallet", RPCArg::Type::BOOL, RPCArg::Default{true}, "When false, returns a serialized transaction which will not be added to the wallet or broadcast"},
                    {"change_address", RPCArg::Type::STR, RPCArg::DefaultHint{"automatic"}, "The bitcoin address to receive the change"},
                    {"change_position", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                    {"change_type", RPCArg::Type::STR, RPCArg::DefaultHint{"set by -changetype"}, "The output type to use. Only valid if change_address is not specified. Options are \"legacy\", \"p2sh-segwit\", \"bech32\" and \"bech32m\"."},
                    {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB.", RPCArgOptions{.also_positional = true}},
                    {"include_watching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only.\n"
                                          "Only solvable inputs can be used. Watch-only destinations are solvable if the public key and/or output script was imported,\n"
                                          "e.g. with 'importpubkey' or 'importmulti' with the 'pubkeys' or 'desc' field."},
                    {"inputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Specify inputs instead of adding them automatically. A JSON array of JSON objects",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"sequence", RPCArg::Type::NUM, RPCArg::Optional::NO, "The sequence number"},
                            {"weight", RPCArg::Type::NUM, RPCArg::DefaultHint{"Calculated from wallet and solving data"}, "The maximum weight for this input, "
                                        "including the weight of the outpoint and sequence number. "
                                        "Note that signature sizes are not guaranteed to be consistent, "
                                        "so the maximum DER signatures size of 73 bytes should be used when considering ECDSA signatures."
                                        "Remember to convert serialized sizes to weight units when necessary."},
                        },
                    },
                    {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
                    {"lock_unspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                    {"psbt", RPCArg::Type::BOOL,  RPCArg::DefaultHint{"automatic"}, "Always return a PSBT, implies add_to_wallet=false."},
                    {"subtract_fee_from_outputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Outputs to subtract the fee from, specified as integer indices.\n"
                    "The fee will be equally deducted from the amount of each specified output.\n"
                    "Those recipients will receive less bitcoins than you enter in their corresponding amount field.\n"
                    "If no outputs are specified here, the sender pays the fee.",
                        {
                            {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                        },
                    },
                },
                FundTxDoc()),
                RPCArgOptions{.oneline_description="options"}},
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
        "\nSend 0.1 BTC with a confirmation target of 6 blocks in economical fee estimate mode\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.1}' 6 economical\n") +
        "Send 0.2 BTC with a fee rate of 1.1 " + CURRENCY_ATOM + "/vB using positional arguments\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.2}' null \"unset\" 1.1\n") +
        "Send 0.2 BTC with a fee rate of 1 " + CURRENCY_ATOM + "/vB using the options argument\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.2}' null \"unset\" null '{\"fee_rate\": 1}'\n") +
        "Send 0.3 BTC with a fee rate of 25 " + CURRENCY_ATOM + "/vB using named arguments\n"
        + HelpExampleCli("-named send", "outputs='{\"" + EXAMPLE_ADDRESS[0] + "\": 0.3}' fee_rate=25\n") +
        "Create a transaction that should confirm the next block, with a specific input, and return result without adding to wallet or broadcasting to the network\n"
        + HelpExampleCli("send", "'{\"" + EXAMPLE_ADDRESS[0] + "\": 0.1}' 1 economical '{\"add_to_wallet\": false, \"inputs\": [{\"txid\":\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\", \"vout\":1}]}'")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;

            UniValue options{request.params[4].isNull() ? UniValue::VOBJ : request.params[4]};
            InterpretFeeEstimationInstructions(/*conf_target=*/request.params[1], /*estimate_mode=*/request.params[2], /*fee_rate=*/request.params[3], options);
            PreventOutdatedOptions(options);


            CCoinControl coin_control;
            bool rbf{options.exists("replaceable") ? options["replaceable"].get_bool() : pwallet->m_signal_rbf};
            UniValue outputs(UniValue::VOBJ);
            outputs = NormalizeOutputs(request.params[0]);
            std::vector<CRecipient> recipients = CreateRecipients(
                    ParseOutputs(outputs),
                    InterpretSubtractFeeFromOutputInstructions(options["subtract_fee_from_outputs"], outputs.getKeys())
            );
            auto it = std::find_if(recipients.begin(), recipients.end(), [](const auto& r) { return std::holds_alternative<V0SilentPaymentDestination>(r.dest); });
            if (it != recipients.end()) {
                IsSilentPaymentsEnabled(*pwallet);
                coin_control.m_silent_payment = true;
            }
            CMutableTransaction rawTx = ConstructTransaction(options["inputs"], request.params[0], options["locktime"], rbf);
            // Automatically select coins, unless at least one is manually selected. Can
            // be overridden by options.add_inputs.
            coin_control.m_allow_other_inputs = rawTx.vin.size() == 0;
            SetOptionsInputWeights(options["inputs"], options);
            // Clear tx.vout since it is not meant to be used now that we are passing outputs directly.
            // This sets us up for a future PR to completely remove tx from the function signature in favor of passing inputs directly
            rawTx.vout.clear();
            auto txr = FundTransaction(*pwallet, rawTx, recipients, options, coin_control, /*override_min_fee=*/false);

            return FinishTransaction(pwallet, options, CMutableTransaction(*txr.tx));
        }
    };
}

RPCHelpMan sendall()
{
    return RPCHelpMan{"sendall",
        "EXPERIMENTAL warning: this call may be changed in future releases.\n"
        "\nSpend the value of all (or specific) confirmed UTXOs in the wallet to one or more recipients.\n"
        "Unconfirmed inbound UTXOs and locked UTXOs will not be spent. Sendall will respect the avoid_reuse wallet flag.\n"
        "If your wallet contains many small inputs, either because it received tiny payments or as a result of accumulating change, consider using `send_max` to exclude inputs that are worth less than the fees needed to spend them.\n",
        {
            {"recipients", RPCArg::Type::ARR, RPCArg::Optional::NO, "The sendall destinations. Each address may only appear once.\n"
                "Optionally some recipients can be specified with an amount to perform payments, but at least one address must appear without a specified amount.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "A bitcoin address which receives an equal share of the unspecified amount."},
                    {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the bitcoin address, the value (float or string) is the amount in " + CURRENCY_UNIT + ""},
                        },
                    },
                },
            },
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, "The fee estimate mode, must be one of (case insensitive):\n"
             "\"" + FeeModes("\"\n\"") + "\""},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
            {
                "options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                Cat<std::vector<RPCArg>>(
                    {
                        {"add_to_wallet", RPCArg::Type::BOOL, RPCArg::Default{true}, "When false, returns the serialized transaction without broadcasting or adding it to the wallet"},
                        {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB.", RPCArgOptions{.also_positional = true}},
                        {"include_watching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch-only.\n"
                                              "Only solvable inputs can be used. Watch-only destinations are solvable if the public key and/or output script was imported,\n"
                                              "e.g. with 'importpubkey' or 'importmulti' with the 'pubkeys' or 'desc' field."},
                        {"inputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Use exactly the specified inputs to build the transaction. Specifying inputs is incompatible with the send_max, minconf, and maxconf options.",
                            {
                                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                    {
                                        {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                        {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                                        {"sequence", RPCArg::Type::NUM, RPCArg::DefaultHint{"depends on the value of the 'replaceable' and 'locktime' arguments"}, "The sequence number"},
                                    },
                                },
                            },
                        },
                        {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
                        {"lock_unspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                        {"psbt", RPCArg::Type::BOOL,  RPCArg::DefaultHint{"automatic"}, "Always return a PSBT, implies add_to_wallet=false."},
                        {"send_max", RPCArg::Type::BOOL, RPCArg::Default{false}, "When true, only use UTXOs that can pay for their own fees to maximize the output amount. When 'false' (default), no UTXO is left behind. send_max is incompatible with providing specific inputs."},
                        {"minconf", RPCArg::Type::NUM, RPCArg::Default{0}, "Require inputs with at least this many confirmations."},
                        {"maxconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Require inputs with at most this many confirmations."},
                    },
                    FundTxDoc()
                ),
                RPCArgOptions{.oneline_description="options"}
            },
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
        "\nSpend all UTXOs from the wallet with a fee rate of 1 " + CURRENCY_ATOM + "/vB using named arguments\n"
        + HelpExampleCli("-named sendall", "recipients='[\"" + EXAMPLE_ADDRESS[0] + "\"]' fee_rate=1\n") +
        "Spend all UTXOs with a fee rate of 1.1 " + CURRENCY_ATOM + "/vB using positional arguments\n"
        + HelpExampleCli("sendall", "'[\"" + EXAMPLE_ADDRESS[0] + "\"]' null \"unset\" 1.1\n") +
        "Spend all UTXOs split into equal amounts to two addresses with a fee rate of 1.5 " + CURRENCY_ATOM + "/vB using the options argument\n"
        + HelpExampleCli("sendall", "'[\"" + EXAMPLE_ADDRESS[0] + "\", \"" + EXAMPLE_ADDRESS[1] + "\"]' null \"unset\" null '{\"fee_rate\": 1.5}'\n") +
        "Leave dust UTXOs in wallet, spend only UTXOs with positive effective value with a fee rate of 10 " + CURRENCY_ATOM + "/vB using the options argument\n"
        + HelpExampleCli("sendall", "'[\"" + EXAMPLE_ADDRESS[0] + "\"]' null \"unset\" null '{\"fee_rate\": 10, \"send_max\": true}'\n") +
        "Spend all UTXOs with a fee rate of 1.3 " + CURRENCY_ATOM + "/vB using named arguments and sending a 0.25 " + CURRENCY_UNIT + " to another recipient\n"
        + HelpExampleCli("-named sendall", "recipients='[{\"" + EXAMPLE_ADDRESS[1] + "\": 0.25}, \""+ EXAMPLE_ADDRESS[0] + "\"]' fee_rate=1.3\n")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::shared_ptr<CWallet> const pwallet{GetWalletForJSONRPCRequest(request)};
            if (!pwallet) return UniValue::VNULL;
            // Make sure the results are valid at least up to the most recent block
            // the user could have gotten from another RPC command prior to now
            pwallet->BlockUntilSyncedToCurrentChain();

            UniValue options{request.params[4].isNull() ? UniValue::VOBJ : request.params[4]};
            InterpretFeeEstimationInstructions(/*conf_target=*/request.params[1], /*estimate_mode=*/request.params[2], /*fee_rate=*/request.params[3], options);
            PreventOutdatedOptions(options);


            std::set<std::string> addresses_without_amount;
            UniValue recipient_key_value_pairs(UniValue::VARR);
            const UniValue& recipients{request.params[0]};
            for (unsigned int i = 0; i < recipients.size(); ++i) {
                const UniValue& recipient{recipients[i]};
                if (recipient.isStr()) {
                    UniValue rkvp(UniValue::VOBJ);
                    rkvp.pushKV(recipient.get_str(), 0);
                    recipient_key_value_pairs.push_back(rkvp);
                    addresses_without_amount.insert(recipient.get_str());
                } else {
                    recipient_key_value_pairs.push_back(recipient);
                }
            }

            if (addresses_without_amount.size() == 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must provide at least one address without a specified amount");
            }

            CCoinControl coin_control;

            SetFeeEstimateMode(*pwallet, coin_control, options["conf_target"], options["estimate_mode"], options["fee_rate"], /*override_min_fee=*/false);

            coin_control.fAllowWatchOnly = ParseIncludeWatchonly(options["include_watching"], *pwallet);

            if (options.exists("minconf")) {
                if (options["minconf"].getInt<int>() < 0)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid minconf (minconf cannot be negative): %s", options["minconf"].getInt<int>()));
                }

                coin_control.m_min_depth = options["minconf"].getInt<int>();
            }

            if (options.exists("maxconf")) {
                coin_control.m_max_depth = options["maxconf"].getInt<int>();

                if (coin_control.m_max_depth < coin_control.m_min_depth) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("maxconf can't be lower than minconf: %d < %d", coin_control.m_max_depth, coin_control.m_min_depth));
                }
            }

            const bool rbf{options.exists("replaceable") ? options["replaceable"].get_bool() : pwallet->m_signal_rbf};

            FeeCalculation fee_calc_out;
            CFeeRate fee_rate{GetMinimumFeeRate(*pwallet, coin_control, &fee_calc_out)};
            // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
            // provided one
            if (coin_control.m_feerate && fee_rate > *coin_control.m_feerate) {
               throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Fee rate (%s) is lower than the minimum fee rate setting (%s)", coin_control.m_feerate->ToString(FeeEstimateMode::SAT_VB), fee_rate.ToString(FeeEstimateMode::SAT_VB)));
            }
            if (fee_calc_out.reason == FeeReason::FALLBACK && !pwallet->m_allow_fallback_fee) {
                // eventually allow a fallback fee
                throw JSONRPCError(RPC_WALLET_ERROR, "Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.");
            }

            CMutableTransaction rawTx{ConstructTransaction(options["inputs"], recipient_key_value_pairs, options["locktime"], rbf)};
            LOCK(pwallet->cs_wallet);

            CAmount total_input_value(0);
            bool send_max{options.exists("send_max") ? options["send_max"].get_bool() : false};
            if (options.exists("inputs") && options.exists("send_max")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot combine send_max with specific inputs.");
            } else if (options.exists("inputs") && (options.exists("minconf") || options.exists("maxconf"))) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot combine minconf or maxconf with specific inputs.");
            } else if (options.exists("inputs")) {
                for (const CTxIn& input : rawTx.vin) {
                    if (pwallet->IsSpent(input.prevout)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Input not available. UTXO (%s:%d) was already spent.", input.prevout.hash.ToString(), input.prevout.n));
                    }
                    const CWalletTx* tx{pwallet->GetWalletTx(input.prevout.hash)};
                    if (!tx || input.prevout.n >= tx->tx->vout.size() || !(pwallet->IsMine(tx->tx->vout[input.prevout.n]) & (coin_control.fAllowWatchOnly ? ISMINE_ALL : ISMINE_SPENDABLE))) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Input not found. UTXO (%s:%d) is not part of wallet.", input.prevout.hash.ToString(), input.prevout.n));
                    }
                    total_input_value += tx->tx->vout[input.prevout.n].nValue;
                }
            } else {
                CoinFilterParams coins_params;
                coins_params.min_amount = 0;
                for (const COutput& output : AvailableCoins(*pwallet, &coin_control, fee_rate, coins_params).All()) {
                    CHECK_NONFATAL(output.input_bytes > 0);
                    if (send_max && fee_rate.GetFee(output.input_bytes) > output.txout.nValue) {
                        continue;
                    }
                    CTxIn input(output.outpoint.hash, output.outpoint.n, CScript(), rbf ? MAX_BIP125_RBF_SEQUENCE : CTxIn::SEQUENCE_FINAL);
                    rawTx.vin.push_back(input);
                    total_input_value += output.txout.nValue;
                }
            }

            // estimate final size of tx
            const TxSize tx_size{CalculateMaximumSignedTxSize(CTransaction(rawTx), pwallet.get())};
            const CAmount fee_from_size{fee_rate.GetFee(tx_size.vsize)};
            const CAmount effective_value{total_input_value - fee_from_size};

            if (fee_from_size > pwallet->m_default_max_tx_fee) {
                throw JSONRPCError(RPC_WALLET_ERROR, TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED).original);
            }

            if (effective_value <= 0) {
                if (send_max) {
                    throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Total value of UTXO pool too low to pay for transaction, try using lower feerate.");
                } else {
                    throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Total value of UTXO pool too low to pay for transaction. Try using lower feerate or excluding uneconomic UTXOs with 'send_max' option.");
                }
            }

            // If this transaction is too large, e.g. because the wallet has many UTXOs, it will be rejected by the node's mempool.
            if (tx_size.weight > MAX_STANDARD_TX_WEIGHT) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Transaction too large.");
            }

            CAmount output_amounts_claimed{0};
            for (const CTxOut& out : rawTx.vout) {
                output_amounts_claimed += out.nValue;
            }

            if (output_amounts_claimed > total_input_value) {
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Assigned more value to outputs than available funds.");
            }

            const CAmount remainder{effective_value - output_amounts_claimed};
            if (remainder < 0) {
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds for fees after creating specified outputs.");
            }

            const CAmount per_output_without_amount{remainder / (long)addresses_without_amount.size()};

            bool gave_remaining_to_first{false};
            for (CTxOut& out : rawTx.vout) {
                CTxDestination dest;
                ExtractDestination(out.scriptPubKey, dest);
                std::string addr{EncodeDestination(dest)};
                if (addresses_without_amount.count(addr) > 0) {
                    out.nValue = per_output_without_amount;
                    if (!gave_remaining_to_first) {
                        out.nValue += remainder % addresses_without_amount.size();
                        gave_remaining_to_first = true;
                    }
                    if (IsDust(out, pwallet->chain().relayDustFee())) {
                        // Dynamically generated output amount is dust
                        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Dynamically assigned remainder results in dust output.");
                    }
                } else {
                    if (IsDust(out, pwallet->chain().relayDustFee())) {
                        // Specified output amount is dust
                        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Specified output amount to %s is below dust threshold.", addr));
                    }
                }
            }

            const bool lock_unspents{options.exists("lock_unspents") ? options["lock_unspents"].get_bool() : false};
            if (lock_unspents) {
                for (const CTxIn& txin : rawTx.vin) {
                    pwallet->LockCoin(txin.prevout);
                }
            }

            return FinishTransaction(pwallet, options, rawTx);
        }
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
                    {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"DEFAULT for Taproot, ALL otherwise"}, "The signature hash type to sign with if not specified by the PSBT. Must be one of\n"
            "       \"DEFAULT\"\n"
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
                        {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "The hex-encoded network transaction if complete"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("walletprocesspsbt", "\"psbt\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

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
    DataStream ssTx{};
    ssTx << psbtx;
    result.pushKV("psbt", EncodeBase64(ssTx.str()));
    result.pushKV("complete", complete);
    if (complete) {
        CMutableTransaction mtx;
        // Returns true if complete, which we already think it is.
        CHECK_NONFATAL(FinalizeAndExtractPSBT(psbtx, mtx));
        DataStream ssTx_final;
        ssTx_final << TX_WITH_WITNESS(mtx);
        result.pushKV("hex", HexStr(ssTx_final));
    }

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
                    {"inputs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "Leave empty to add inputs automatically. See add_inputs option.",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                    {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                                    {"sequence", RPCArg::Type::NUM, RPCArg::DefaultHint{"depends on the value of the 'locktime' and 'options.replaceable' arguments"}, "The sequence number"},
                                    {"weight", RPCArg::Type::NUM, RPCArg::DefaultHint{"Calculated from wallet and solving data"}, "The maximum weight for this input, "
                                        "including the weight of the outpoint and sequence number. "
                                        "Note that signature sizes are not guaranteed to be consistent, "
                                        "so the maximum DER signatures size of 73 bytes should be used when considering ECDSA signatures."
                                        "Remember to convert serialized sizes to weight units when necessary."},
                                },
                            },
                        },
                        },
                    {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs specified as key-value pairs.\n"
                            "Each key may only appear once, i.e. there can only be one 'data' output, and no address may be duplicated.\n"
                            "At least one output of either type must be specified.\n"
                            "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                            "accepted as second parameter.",
                        OutputsDoc(),
                        RPCArgOptions{.skip_type_check = true}},
                    {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
                    {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                        Cat<std::vector<RPCArg>>(
                        {
                            {"add_inputs", RPCArg::Type::BOOL, RPCArg::DefaultHint{"false when \"inputs\" are specified, true otherwise"}, "Automatically include coins from the wallet to cover the target amount.\n"},
                            {"include_unsafe", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include inputs that are not safe to spend (unconfirmed transactions from outside keys and unconfirmed replacement transactions).\n"
                                                          "Warning: the resulting transaction may become invalid if one of the unsafe inputs disappears.\n"
                                                          "If that happens, you will need to fund the transaction with different inputs and republish it."},
                            {"minconf", RPCArg::Type::NUM, RPCArg::Default{0}, "If add_inputs is specified, require inputs with at least this many confirmations."},
                            {"maxconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If add_inputs is specified, require inputs with at most this many confirmations."},
                            {"changeAddress", RPCArg::Type::STR, RPCArg::DefaultHint{"automatic"}, "The bitcoin address to receive the change"},
                            {"changePosition", RPCArg::Type::NUM, RPCArg::DefaultHint{"random"}, "The index of the change output"},
                            {"change_type", RPCArg::Type::STR, RPCArg::DefaultHint{"set by -changetype"}, "The output type to use. Only valid if changeAddress is not specified. Options are \"legacy\", \"p2sh-segwit\", \"bech32\", and \"bech32m\"."},
                            {"includeWatching", RPCArg::Type::BOOL, RPCArg::DefaultHint{"true for watch-only wallets, otherwise false"}, "Also select inputs which are watch only"},
                            {"lockUnspents", RPCArg::Type::BOOL, RPCArg::Default{false}, "Lock selected unspent outputs"},
                            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."},
                            {"feeRate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_UNIT + "/kvB."},
                            {"subtractFeeFromOutputs", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The outputs to subtract the fee from.\n"
                                                          "The fee will be equally deducted from the amount of each specified output.\n"
                                                          "Those recipients will receive less bitcoins than you enter in their corresponding amount field.\n"
                                                          "If no outputs are specified here, the sender pays the fee.",
                                {
                                    {"vout_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The zero-based output index, before a change output is added."},
                                },
                            },
                        },
                        FundTxDoc()),
                        RPCArgOptions{.oneline_description="options"}},
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
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    CWallet& wallet{*pwallet};
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    UniValue options{request.params[3].isNull() ? UniValue::VOBJ : request.params[3]};

    const UniValue &replaceable_arg = options["replaceable"];
    const bool rbf{replaceable_arg.isNull() ? wallet.m_signal_rbf : replaceable_arg.get_bool()};
    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2], rbf);
    UniValue outputs(UniValue::VOBJ);
    outputs = NormalizeOutputs(request.params[1]);
    std::vector<CRecipient> recipients = CreateRecipients(
            ParseOutputs(outputs),
            InterpretSubtractFeeFromOutputInstructions(options["subtractFeeFromOutputs"], outputs.getKeys())
    );
    CCoinControl coin_control;
    auto it = std::find_if(recipients.begin(), recipients.end(), [](const auto& r) { return std::holds_alternative<V0SilentPaymentDestination>(r.dest); });
    if (it != recipients.end()) {
        IsSilentPaymentsEnabled(*pwallet);
        coin_control.m_silent_payment = true;
    }
    // Automatically select coins, unless +at least one is manually selected. Can
    // be overridden by options.add_inputs+.
    coin_control.m_allow_other_inputs = rawTx.vin.size() == 0;
    SetOptionsInputWeights(request.params[0], options);
    // Clear tx.vout since it is not meant to be used now that we are passing outputs directly.
    // This sets us up for a future PR to completely remove tx from the function signature in favor of passing inputs directly
    rawTx.vout.clear();
    auto txr = FundTransaction(wallet, rawTx, recipients, options, coin_control, /*override_min_fee=*/true);

    // Make a blank psbt
    PartiallySignedTransaction psbtx(CMutableTransaction(*txr.tx));

    // Fill transaction with out data but don't sign
    bool bip32derivs = request.params[4].isNull() ? true : request.params[4].get_bool();
    bool complete = true;
    const TransactionError err{wallet.FillPSBT(psbtx, complete, 1, /*sign=*/false, /*bip32derivs=*/bip32derivs)};
    if (err != TransactionError::OK) {
        throw JSONRPCTransactionError(err);
    }

    // Serialize the PSBT
    DataStream ssTx{};
    ssTx << psbtx;

    UniValue result(UniValue::VOBJ);
    result.pushKV("psbt", EncodeBase64(ssTx.str()));
    result.pushKV("fee", ValueFromAmount(txr.fee));
    result.pushKV("changepos", txr.change_pos ? (int)*txr.change_pos : -1);
    return result;
},
    };
}
} // namespace wallet
