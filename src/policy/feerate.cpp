// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <tinyformat.h>


CFeeRate::CFeeRate(const CAmount& nFeePaid, int32_t virtual_bytes)
{
    if (virtual_bytes > 0) {
        m_feerate = FeePerVSize(nFeePaid, virtual_bytes);
    } else {
        m_feerate = FeePerVSize();
    }
}

CAmount CFeeRate::GetFee(int32_t virtual_bytes) const
{
    Assume(virtual_bytes >= 0);
    if (m_feerate.IsEmpty()) { return CAmount(0);}
    CAmount nFee = CAmount(m_feerate.EvaluateFeeUp(virtual_bytes));
    if (nFee == 0 && virtual_bytes != 0 && m_feerate.fee < 0) return CAmount(-1);
    return nFee;
}

std::string CFeeRate::ToString(const FeeEstimateMode& fee_estimate_mode) const
{
    const CAmount feerate_per_kvb = GetFeePerK();
    switch (fee_estimate_mode) {
    case FeeEstimateMode::SAT_VB: return strprintf("%d.%03d %s/vB", feerate_per_kvb / 1000, feerate_per_kvb % 1000, CURRENCY_ATOM);
    default:                      return strprintf("%d.%08d %s/kvB", feerate_per_kvb / COIN, feerate_per_kvb % COIN, CURRENCY_UNIT);
    }
}
