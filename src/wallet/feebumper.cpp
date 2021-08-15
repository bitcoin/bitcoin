// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/system.h>
#include <util/validation.h>

//! Check whether transaction has descendant in wallet or mempool, or has been
//! mined, or conflicts with a mined transaction. Return a feebumper::Result.
static feebumper::Result PreconditionChecks(interfaces::Chain::Lock& locked_chain, const CWallet* wallet, const CWalletTx& wtx, std::vector<std::string>& errors) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet)
{
    if (wallet->HasWalletSpend(wtx.GetHash())) {
        errors.push_back("Transaction has descendants in the wallet");
        return feebumper::Result::INVALID_PARAMETER;
    }

    {
        if (wallet->chain().hasDescendantsInMempool(wtx.GetHash())) {
            errors.push_back("Transaction has descendants in the mempool");
            return feebumper::Result::INVALID_PARAMETER;
        }
    }

    if (wtx.GetDepthInMainChain(locked_chain) != 0) {
        errors.push_back("Transaction has been mined, or is conflicted with a mined transaction");
        return feebumper::Result::WALLET_ERROR;
    }

    if (!SignalsOptInRBF(*wtx.tx)) {
        errors.push_back("Transaction is not BIP 125 replaceable");
        return feebumper::Result::WALLET_ERROR;
    }

    if (wtx.mapValue.count("replaced_by_txid")) {
        errors.push_back(strprintf("Cannot bump transaction %s which was already bumped by transaction %s", wtx.GetHash().ToString(), wtx.mapValue.at("replaced_by_txid")));
        return feebumper::Result::WALLET_ERROR;
    }

    // check that original tx consists entirely of our inputs
    // if not, we can't bump the fee, because the wallet has no way of knowing the value of the other inputs (thus the fee)
    if (!wallet->IsAllFromMe(*wtx.tx, ISMINE_SPENDABLE)) {
        errors.push_back("Transaction contains inputs that don't belong to this wallet");
        return feebumper::Result::WALLET_ERROR;
    }

    return feebumper::Result::OK;
}

//! Check if the user provided a valid feeRate
static feebumper::Result CheckFeeRate(const CWallet* wallet, const CWalletTx& wtx, const CFeeRate& newFeerate, const int64_t maxTxSize, std::vector<std::string>& errors) {
    // check that fee rate is higher than mempool's minimum fee
    // (no point in bumping fee if we know that the new tx won't be accepted to the mempool)
    // This may occur if the user set FeeRate, TotalFee or paytxfee too low, if fallbackfee is too low, or, perhaps,
    // in a rare situation where the mempool minimum fee increased significantly since the fee estimation just a
    // moment earlier. In this case, we report an error to the user, who may adjust the fee.
    CFeeRate minMempoolFeeRate = wallet->chain().mempoolMinFee();

    if (newFeerate.GetFeePerK() < minMempoolFeeRate.GetFeePerK()) {
        errors.push_back(strprintf(
            "New fee rate (%s) is lower than the minimum fee rate (%s) to get into the mempool -- ",
            FormatMoney(newFeerate.GetFeePerK()),
            FormatMoney(minMempoolFeeRate.GetFeePerK())));
        return feebumper::Result::WALLET_ERROR;
    }

    CAmount new_total_fee = newFeerate.GetFee(maxTxSize);

    CFeeRate incrementalRelayFee = std::max(wallet->chain().relayIncrementalFee(), CFeeRate(WALLET_INCREMENTAL_RELAY_FEE));

    // Given old total fee and transaction size, calculate the old feeRate
    CAmount old_fee = wtx.GetDebit(ISMINE_SPENDABLE) - wtx.tx->GetValueOut();
    const int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    CFeeRate nOldFeeRate(old_fee, txSize);
    // Min total fee is old fee + relay fee
    CAmount minTotalFee = nOldFeeRate.GetFee(maxTxSize) + incrementalRelayFee.GetFee(maxTxSize);

    if (new_total_fee < minTotalFee) {
        errors.push_back(strprintf("Insufficient total fee %s, must be at least %s (oldFee %s + incrementalFee %s)",
            FormatMoney(new_total_fee), FormatMoney(minTotalFee), FormatMoney(nOldFeeRate.GetFee(maxTxSize)), FormatMoney(incrementalRelayFee.GetFee(maxTxSize))));
        return feebumper::Result::INVALID_PARAMETER;
    }

    CAmount requiredFee = GetRequiredFee(*wallet, maxTxSize);
    if (new_total_fee < requiredFee) {
        errors.push_back(strprintf("Insufficient total fee (cannot be less than required fee %s)",
            FormatMoney(requiredFee)));
        return feebumper::Result::INVALID_PARAMETER;
    }

    // Check that in all cases the new fee doesn't violate maxTxFee
    const CAmount max_tx_fee = wallet->m_default_max_tx_fee;
    if (new_total_fee > max_tx_fee) {
        errors.push_back(strprintf("Specified or calculated fee %s is too high (cannot be higher than -maxtxfee %s)",
                            FormatMoney(new_total_fee), FormatMoney(max_tx_fee)));
        return feebumper::Result::WALLET_ERROR;
    }

    return feebumper::Result::OK;
}

