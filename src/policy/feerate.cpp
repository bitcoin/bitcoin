// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <tinyformat.h>


CFeeRate::CFeeRate(const CAmount& nFeePaid, int32_t num_bytes)
{
    if (num_bytes > 0) {
        m_feerate = FeePerVSize(nFeePaid, num_bytes);
    } else {
        m_feerate = FeePerVSize();
    }
}

CAmount CFeeRate::GetFee(int32_t num_bytes) const
{
    if (m_feerate.IsEmpty()) { return CAmount(0);}
    if (num_bytes < 0) { return CAmount(-1);}
    CAmount nFee = CAmount(m_feerate.EvaluateFeeUp(num_bytes));
    if (nFee == 0 && num_bytes != 0 && m_feerate.fee < 0) return CAmount(-1);
    return nFee;
}

std::string CFeeRate::ToString(const FeeEstimateMode& fee_estimate_mode) const
{
    const CAmount rate_per_kvb = GetFeePerK();
    switch (fee_estimate_mode) {
    case FeeEstimateMode::SAT_VB: return strprintf("%d.%03d %s/vB", rate_per_kvb / 1000, rate_per_kvb % 1000, CURRENCY_ATOM);
    default:                      return strprintf("%d.%08d %s/kvB", rate_per_kvb / COIN, rate_per_kvb % COIN, CURRENCY_UNIT);
    }
}
