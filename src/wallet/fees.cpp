// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/fees.h>

#include <policy/policy.h>
#include <txmempool.h>
#include <util.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>


CAmount GetRequiredFee(const CWallet& wallet, unsigned int nTxBytes)
{
    return GetRequiredFeeRate(wallet).GetFee(nTxBytes);
}

CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes, const CCoinControl& coin_control, const CTxMemPool& pool, const CBlockPolicyEstimator& estimator, FeeCalculation* feeCalc)
{
    CAmount fee_needed = GetMinimumFeeRate(wallet, coin_control, pool, estimator, feeCalc).GetFee(nTxBytes);
    // Always obey the maximum
    if (fee_needed > maxTxFee) {
        fee_needed = maxTxFee;
        if (feeCalc) feeCalc->reason = FeeReason::MAXTXFEE;
    }
    return fee_needed;
}

CFeeRate GetRequiredFeeRate(const CWallet& wallet)
{
    return std::max(wallet.m_min_fee, ::minRelayTxFee);
}

CFeeRate GetMinimumFeeRate(const CWallet& wallet, const CCoinControl& coin_control, const CTxMemPool& pool, const CBlockPolicyEstimator& estimator, FeeCalculation* feeCalc)
{
    /* User control of how to calculate fee uses the following parameter precedence:
       1. coin_control.m_feerate
       2. coin_control.m_confirm_target
       3. m_pay_tx_fee (user-set member variable of wallet)
       4. m_confirm_target (user-set member variable of wallet)
       The first parameter that is set is used.
    */
    CFeeRate feerate_needed;
    if (coin_control.m_feerate) { // 1.
        feerate_needed = *(coin_control.m_feerate);
        if (feeCalc) feeCalc->reason = FeeReason::PAYTXFEE;
        // Allow to override automatic min/max check over coin control instance
        if (coin_control.fOverrideFeeRate) return feerate_needed;
    }
    else if (!coin_control.m_confirm_target && wallet.m_pay_tx_fee != CFeeRate(0)) { // 3. TODO: remove magic value of 0 for wallet member m_pay_tx_fee
        feerate_needed = wallet.m_pay_tx_fee;
        if (feeCalc) feeCalc->reason = FeeReason::PAYTXFEE;
    }
    else { // 2. or 4.
        // We will use smart fee estimation
        unsigned int target = coin_control.m_confirm_target ? *coin_control.m_confirm_target : wallet.m_confirm_target;
        // By default estimates are economical
        bool conservative_estimate = true;
        // Allow to override the default fee estimate mode over the CoinControl instance
        if (coin_control.m_fee_mode == FeeEstimateMode::CONSERVATIVE) conservative_estimate = true;
        else if (coin_control.m_fee_mode == FeeEstimateMode::ECONOMICAL) conservative_estimate = false;

        feerate_needed = estimator.estimateSmartFee(target, feeCalc, conservative_estimate);
        if (feerate_needed == CFeeRate(0)) {
            // if we don't have enough data for estimateSmartFee, then use fallback fee
            feerate_needed = wallet.m_fallback_fee;
            if (feeCalc) feeCalc->reason = FeeReason::FALLBACK;
        }
        // Obey mempool min fee when using smart fee estimation
        CFeeRate min_mempool_feerate = pool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000);
        if (feerate_needed < min_mempool_feerate) {
            feerate_needed = min_mempool_feerate;
            if (feeCalc) feeCalc->reason = FeeReason::MEMPOOL_MIN;
        }
    }

    // prevent user from paying a fee below the required fee rate
    CFeeRate required_feerate = GetRequiredFeeRate(wallet);
    if (required_feerate > feerate_needed) {
        feerate_needed = required_feerate;
        if (feeCalc) feeCalc->reason = FeeReason::REQUIRED;
    }
    return feerate_needed;
}

CFeeRate GetDiscardRate(const CWallet& wallet, const CBlockPolicyEstimator& estimator)
{
    unsigned int highest_target = estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    CFeeRate discard_rate = estimator.estimateSmartFee(highest_target, nullptr /* FeeCalculation */, false /* conservative */);
    // Don't let discard_rate be greater than longest possible fee estimate if we get a valid fee estimate
    discard_rate = (discard_rate == CFeeRate(0)) ? wallet.m_discard_rate : std::min(discard_rate, wallet.m_discard_rate);
    // Discard rate must be at least dustRelayFee
    discard_rate = std::max(discard_rate, ::dustRelayFee);
    return discard_rate;
}
