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


CAmount GetRequiredFee(unsigned int nTxBytes)
{
    return std::max(CWallet::minTxFee.GetFee(nTxBytes), ::minRelayTxFee.GetFee(nTxBytes));
}


CAmount GetMinimumFee(unsigned int nTxBytes, const CCoinControl& coin_control, const CTxMemPool& pool, const CBlockPolicyEstimator& estimator, FeeCalculation *feeCalc)
{
    /* User control of how to calculate fee uses the following parameter precedence:
       1. coin_control.m_feerate
       2. coin_control.m_confirm_target
       3. payTxFee (user-set global variable)
       4. nTxConfirmTarget (user-set global variable)
       The first parameter that is set is used.
    */
    CAmount fee_needed;
    if (coin_control.m_feerate) { // 1.
        fee_needed = coin_control.m_feerate->GetFee(nTxBytes);
        if (feeCalc) feeCalc->reason = FeeReason::PAYTXFEE;
        // Allow to override automatic min/max check over coin control instance
        if (coin_control.fOverrideFeeRate) return fee_needed;
    }
    else if (!coin_control.m_confirm_target && ::payTxFee != CFeeRate(0)) { // 3. TODO: remove magic value of 0 for global payTxFee
        fee_needed = ::payTxFee.GetFee(nTxBytes);
        if (feeCalc) feeCalc->reason = FeeReason::PAYTXFEE;
    }
    else { // 2. or 4.
        // We will use smart fee estimation
        unsigned int target = coin_control.m_confirm_target ? *coin_control.m_confirm_target : ::nTxConfirmTarget;
        // By default estimates are economical iff we are signaling opt-in-RBF
        bool conservative_estimate = !coin_control.signalRbf;
        // Allow to override the default fee estimate mode over the CoinControl instance
        if (coin_control.m_fee_mode == FeeEstimateMode::CONSERVATIVE) conservative_estimate = true;
        else if (coin_control.m_fee_mode == FeeEstimateMode::ECONOMICAL) conservative_estimate = false;

        fee_needed = estimator.estimateSmartFee(target, feeCalc, conservative_estimate).GetFee(nTxBytes);
        if (fee_needed == 0) {
            // if we don't have enough data for estimateSmartFee, then use fallbackFee
            fee_needed = CWallet::fallbackFee.GetFee(nTxBytes);
            if (feeCalc) feeCalc->reason = FeeReason::FALLBACK;
        }
        // Obey mempool min fee when using smart fee estimation
        CAmount min_mempool_fee = pool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFee(nTxBytes);
        if (fee_needed < min_mempool_fee) {
            fee_needed = min_mempool_fee;
            if (feeCalc) feeCalc->reason = FeeReason::MEMPOOL_MIN;
        }
    }

    // prevent user from paying a fee below minRelayTxFee or minTxFee
    CAmount required_fee = GetRequiredFee(nTxBytes);
    if (required_fee > fee_needed) {
        fee_needed = required_fee;
        if (feeCalc) feeCalc->reason = FeeReason::REQUIRED;
    }
    // But always obey the maximum
    if (fee_needed > maxTxFee) {
        fee_needed = maxTxFee;
        if (feeCalc) feeCalc->reason = FeeReason::MAXTXFEE;
    }
    return fee_needed;
}


CFeeRate GetDiscardRate(const CBlockPolicyEstimator& estimator)
{
    unsigned int highest_target = estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    CFeeRate discard_rate = estimator.estimateSmartFee(highest_target, nullptr /* FeeCalculation */, false /* conservative */);
    // Don't let discard_rate be greater than longest possible fee estimate if we get a valid fee estimate
    discard_rate = (discard_rate == CFeeRate(0)) ? CWallet::m_discard_rate : std::min(discard_rate, CWallet::m_discard_rate);
    // Discard rate must be at least dustRelayFee
    discard_rate = std::max(discard_rate, ::dustRelayFee);
    return discard_rate;
}
