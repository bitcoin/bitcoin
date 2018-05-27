// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include "wallet/coincontrol.h"
#include "wallet/feebumper.h"
#include "wallet/wallet.h"
#include "policy/fees.h"
#include "policy/policy.h"
#include "policy/rbf.h"
#include "validation.h" //for mempool access
#include "txmempool.h"
#include "utilmoneystr.h"
#include "util.h"
#include "net.h"

// Calculate the size of the transaction assuming all signatures are max size
// Use DummySignatureCreator, which inserts 72 byte signatures everywhere.
// TODO: re-use this in CWallet::CreateTransaction (right now
// CreateTransaction uses the constructed dummy-signed tx to do a priority
// calculation, but we should be able to refactor after priority is removed).
// NOTE: this requires that all inputs must be in mapWallet (eg the tx should
// be IsAllFromMe).
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *pWallet)
{
    CMutableTransaction txNew(tx);
    std::vector<CInputCoin> vCoins;
    // Look up the inputs.  We should have already checked that this transaction
    // IsAllFromMe(ISMINE_SPENDABLE), so every input should already be in our
    // wallet, with a valid index into the vout array.
    for (auto& input : tx.vin) {
        const auto mi = pWallet->mapWallet.find(input.prevout.hash);
        assert(mi != pWallet->mapWallet.end() && input.prevout.n < mi->second.tx->vout.size());
        vCoins.emplace_back(CInputCoin(&(mi->second), input.prevout.n));
    }
    if (!pWallet->DummySignTx(txNew, vCoins)) {
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
        return -1;
    }
    return GetVirtualTransactionSize(txNew);
}

bool CFeeBumper::preconditionChecks(const CWallet *pWallet, const CWalletTx& wtx) {
    if (pWallet->HasWalletSpend(wtx.GetHash())) {
        vErrors.push_back("Transaction has descendants in the wallet");
        currentResult = BumpFeeResult::INVALID_PARAMETER;
        return false;
    }

    {
        LOCK(mempool.cs);
        auto it_mp = mempool.mapTx.find(wtx.GetHash());
        if (it_mp != mempool.mapTx.end() && it_mp->GetCountWithDescendants() > 1) {
            vErrors.push_back("Transaction has descendants in the mempool");
            currentResult = BumpFeeResult::INVALID_PARAMETER;
            return false;
        }
    }

    if (wtx.GetDepthInMainChain() != 0) {
        vErrors.push_back("Transaction has been mined, or is conflicted with a mined transaction");
        currentResult = BumpFeeResult::WALLET_ERROR;
        return false;
    }
    return true;
}