static CFeeRate EstimateFeeRate(CWallet* wallet, const CWalletTx& wtx, const CAmount old_fee, CCoinControl& coin_control)
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
    // aware of. This ensures we're over the over the required relay fee rate
    // (BIP 125 rule 4).  The replacement tx will be at least as large as the
    // original tx, so the total fee will be greater (BIP 125 rule 3)
    CFeeRate node_incremental_relay_fee = wallet->chain().relayIncrementalFee();
    CFeeRate wallet_incremental_relay_fee = CFeeRate(WALLET_INCREMENTAL_RELAY_FEE);
    feerate += std::max(node_incremental_relay_fee, wallet_incremental_relay_fee);

    // Fee rate must also be at least the wallet's GetMinimumFeeRate
    CFeeRate min_feerate(GetMinimumFeeRate(*wallet, coin_control, /* feeCalc */ nullptr));

    // Set the required fee rate for the replacement transaction in coin control.
    return std::max(feerate, min_feerate);
}

namespace feebumper {

bool TransactionCanBeBumped(const CWallet* wallet, const uint256& txid)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    const CWalletTx* wtx = wallet->GetWalletTx(txid);
    if (wtx == nullptr) return false;

    std::vector<std::string> errors_dummy;
    feebumper::Result res = PreconditionChecks(*locked_chain, wallet, *wtx, errors_dummy);
    return res == feebumper::Result::OK;
}

