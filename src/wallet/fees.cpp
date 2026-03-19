// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/fees.h>

#include <wallet/coincontrol.h>
#include <wallet/wallet.h>


namespace wallet {
CAmount GetRequiredFee(const CWallet& wallet, unsigned int nTxBytes)
{
    return GetRequiredFeeRate(wallet).GetFee(static_cast<int32_t>(nTxBytes));
}


CAmount GetMinimumFee(const MinimumFeeRateResult& min_fee_rate, unsigned int nTxBytes)
{
    return min_fee_rate.fee_rate.GetFee(static_cast<int32_t>(nTxBytes));
}

CFeeRate GetRequiredFeeRate(const CWallet& wallet)
{
    return std::max(wallet.m_min_fee, wallet.chain().relayMinFee());
}

MinimumFeeRateResult GetMinimumFeeRate(const CWallet& wallet, const CCoinControl& coin_control)
{
    /* User control of how to calculate fee uses the following parameter precedence:
       1. coin_control.m_feerate
       2. coin_control.m_confirm_target
       3. m_confirm_target (user-set member variable of wallet)
       The first parameter that is set is used.
    */
    MinimumFeeRateResult res;
    if (coin_control.m_feerate) { // 1.
        res.fee_rate = *(coin_control.m_feerate);
        // Allow to override automatic min/max check over coin control instance
        if (coin_control.fOverrideFeeRate) return res;
    }
    else { // 2. or 3.
        // We will use smart fee estimation
        unsigned int target = coin_control.m_confirm_target ? *coin_control.m_confirm_target : wallet.m_confirm_target;
        // By default estimates are economical iff we are signaling opt-in-RBF
        bool conservative_estimate = !coin_control.m_signal_bip125_rbf.value_or(wallet.m_signal_rbf);
        // Allow to override the default fee estimate mode over the CoinControl instance
        if (coin_control.m_fee_mode == FeeEstimateMode::CONSERVATIVE) conservative_estimate = true;
        else if (coin_control.m_fee_mode == FeeEstimateMode::ECONOMICAL) conservative_estimate = false;

        FeeCalculation feeCalc;
        res.fee_rate = wallet.chain().estimateSmartFee(target, conservative_estimate, &feeCalc);
        if (res.fee_rate == CFeeRate(0)) {
            // if we don't have enough data for estimateSmartFee, then use fallback fee
            res.fee_rate = wallet.m_fallback_fee;
            res.fee_reason = FeeReason::FALLBACK;

            // directly return if fallback fee is disabled (feerate 0 == disabled)
            if (wallet.m_fallback_fee == CFeeRate(0)) return res;
        } else {
            res.returned_target = feeCalc.returnedTarget;
            res.fee_reason = feeCalc.reason;
        }

        // Obey mempool min fee when using smart fee estimation
        CFeeRate min_mempool_feerate = wallet.chain().mempoolMinFee();
        if (res.fee_rate < min_mempool_feerate) {
            res.fee_rate = min_mempool_feerate;
            res.fee_reason = FeeReason::MEMPOOL_MIN;
        }
    }

    // prevent user from paying a fee below the required fee rate
    CFeeRate required_feerate = GetRequiredFeeRate(wallet);
    if (required_feerate > res.fee_rate) {
        res.fee_rate = required_feerate;
        res.fee_reason = FeeReason::REQUIRED;
    }
    return res;
}

CFeeRate GetDiscardRate(const CWallet& wallet)
{
    unsigned int highest_target = wallet.chain().estimateMaxBlocks();
    CFeeRate discard_rate = wallet.chain().estimateSmartFee(highest_target, /*conservative=*/false);
    // Don't let discard_rate be greater than longest possible fee estimate if we get a valid fee estimate
    discard_rate = (discard_rate == CFeeRate(0)) ? wallet.m_discard_rate : std::min(discard_rate, wallet.m_discard_rate);
    // Discard rate must be at least dust relay feerate
    discard_rate = std::max(discard_rate, wallet.chain().relayDustFee());
    return discard_rate;
}
} // namespace wallet
