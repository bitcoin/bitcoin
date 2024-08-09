// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <node/types.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

namespace wallet {
//! Check whether transaction has descendant in wallet or mempool, or has been
//! mined, or conflicts with a mined transaction. Return a feebumper::Result.
static feebumper::Result PreconditionChecks(const CWallet& wallet, const CWalletTx& wtx, bool require_mine, std::vector<bilingual_str>& errors) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    if (wallet.HasWalletSpend(wtx.tx)) {
        errors.push_back(Untranslated("Transaction has descendants in the wallet"));
        return feebumper::Result::INVALID_PARAMETER;
    }

    {
        if (wallet.chain().hasDescendantsInMempool(wtx.GetHash())) {
            errors.push_back(Untranslated("Transaction has descendants in the mempool"));
            return feebumper::Result::INVALID_PARAMETER;
        }
    }

    if (wallet.GetTxDepthInMainChain(wtx) != 0) {
        errors.push_back(Untranslated("Transaction has been mined, or is conflicted with a mined transaction"));
        return feebumper::Result::WALLET_ERROR;
    }

    if (!SignalsOptInRBF(*wtx.tx)) {
        errors.push_back(Untranslated("Transaction is not BIP 125 replaceable"));
        return feebumper::Result::WALLET_ERROR;
    }

    if (wtx.mapValue.count("replaced_by_txid")) {
        errors.push_back(strprintf(Untranslated("Cannot bump transaction %s which was already bumped by transaction %s"), wtx.GetHash().ToString(), wtx.mapValue.at("replaced_by_txid")));
        return feebumper::Result::WALLET_ERROR;
    }

    if (require_mine) {
        // check that original tx consists entirely of our inputs
        // if not, we can't bump the fee, because the wallet has no way of knowing the value of the other inputs (thus the fee)
        isminefilter filter = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
        if (!AllInputsMine(wallet, *wtx.tx, filter)) {
            errors.push_back(Untranslated("Transaction contains inputs that don't belong to this wallet"));
            return feebumper::Result::WALLET_ERROR;
        }
    }

    return feebumper::Result::OK;
}

//! Check if the user provided a valid feeRate
static feebumper::Result CheckFeeRate(const CWallet& wallet, const CMutableTransaction& mtx, const CFeeRate& newFeerate, const int64_t maxTxSize, CAmount old_fee, std::vector<bilingual_str>& errors)
{
    // check that fee rate is higher than mempool's minimum fee
    // (no point in bumping fee if we know that the new tx won't be accepted to the mempool)
    // This may occur if the user set fee_rate or paytxfee too low, if fallbackfee is too low, or, perhaps,
    // in a rare situation where the mempool minimum fee increased significantly since the fee estimation just a
    // moment earlier. In this case, we report an error to the user, who may adjust the fee.
    CFeeRate minMempoolFeeRate = wallet.chain().mempoolMinFee();

    if (newFeerate.GetFeePerK() < minMempoolFeeRate.GetFeePerK()) {
        errors.push_back(strprintf(
            Untranslated("New fee rate (%s) is lower than the minimum fee rate (%s) to get into the mempool -- "),
            FormatMoney(newFeerate.GetFeePerK()),
            FormatMoney(minMempoolFeeRate.GetFeePerK())));
        return feebumper::Result::WALLET_ERROR;
    }

    std::vector<COutPoint> reused_inputs;
    reused_inputs.reserve(mtx.vin.size());
    for (const CTxIn& txin : mtx.vin) {
        reused_inputs.push_back(txin.prevout);
    }

    std::optional<CAmount> combined_bump_fee = wallet.chain().calculateCombinedBumpFee(reused_inputs, newFeerate);
    if (!combined_bump_fee.has_value()) {
        errors.push_back(strprintf(Untranslated("Failed to calculate bump fees, because unconfirmed UTXOs depend on enormous cluster of unconfirmed transactions.")));
    }
    CAmount new_total_fee = newFeerate.GetFee(maxTxSize) + combined_bump_fee.value();

    CFeeRate incrementalRelayFee = wallet.chain().relayIncrementalFee();

    // Min total fee is old fee + relay fee
    CAmount minTotalFee = old_fee + incrementalRelayFee.GetFee(maxTxSize);

    if (new_total_fee < minTotalFee) {
        errors.push_back(strprintf(Untranslated("Insufficient total fee %s, must be at least %s (oldFee %s + incrementalFee %s)"),
            FormatMoney(new_total_fee), FormatMoney(minTotalFee), FormatMoney(old_fee), FormatMoney(incrementalRelayFee.GetFee(maxTxSize))));
        return feebumper::Result::INVALID_PARAMETER;
    }

    CAmount requiredFee = GetRequiredFee(wallet, maxTxSize);
    if (new_total_fee < requiredFee) {
        errors.push_back(strprintf(Untranslated("Insufficient total fee (cannot be less than required fee %s)"),
            FormatMoney(requiredFee)));
        return feebumper::Result::INVALID_PARAMETER;
    }

    // Check that in all cases the new fee doesn't violate maxTxFee
    const CAmount max_tx_fee = wallet.m_default_max_tx_fee;
    if (new_total_fee > max_tx_fee) {
        errors.push_back(strprintf(Untranslated("Specified or calculated fee %s is too high (cannot be higher than -maxtxfee %s)"),
            FormatMoney(new_total_fee), FormatMoney(max_tx_fee)));
        return feebumper::Result::WALLET_ERROR;
    }

    return feebumper::Result::OK;
}

