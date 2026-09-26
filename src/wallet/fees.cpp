// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/fees.h>

#include <policy/feerate.h>
#include <util/fees.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

#include <optional>

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
    if (coin_control.m_feerate) { // 1.
        CFeeRate fee_rate{*coin_control.m_feerate};
        // Allow to override automatic min/max check over coin control instance
        if (coin_control.fOverrideFeeRate) return {fee_rate, FeeReason::USER_SPECIFIED, std::nullopt};

        CFeeRate required_feerate = GetRequiredFeeRate(wallet);
        if (required_feerate > fee_rate) return {required_feerate, FeeReason::REQUIRED, std::nullopt};

        return {fee_rate, FeeReason::USER_SPECIFIED, std::nullopt};
    }

    // We will use smart fee estimation
    unsigned int target = coin_control.m_confirm_target ? *coin_control.m_confirm_target : wallet.m_confirm_target;
    // By default estimates are economical iff we are signaling opt-in-RBF
    bool conservative_estimate = !coin_control.m_signal_bip125_rbf.value_or(wallet.m_signal_rbf);
    // Allow to override the default fee estimate mode over the CoinControl instance
    if (coin_control.m_fee_mode == FeeEstimateMode::CONSERVATIVE)
        conservative_estimate = true;
    else if (coin_control.m_fee_mode == FeeEstimateMode::ECONOMICAL)
        conservative_estimate = false;

    const auto fee_estimation_res = wallet.chain().getFeeRateEstimate(target, conservative_estimate);
    const FeeRateEstimation& estimation{FeeRateEstimationRef(fee_estimation_res)};
    CFeeRate fee_rate{estimation.feerate};
    FeeReason fee_reason{FeeReason::FEE_RATE_ESTIMATOR};
    // Only fee rate estimator results have a returned target.
    std::optional<int> returned_target{estimation.returned_target};
    if (fee_rate == CFeeRate(0)) {
        // if we don't have enough data for getFeeRateEstimate, then use fallback fee
        fee_rate = wallet.m_fallback_fee;
        fee_reason = FeeReason::FALLBACK;
        returned_target = std::nullopt;
        // directly return if fallback fee is disabled (feerate 0 == disabled)
        if (wallet.m_fallback_fee == CFeeRate(0)) return {fee_rate, FeeReason::FALLBACK, std::nullopt};
    }

    // Obey mempool min fee when using smart fee estimation or fallback fee
    CFeeRate min_mempool_feerate = wallet.chain().mempoolMinFee();
    if (fee_rate < min_mempool_feerate) {
        fee_rate = min_mempool_feerate;
        fee_reason = FeeReason::MEMPOOL_MIN;
        returned_target = std::nullopt;
    }

    CFeeRate required_feerate = GetRequiredFeeRate(wallet);
    if (required_feerate > fee_rate) return {required_feerate, FeeReason::REQUIRED, std::nullopt};

    return {fee_rate, fee_reason, returned_target};
}

CFeeRate GetDiscardRate(const CWallet& wallet)
{
    unsigned int highest_target = wallet.chain().maximumFeeEstimationTargetBlocks();
    const auto res = wallet.chain().getFeeRateEstimate(highest_target, /*conservative=*/false);
    auto discard_rate = res ? CFeeRate(res->feerate) : CFeeRate(0);
    // Don't let discard_rate be greater than longest possible fee estimate if we get a valid fee estimate
    discard_rate = (discard_rate == CFeeRate(0)) ? wallet.m_discard_rate : std::min(discard_rate, wallet.m_discard_rate);
    // Discard rate must be at least dust relay feerate
    discard_rate = std::max(discard_rate, wallet.chain().relayDustFee());
    return discard_rate;
}
} // namespace wallet
