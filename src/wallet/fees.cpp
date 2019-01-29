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


CAmount GetMinimumFee(unsigned int nTxBytes, const CCoinControl& coin_control, const CTxMemPool& pool)
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
        // Allow to override automatic min/max check over coin control instance
        if (coin_control.fOverrideFeeRate) return fee_needed;
    }
    else if (!coin_control.m_confirm_target && ::payTxFee != CFeeRate(0)) { // 3. TODO: remove magic value of 0 for global payTxFee
        fee_needed = ::payTxFee.GetFee(nTxBytes);
    }
    else { // 2. or 4.
        // if we don't have enough data for estimateSmartFee, then use fallbackFee
        fee_needed = CWallet::fallbackFee.GetFee(nTxBytes);
        // Obey mempool min fee when using smart fee estimation
        CAmount min_mempool_fee = pool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFee(nTxBytes);
        if (fee_needed < min_mempool_fee) {
            fee_needed = min_mempool_fee;
        }
    }

    // prevent user from paying a fee below minRelayTxFee or minTxFee
    CAmount required_fee = GetRequiredFee(nTxBytes);
    if (required_fee > fee_needed) {
        fee_needed = required_fee;
    }
    // But always obey the maximum
    if (fee_needed > maxTxFee) {
        fee_needed = maxTxFee;
    }
    return fee_needed;
}


CFeeRate GetDiscardRate()
{
    CFeeRate discard_rate = CWallet::m_discard_rate;
    // Discard rate must be at least dustRelayFee
    discard_rate = std::max(discard_rate, ::dustRelayFee);
    return discard_rate;
}