static CFeeRate EstimateFeeRate(const CWallet& wallet, const CWalletTx& wtx, const CAmount old_fee, const CCoinControl& coin_control)
{
    // Get the fee rate of the original transaction. This is calculated from
    // the tx fee/vsize, so it may have been rounded down. Add 1 satoshi to the
    // result.
    int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    CFeeRate feerate(old_fee, txSize);
    feerate += CFeeRate(1);

    // The node has a configurable incremental relay fee. Increment the fee by
    // the minimum of that and the wallet's conservative
    // WALLET_INCREMENTAL_RELAY_FEE value to future proof against changes to
    // network wide policy for incremental relay fee that our node may not be
    // aware of. This ensures we're over the required relay fee rate
    // (Rule 4).  The replacement tx will be at least as large as the
    // original tx, so the total fee will be greater (Rule 3)
    CFeeRate node_incremental_relay_fee = wallet.chain().relayIncrementalFee();
    CFeeRate wallet_incremental_relay_fee = CFeeRate(WALLET_INCREMENTAL_RELAY_FEE);
    feerate += std::max(node_incremental_relay_fee, wallet_incremental_relay_fee);

    // Fee rate must also be at least the wallet's GetMinimumFeeRate
    CFeeRate min_feerate(GetMinimumFeeRate(wallet, coin_control, /*feeCalc=*/nullptr));

    // Set the required fee rate for the replacement transaction in coin control.
    return std::max(feerate, min_feerate);
}