CFeeBumper::CFeeBumper(const CWallet *pWallet, const uint256 txidIn, const CCoinControl& coin_control, CAmount totalFee)
    :
    txid(std::move(txidIn)),
    nOldFee(0),
    nNewFee(0)
{
    vErrors.clear();
    bumpedTxid.SetNull();
    AssertLockHeld(pWallet->cs_wallet);
    if (!pWallet->mapWallet.count(txid)) {
        vErrors.push_back("Invalid or non-wallet transaction id");
        currentResult = BumpFeeResult::INVALID_ADDRESS_OR_KEY;
        return;
    }
    auto it = pWallet->mapWallet.find(txid);
    const CWalletTx& wtx = it->second;

    if (!preconditionChecks(pWallet, wtx)) {
        return;
    }

    if (!SignalsOptInRBF(wtx)) {
        vErrors.push_back("Transaction is not BIP 125 replaceable");
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    if (wtx.mapValue.count("replaced_by_txid")) {
        vErrors.push_back(strprintf("Cannot bump transaction %s which was already bumped by transaction %s", txid.ToString(), wtx.mapValue.at("replaced_by_txid")));
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    // check that original tx consists entirely of our inputs
    // if not, we can't bump the fee, because the wallet has no way of knowing the value of the other inputs (thus the fee)
    if (!pWallet->IsAllFromMe(wtx, ISMINE_SPENDABLE)) {
        vErrors.push_back("Transaction contains inputs that don't belong to this wallet");
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    // figure out which output was change
    // if there was no change output or multiple change outputs, fail
    int nOutput = -1;
    for (size_t i = 0; i < wtx.tx->vout.size(); ++i) {
        if (pWallet->IsChange(wtx.tx->vout[i])) {
            if (nOutput != -1) {
                vErrors.push_back("Transaction has multiple change outputs");
                currentResult = BumpFeeResult::WALLET_ERROR;
                return;
            }
            nOutput = i;
        }
    }
    if (nOutput == -1) {
        vErrors.push_back("Transaction does not have a change output");
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    // Calculate the expected size of the new transaction.
    int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    const int64_t maxNewTxSize = CalculateMaximumSignedTxSize(*wtx.tx, pWallet);
    if (maxNewTxSize < 0) {
        vErrors.push_back("Transaction contains inputs that cannot be signed");
        currentResult = BumpFeeResult::INVALID_ADDRESS_OR_KEY;
        return;
    }

    // calculate the old fee and fee-rate
    nOldFee = wtx.GetDebit(ISMINE_SPENDABLE) - wtx.tx->GetValueOut();
    CFeeRate nOldFeeRate(nOldFee, txSize);
    CFeeRate nNewFeeRate;
    // The wallet uses a conservative WALLET_INCREMENTAL_RELAY_FEE value to
    // future proof against changes to network wide policy for incremental relay
    // fee that our node may not be aware of.
    CFeeRate walletIncrementalRelayFee = CFeeRate(WALLET_INCREMENTAL_RELAY_FEE);
    if (::incrementalRelayFee > walletIncrementalRelayFee) {
        walletIncrementalRelayFee = ::incrementalRelayFee;
    }

    if (totalFee > 0) {
        CAmount minTotalFee = nOldFeeRate.GetFee(maxNewTxSize) + ::incrementalRelayFee.GetFee(maxNewTxSize);
        if (totalFee < minTotalFee) {
            vErrors.push_back(strprintf("Insufficient totalFee, must be at least %s (oldFee %s + incrementalFee %s)",
                                                                FormatMoney(minTotalFee), FormatMoney(nOldFeeRate.GetFee(maxNewTxSize)), FormatMoney(::incrementalRelayFee.GetFee(maxNewTxSize))));
            currentResult = BumpFeeResult::INVALID_PARAMETER;
            return;
        }
        CAmount requiredFee = CWallet::GetRequiredFee(maxNewTxSize);
        if (totalFee < requiredFee) {
            vErrors.push_back(strprintf("Insufficient totalFee (cannot be less than required fee %s)",
                                                                FormatMoney(requiredFee)));
            currentResult = BumpFeeResult::INVALID_PARAMETER;
            return;
        }
        nNewFee = totalFee;
        nNewFeeRate = CFeeRate(totalFee, maxNewTxSize);
    } else {
        nNewFee = CWallet::GetMinimumFee(maxNewTxSize, coin_control, mempool, ::feeEstimator, nullptr /* FeeCalculation */);
        nNewFeeRate = CFeeRate(nNewFee, maxNewTxSize);

        // New fee rate must be at least old rate + minimum incremental relay rate
        // walletIncrementalRelayFee.GetFeePerK() should be exact, because it's initialized
        // in that unit (fee per kb).
        // However, nOldFeeRate is a calculated value from the tx fee/size, so
        // add 1 satoshi to the result, because it may have been rounded down.
        if (nNewFeeRate.GetFeePerK() < nOldFeeRate.GetFeePerK() + 1 + walletIncrementalRelayFee.GetFeePerK()) {
            nNewFeeRate = CFeeRate(nOldFeeRate.GetFeePerK() + 1 + walletIncrementalRelayFee.GetFeePerK());
            nNewFee = nNewFeeRate.GetFee(maxNewTxSize);
        }
    }

    // Check that in all cases the new fee doesn't violate maxTxFee
     if (nNewFee > maxTxFee) {
         vErrors.push_back(strprintf("Specified or calculated fee %s is too high (cannot be higher than maxTxFee %s)",
                               FormatMoney(nNewFee), FormatMoney(maxTxFee)));
         currentResult = BumpFeeResult::WALLET_ERROR;
         return;
     }

    // check that fee rate is higher than mempool's minimum fee
    // (no point in bumping fee if we know that the new tx won't be accepted to the mempool)
    // This may occur if the user set TotalFee or paytxfee too low, if fallbackfee is too low, or, perhaps,
    // in a rare situation where the mempool minimum fee increased significantly since the fee estimation just a
    // moment earlier. In this case, we report an error to the user, who may use totalFee to make an adjustment.
    CFeeRate minMempoolFeeRate = mempool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000);
    if (nNewFeeRate.GetFeePerK() < minMempoolFeeRate.GetFeePerK()) {
        vErrors.push_back(strprintf("New fee rate (%s) is less than the minimum fee rate (%s) to get into the mempool. totalFee value should to be at least %s or settxfee value should be at least %s to add transaction.", FormatMoney(nNewFeeRate.GetFeePerK()), FormatMoney(minMempoolFeeRate.GetFeePerK()), FormatMoney(minMempoolFeeRate.GetFee(maxNewTxSize)), FormatMoney(minMempoolFeeRate.GetFeePerK())));
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    // Now modify the output to increase the fee.
    // If the output is not large enough to pay the fee, fail.
    CAmount nDelta = nNewFee - nOldFee;
    assert(nDelta > 0);
    mtx =  *wtx.tx;
    CTxOut* poutput = &(mtx.vout[nOutput]);
    if (poutput->nValue < nDelta) {
        vErrors.push_back("Change output is too small to bump the fee");
        currentResult = BumpFeeResult::WALLET_ERROR;
        return;
    }

    // If the output would become dust, discard it (converting the dust to fee)
    poutput->nValue -= nDelta;
    if (poutput->nValue <= GetDustThreshold(*poutput, ::dustRelayFee)) {
        LogPrint(BCLog::RPC, "Bumping fee and discarding dust output\n");
        nNewFee += poutput->nValue;
        mtx.vout.erase(mtx.vout.begin() + nOutput);
    }

    // Mark new tx not replaceable, if requested.
    if (!coin_control.signalRbf) {
        for (auto& input : mtx.vin) {
            if (input.nSequence < 0xfffffffe) input.nSequence = 0xfffffffe;
        }
    }

    currentResult = BumpFeeResult::OK;
}

bool CFeeBumper::signTransaction(CWallet *pWallet)
{
     return pWallet->SignTransaction(mtx);
}

bool CFeeBumper::commit(CWallet *pWallet)
{
    AssertLockHeld(pWallet->cs_wallet);
    if (!vErrors.empty() || currentResult != BumpFeeResult::OK) {
        return false;
    }
    if (txid.IsNull() || !pWallet->mapWallet.count(txid)) {
        vErrors.push_back("Invalid or non-wallet transaction id");
        currentResult = BumpFeeResult::MISC_ERROR;
        return false;
    }
    CWalletTx& oldWtx = pWallet->mapWallet[txid];

    // make sure the transaction still has no descendants and hasn't been mined in the meantime
    if (!preconditionChecks(pWallet, oldWtx)) {
        return false;
    }

    CWalletTx wtxBumped(pWallet, MakeTransactionRef(std::move(mtx)));
    // commit/broadcast the tx
    CReserveKey reservekey(pWallet);
    wtxBumped.mapValue = oldWtx.mapValue;
    wtxBumped.mapValue["replaces_txid"] = oldWtx.GetHash().ToString();
    wtxBumped.vOrderForm = oldWtx.vOrderForm;
    wtxBumped.strFromAccount = oldWtx.strFromAccount;
    wtxBumped.fTimeReceivedIsTxTime = true;
    wtxBumped.fFromMe = true;
    CValidationState state;
    if (!pWallet->CommitTransaction(wtxBumped, reservekey, g_connman.get(), state)) {
        // NOTE: CommitTransaction never returns false, so this should never happen.
        vErrors.push_back(strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason()));
        return false;
    }

    bumpedTxid = wtxBumped.GetHash();
    if (state.IsInvalid()) {
        // This can happen if the mempool rejected the transaction.  Report
        // what happened in the "errors" response.
        vErrors.push_back(strprintf("Error: The transaction was rejected: %s", FormatStateMessage(state)));
    }

    // mark the original tx as bumped
    if (!pWallet->MarkReplaced(oldWtx.GetHash(), wtxBumped.GetHash())) {
        // TODO: see if JSON-RPC has a standard way of returning a response
        // along with an exception. It would be good to return information about
        // wtxBumped to the caller even if marking the original transaction
        // replaced does not succeed for some reason.
        vErrors.push_back("Error: Created new bumpfee transaction but could not mark the original transaction as replaced.");
    }
    return true;
}