Result CreateTotalBumpTransaction(const CWallet* wallet, const uint256& txid, const CCoinControl& coin_control, CAmount total_fee, std::vector<std::string>& errors,
                                  CAmount& old_fee, CAmount& new_fee, CMutableTransaction& mtx)
{
    new_fee = total_fee;

    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    errors.clear();
    auto it = wallet->mapWallet.find(txid);
    if (it == wallet->mapWallet.end()) {
        errors.push_back("Invalid or non-wallet transaction id");
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    const CWalletTx& wtx = it->second;

    Result result = PreconditionChecks(*locked_chain, wallet, wtx, errors);
    if (result != Result::OK) {
        return result;
    }

    // figure out which output was change
    // if there was no change output or multiple change outputs, fail
    int nOutput = -1;
    for (size_t i = 0; i < wtx.tx->vout.size(); ++i) {
        if (wallet->IsChange(wtx.tx->vout[i])) {
            if (nOutput != -1) {
                errors.push_back("Transaction has multiple change outputs");
                return Result::WALLET_ERROR;
            }
            nOutput = i;
        }
    }
    if (nOutput == -1) {
        errors.push_back("Transaction does not have a change output");
        return Result::WALLET_ERROR;
    }

    // Calculate the expected size of the new transaction.
    int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    const int64_t maxNewTxSize = CalculateMaximumSignedTxSize(*wtx.tx, wallet);
    if (maxNewTxSize < 0) {
        errors.push_back("Transaction contains inputs that cannot be signed");
        return Result::INVALID_ADDRESS_OR_KEY;
    }

    // calculate the old fee and fee-rate
    old_fee = wtx.GetDebit(ISMINE_SPENDABLE) - wtx.tx->GetValueOut();
    CFeeRate nOldFeeRate(old_fee, txSize);
    // The wallet uses a conservative WALLET_INCREMENTAL_RELAY_FEE value to
    // future proof against changes to network wide policy for incremental relay
    // fee that our node may not be aware of.
    CFeeRate nodeIncrementalRelayFee = wallet->chain().relayIncrementalFee();
    CFeeRate walletIncrementalRelayFee = CFeeRate(WALLET_INCREMENTAL_RELAY_FEE);
    if (nodeIncrementalRelayFee > walletIncrementalRelayFee) {
        walletIncrementalRelayFee = nodeIncrementalRelayFee;
    }

    CAmount minTotalFee = nOldFeeRate.GetFee(maxNewTxSize) + nodeIncrementalRelayFee.GetFee(maxNewTxSize);
    if (total_fee < minTotalFee) {
        errors.push_back(strprintf("Insufficient totalFee, must be at least %s (oldFee %s + incrementalFee %s)",
            FormatMoney(minTotalFee), FormatMoney(nOldFeeRate.GetFee(maxNewTxSize)), FormatMoney(nodeIncrementalRelayFee.GetFee(maxNewTxSize))));
        return Result::INVALID_PARAMETER;
    }
    CAmount requiredFee = GetRequiredFee(*wallet, maxNewTxSize);
    if (total_fee < requiredFee) {
        errors.push_back(strprintf("Insufficient totalFee (cannot be less than required fee %s)",
            FormatMoney(requiredFee)));
        return Result::INVALID_PARAMETER;
    }

    // Check that in all cases the new fee doesn't violate maxTxFee
     const CAmount max_tx_fee = wallet->m_default_max_tx_fee;
     if (new_fee > max_tx_fee) {
         errors.push_back(strprintf("Specified or calculated fee %s is too high (cannot be higher than -maxtxfee %s)",
                               FormatMoney(new_fee), FormatMoney(max_tx_fee)));
         return Result::WALLET_ERROR;
     }

    // check that fee rate is higher than mempool's minimum fee
    // (no point in bumping fee if we know that the new tx won't be accepted to the mempool)
    // This may occur if the user set TotalFee or paytxfee too low, if fallbackfee is too low, or, perhaps,
    // in a rare situation where the mempool minimum fee increased significantly since the fee estimation just a
    // moment earlier. In this case, we report an error to the user, who may use total_fee to make an adjustment.
    CFeeRate minMempoolFeeRate = wallet->chain().mempoolMinFee();
    CFeeRate nNewFeeRate = CFeeRate(total_fee, maxNewTxSize);
    if (nNewFeeRate.GetFeePerK() < minMempoolFeeRate.GetFeePerK()) {
        errors.push_back(strprintf(
            "New fee rate (%s) is lower than the minimum fee rate (%s) to get into the mempool -- "
            "the totalFee value should be at least %s to add transaction",
            FormatMoney(nNewFeeRate.GetFeePerK()),
            FormatMoney(minMempoolFeeRate.GetFeePerK()),
            FormatMoney(minMempoolFeeRate.GetFee(maxNewTxSize))));
        return Result::WALLET_ERROR;
    }

    // Now modify the output to increase the fee.
    // If the output is not large enough to pay the fee, fail.
    CAmount nDelta = new_fee - old_fee;
    assert(nDelta > 0);
    mtx = CMutableTransaction{*wtx.tx};
    CTxOut* poutput = &(mtx.vout[nOutput]);
    if (poutput->nValue < nDelta) {
        errors.push_back("Change output is too small to bump the fee");
        return Result::WALLET_ERROR;
    }

    // If the output would become dust, discard it (converting the dust to fee)
    poutput->nValue -= nDelta;
    if (poutput->nValue <= GetDustThreshold(*poutput, GetDiscardRate(*wallet))) {
        wallet->WalletLogPrintf("Bumping fee and discarding dust output\n");
        new_fee += poutput->nValue;
        mtx.vout.erase(mtx.vout.begin() + nOutput);
    }

    // Mark new tx not replaceable, if requested.
    if (!coin_control.m_signal_bip125_rbf.get_value_or(wallet->m_signal_rbf)) {
        for (auto& input : mtx.vin) {
            if (input.nSequence < 0xfffffffe) input.nSequence = 0xfffffffe;
        }
    }

    return Result::OK;
}


Result CreateRateBumpTransaction(CWallet* wallet, const uint256& txid, const CCoinControl& coin_control, std::vector<std::string>& errors,
                                 CAmount& old_fee, CAmount& new_fee, CMutableTransaction& mtx)
{
    // We are going to modify coin control later, copy to re-use
    CCoinControl new_coin_control(coin_control);

    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    errors.clear();
    auto it = wallet->mapWallet.find(txid);
    if (it == wallet->mapWallet.end()) {
        errors.push_back("Invalid or non-wallet transaction id");
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    const CWalletTx& wtx = it->second;

    Result result = PreconditionChecks(*locked_chain, wallet, wtx, errors);
    if (result != Result::OK) {
        return result;
    }

    // Fill in recipients(and preserve a single change key if there is one)
    std::vector<CRecipient> recipients;
    for (const auto& output : wtx.tx->vout) {
        if (!wallet->IsChange(output)) {
            CRecipient recipient = {output.scriptPubKey, output.nValue, false};
            recipients.push_back(recipient);
        } else {
            CTxDestination change_dest;
            ExtractDestination(output.scriptPubKey, change_dest);
            new_coin_control.destChange = change_dest;
        }
    }

    old_fee = wtx.GetDebit(ISMINE_SPENDABLE) - wtx.tx->GetValueOut();

    if (coin_control.m_feerate) {
        // The user provided a feeRate argument.
        // We calculate this here to avoid compiler warning on the cs_wallet lock
        const int64_t maxTxSize = CalculateMaximumSignedTxSize(*wtx.tx, wallet);
        Result res = CheckFeeRate(wallet, wtx, *(new_coin_control.m_feerate), maxTxSize, errors);
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
    new_coin_control.fAllowOtherInputs = true;

    // We cannot source new unconfirmed inputs(bip125 rule 2)
    new_coin_control.m_min_depth = 1;

    CTransactionRef tx_new = MakeTransactionRef();
    CAmount fee_ret;
    int change_pos_in_out = -1; // No requested location for change
    std::string fail_reason;
    if (!wallet->CreateTransaction(*locked_chain, recipients, tx_new, fee_ret, change_pos_in_out, fail_reason, new_coin_control, false)) {
        errors.push_back("Unable to create transaction: " + fail_reason);
        return Result::WALLET_ERROR;
    }

    // Write back new fee if successful
    new_fee = fee_ret;

    // Write back transaction
    mtx = CMutableTransaction(*tx_new);
    // Mark new tx not replaceable, if requested.
    if (!coin_control.m_signal_bip125_rbf.get_value_or(wallet->m_signal_rbf)) {
        for (auto& input : mtx.vin) {
            if (input.nSequence < 0xfffffffe) input.nSequence = 0xfffffffe;
        }
    }

    return Result::OK;
}

bool SignTransaction(CWallet* wallet, CMutableTransaction& mtx) {
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    return wallet->SignTransaction(mtx);
}

Result CommitTransaction(CWallet* wallet, const uint256& txid, CMutableTransaction&& mtx, std::vector<std::string>& errors, uint256& bumped_txid)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    if (!errors.empty()) {
        return Result::MISC_ERROR;
    }
    auto it = txid.IsNull() ? wallet->mapWallet.end() : wallet->mapWallet.find(txid);
    if (it == wallet->mapWallet.end()) {
        errors.push_back("Invalid or non-wallet transaction id");
        return Result::MISC_ERROR;
    }
    CWalletTx& oldWtx = it->second;

    // make sure the transaction still has no descendants and hasn't been mined in the meantime
    Result result = PreconditionChecks(*locked_chain, wallet, oldWtx, errors);
    if (result != Result::OK) {
        return result;
    }

    // commit/broadcast the tx
    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    mapValue_t mapValue = oldWtx.mapValue;
    mapValue["replaces_txid"] = oldWtx.GetHash().ToString();

    CValidationState state;
    if (!wallet->CommitTransaction(tx, std::move(mapValue), oldWtx.vOrderForm, state)) {
        // NOTE: CommitTransaction never returns false, so this should never happen.
        errors.push_back(strprintf("The transaction was rejected: %s", FormatStateMessage(state)));
        return Result::WALLET_ERROR;
    }

    bumped_txid = tx->GetHash();
    if (state.IsInvalid()) {
        // This can happen if the mempool rejected the transaction.  Report
        // what happened in the "errors" response.
        errors.push_back(strprintf("Error: The transaction was rejected: %s", FormatStateMessage(state)));
    }

    // mark the original tx as bumped
    if (!wallet->MarkReplaced(oldWtx.GetHash(), bumped_txid)) {
        // TODO: see if JSON-RPC has a standard way of returning a response
        // along with an exception. It would be good to return information about
        // wtxBumped to the caller even if marking the original transaction
        // replaced does not succeed for some reason.
        errors.push_back("Created new bumpfee transaction but could not mark the original transaction as replaced");
    }
    return Result::OK;
}

} // namespace feebumper