namespace feebumper {

bool TransactionCanBeBumped(const CWallet& wallet, const uint256& txid)
{
    LOCK(wallet.cs_wallet);
    const CWalletTx* wtx = wallet.GetWalletTx(txid);
    if (wtx == nullptr) return false;

    std::vector<bilingual_str> errors_dummy;
    feebumper::Result res = PreconditionChecks(wallet, *wtx, /* require_mine=*/ true, errors_dummy);
    return res == feebumper::Result::OK;
}

Result CreateRateBumpTransaction(CWallet& wallet, const uint256& txid, const CCoinControl& coin_control, std::vector<bilingual_str>& errors,
                                 CAmount& old_fee, CAmount& new_fee, CMutableTransaction& mtx, bool require_mine, const std::vector<CTxOut>& outputs, std::optional<uint32_t> original_change_index)
{
    // For now, cannot specify both new outputs to use and an output index to send change
    if (!outputs.empty() && original_change_index.has_value()) {
        errors.push_back(Untranslated("The options 'outputs' and 'original_change_index' are incompatible. You can only either specify a new set of outputs, or designate a change output to be recycled."));
        return Result::INVALID_PARAMETER;
    }

    // We are going to modify coin control later, copy to reuse
    CCoinControl new_coin_control(coin_control);

    LOCK(wallet.cs_wallet);
    errors.clear();
    auto it = wallet.mapWallet.find(txid);
    if (it == wallet.mapWallet.end()) {
        errors.push_back(Untranslated("Invalid or non-wallet transaction id"));
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    const CWalletTx& wtx = it->second;

    // Make sure that original_change_index is valid
    if (original_change_index.has_value() && original_change_index.value() >= wtx.tx->vout.size()) {
        errors.push_back(Untranslated("Change position is out of range"));
        return Result::INVALID_PARAMETER;
    }

    // Retrieve all of the UTXOs and add them to coin control
    // While we're here, calculate the input amount
    std::map<COutPoint, Coin> coins;
    CAmount input_value = 0;
    std::vector<CTxOut> spent_outputs;
    for (const CTxIn& txin : wtx.tx->vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    wallet.chain().findCoins(coins);
    for (const CTxIn& txin : wtx.tx->vin) {
        const Coin& coin = coins.at(txin.prevout);
        if (coin.out.IsNull()) {
            errors.push_back(Untranslated(strprintf("%s:%u is already spent", txin.prevout.hash.GetHex(), txin.prevout.n)));
            return Result::MISC_ERROR;
        }
        PreselectedInput& preset_txin = new_coin_control.Select(txin.prevout);
        if (!wallet.IsMine(txin.prevout)) {
            preset_txin.SetTxOut(coin.out);
        }
        input_value += coin.out.nValue;
        spent_outputs.push_back(coin.out);
    }

    // Figure out if we need to compute the input weight, and do so if necessary
    PrecomputedTransactionData txdata;
    txdata.Init(*wtx.tx, std::move(spent_outputs), /* force=*/ true);
    for (unsigned int i = 0; i < wtx.tx->vin.size(); ++i) {
        const CTxIn& txin = wtx.tx->vin.at(i);
        const Coin& coin = coins.at(txin.prevout);

        if (new_coin_control.IsExternalSelected(txin.prevout)) {
            // For external inputs, we estimate the size using the size of this input
            int64_t input_weight = GetTransactionInputWeight(txin);
            // Because signatures can have different sizes, we need to figure out all of the
            // signature sizes and replace them with the max sized signature.
            // In order to do this, we verify the script with a special SignatureChecker which
            // will observe the signatures verified and record their sizes.
            SignatureWeights weights;
            TransactionSignatureChecker tx_checker(wtx.tx.get(), i, coin.out.nValue, txdata, MissingDataBehavior::FAIL);
            SignatureWeightChecker size_checker(weights, tx_checker);
            VerifyScript(txin.scriptSig, coin.out.scriptPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, size_checker);
            // Add the difference between max and current to input_weight so that it represents the largest the input could be
            input_weight += weights.GetWeightDiffToMax();
            new_coin_control.SetInputWeight(txin.prevout, input_weight);
        }
    }

    Result result = PreconditionChecks(wallet, wtx, require_mine, errors);
    if (result != Result::OK) {
        return result;
    }

    // Calculate the old output amount.
    CAmount output_value = 0;
    for (const auto& old_output : wtx.tx->vout) {
        output_value += old_output.nValue;
    }

    old_fee = input_value - output_value;

    // Fill in recipients (and preserve a single change key if there
    // is one). If outputs vector is non-empty, replace original
    // outputs with its contents, otherwise use original outputs.
    std::vector<CRecipient> recipients;
    CAmount new_outputs_value = 0;
    const auto& txouts = outputs.empty() ? wtx.tx->vout : outputs;
    for (size_t i = 0; i < txouts.size(); ++i) {
        const CTxOut& output = txouts.at(i);
        CTxDestination dest;
        ExtractDestination(output.scriptPubKey, dest);
        if (original_change_index.has_value() ?  original_change_index.value() == i : OutputIsChange(wallet, output)) {
            new_coin_control.destChange = dest;
        } else {
            CRecipient recipient = {dest, output.nValue, false};
            recipients.push_back(recipient);
        }
        new_outputs_value += output.nValue;
    }

    // If no recipients, means that we are sending coins to a change address
    if (recipients.empty()) {
        // Just as a sanity check, ensure that the change address exist
        if (std::get_if<CNoDestination>(&new_coin_control.destChange)) {
            errors.emplace_back(Untranslated("Unable to create transaction. Transaction must have at least one recipient"));
            return Result::INVALID_PARAMETER;
        }

        // Add change as recipient with SFFO flag enabled, so fees are deduced from it.
        // If the output differs from the original tx output (because the user customized it) a new change output will be created.
        recipients.emplace_back(CRecipient{new_coin_control.destChange, new_outputs_value, /*fSubtractFeeFromAmount=*/true});
        new_coin_control.destChange = CNoDestination();
    }

    if (coin_control.m_feerate) {
        // The user provided a feeRate argument.
        // We calculate this here to avoid compiler warning on the cs_wallet lock
        // We need to make a temporary transaction with no input witnesses as the dummy signer expects them to be empty for external inputs
        CMutableTransaction temp_mtx{*wtx.tx};
        for (auto& txin : temp_mtx.vin) {
            txin.scriptSig.clear();
            txin.scriptWitness.SetNull();
        }
        temp_mtx.vout = txouts;
        const int64_t maxTxSize{CalculateMaximumSignedTxSize(CTransaction(temp_mtx), &wallet, &new_coin_control).vsize};
        Result res = CheckFeeRate(wallet, temp_mtx, *new_coin_control.m_feerate, maxTxSize, old_fee, errors);
        if (res != Result::OK) {
            return res;
        }
    } else {
        // The user did not provide a feeRate argument
        new_coin_control.m_feerate = EstimateFeeRate(wallet, wtx, old_fee, new_coin_control);
    }

    // Fill in required inputs we are double-spending(all of them)
    // N.B.: bip125 doesn't require all the inputs in the replaced transaction to be
    // used in the replacement transaction, but it's very important for wallets to make
    // sure that happens. If not, it would be possible to bump a transaction A twice to
    // A2 and A3 where A2 and A3 don't conflict (or alternatively bump A to A2 and A2
    // to A3 where A and A3 don't conflict). If both later get confirmed then the sender
    // has accidentally double paid.
    for (const auto& inputs : wtx.tx->vin) {
        new_coin_control.Select(COutPoint(inputs.prevout));
    }
    new_coin_control.m_allow_other_inputs = true;

    // We cannot source new unconfirmed inputs(bip125 rule 2)
    new_coin_control.m_min_depth = 1;

    auto res = CreateTransaction(wallet, recipients, /*change_pos=*/std::nullopt, new_coin_control, false);
    if (!res) {
        errors.push_back(Untranslated("Unable to create transaction.") + Untranslated(" ") + util::ErrorString(res));
        return Result::WALLET_ERROR;
    }

    const auto& txr = *res;
    // Write back new fee if successful
    new_fee = txr.fee;

    // Write back transaction
    mtx = CMutableTransaction(*txr.tx);

    return Result::OK;
}

bool SignTransaction(CWallet& wallet, CMutableTransaction& mtx) {
    LOCK(wallet.cs_wallet);

    if (wallet.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER)) {
        // Make a blank psbt
        PartiallySignedTransaction psbtx(mtx);

        // First fill transaction with our data without signing,
        // so external signers are not asked to sign more than once.
        bool complete;
        wallet.FillPSBT(psbtx, complete, SIGHASH_ALL, false /* sign */, true /* bip32derivs */);
        auto err{wallet.FillPSBT(psbtx, complete, SIGHASH_ALL, true /* sign */, false  /* bip32derivs */)};
        if (err) return false;
        complete = FinalizeAndExtractPSBT(psbtx, mtx);
        return complete;
    } else {
        return wallet.SignTransaction(mtx);
    }
}

Result CommitTransaction(CWallet& wallet, const uint256& txid, CMutableTransaction&& mtx, std::vector<bilingual_str>& errors, uint256& bumped_txid)
{
    LOCK(wallet.cs_wallet);
    if (!errors.empty()) {
        return Result::MISC_ERROR;
    }
    auto it = txid.IsNull() ? wallet.mapWallet.end() : wallet.mapWallet.find(txid);
    if (it == wallet.mapWallet.end()) {
        errors.push_back(Untranslated("Invalid or non-wallet transaction id"));
        return Result::MISC_ERROR;
    }
    const CWalletTx& oldWtx = it->second;

    // make sure the transaction still has no descendants and hasn't been mined in the meantime
    Result result = PreconditionChecks(wallet, oldWtx, /* require_mine=*/ false, errors);
    if (result != Result::OK) {
        return result;
    }

    // commit/broadcast the tx
    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    mapValue_t mapValue = oldWtx.mapValue;
    mapValue["replaces_txid"] = oldWtx.GetHash().ToString();

    wallet.CommitTransaction(tx, std::move(mapValue), oldWtx.vOrderForm);

    // mark the original tx as bumped
    bumped_txid = tx->GetHash();
    if (!wallet.MarkReplaced(oldWtx.GetHash(), bumped_txid)) {
        errors.push_back(Untranslated("Created new bumpfee transaction but could not mark the original transaction as replaced"));
    }
    return Result::OK;
}

Result CreateRateBumpDeniabilizationTransaction(CWallet& wallet, const uint256& txid, unsigned int confirm_target, bool sign, bilingual_str& error, CAmount& old_fee, CAmount& new_fee, CTransactionRef& new_tx)
{
    CCoinControl coin_control = SetupDeniabilizationCoinControl(confirm_target);
    coin_control.m_feerate = CalculateDeniabilizationFeeRate(wallet, confirm_target);

    LOCK(wallet.cs_wallet);

    auto it = wallet.mapWallet.find(txid);
    if (it == wallet.mapWallet.end()) {
        error = Untranslated("Invalid or non-wallet transaction id");
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    const CWalletTx& wtx = it->second;

    // Retrieve all of the UTXOs and add them to coin control
    // While we're here, calculate the input amount
    std::map<COutPoint, Coin> coins;
    CAmount input_value = 0;
    for (const CTxIn& txin : wtx.tx->vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    wallet.chain().findCoins(coins);
    for (const CTxIn& txin : wtx.tx->vin) {
        const Coin& coin = coins.at(txin.prevout);
        if (coin.out.IsNull()) {
            error = Untranslated(strprintf("%s:%u is already spent", txin.prevout.hash.GetHex(), txin.prevout.n));
            return Result::MISC_ERROR;
        }
        if (!wallet.IsMine(txin.prevout)) {
            error = Untranslated("All inputs must be from our wallet.");
            return Result::MISC_ERROR;
        }
        coin_control.Select(txin.prevout);
        input_value += coin.out.nValue;
    }

    std::vector<bilingual_str> dymmy_errors;
    Result result = PreconditionChecks(wallet, wtx, /*require_mine=*/true, dymmy_errors);
    if (result != Result::OK) {
        error = dymmy_errors.front();
        return result;
    }

    // Calculate the old output amount.
    CAmount output_value = 0;
    for (const auto& old_output : wtx.tx->vout) {
        output_value += old_output.nValue;
    }

    old_fee = input_value - output_value;

    std::vector<CRecipient> recipients;
    for (const auto& output : wtx.tx->vout) {
        CTxDestination destination = CNoDestination();
        ExtractDestination(output.scriptPubKey, destination);
        CRecipient recipient = {destination, output.nValue, false};
        recipients.push_back(recipient);
    }
    // the last recipient gets the old fee
    recipients.back().nAmount += old_fee;
    // and pays the new fee
    recipients.back().fSubtractFeeFromAmount = true;
    // we don't expect to get change, but we provide the address to prevent CreateTransactionInternal from generating a change address
    coin_control.destChange = recipients.back().dest;

    for (const auto& inputs : wtx.tx->vin) {
        coin_control.Select(COutPoint(inputs.prevout));
    }

    auto res = CreateTransaction(wallet, recipients, std::nullopt, coin_control, /*sign=*/false);
    if (!res) {
        error = util::ErrorString(res);
        return Result::WALLET_ERROR;
    }

    // make sure we didn't get a change position assigned (we don't expect to use the channge address)
    Assert(!res->change_pos.has_value());

    // spoof the transaction fingerprint to increase the transaction privacy
    {
        FastRandomContext rng_fast;
        CMutableTransaction spoofedTx(*res->tx);
        SpoofTransactionFingerprint(spoofedTx, rng_fast, coin_control.m_signal_bip125_rbf);
        if (sign && !wallet.SignTransaction(spoofedTx)) {
            error = Untranslated("Signing the deniabilization fee bump transaction failed.");
            return Result::MISC_ERROR;
        }
        // store the spoofed transaction in the result
        res->tx = MakeTransactionRef(std::move(spoofedTx));
    }

    // write back the new fee
    new_fee = res->fee;
    // write back the transaction
    new_tx = res->tx;
    return Result::OK;
}

} // namespace feebumper
} // namespace wallet
