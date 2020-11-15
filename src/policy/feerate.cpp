// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>

#include <tinyformat.h>

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nBytes_)
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0)
        nSatoshisPerK = nFeePaid * 1000 / nSize;
    else
        nSatoshisPerK = 0;
}

CAmount CFeeRate::GetFee(size_t nBytes_) const
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    CAmount nFee = nSatoshisPerK * nSize / 1000;

    if (nFee == 0 && nSize != 0) {
        if (nSatoshisPerK > 0)
            nFee = CAmount(1);
        if (nSatoshisPerK < 0)
            nFee = CAmount(-1);
    }

    return nFee;
}

std::string CFeeRate::ToString(FeeEstimateMode mode, bool with_units) const
{
    if (with_units) {
        switch (mode) {
        case FeeEstimateMode::SAT_VB:
            return strprintf("%d.%03d %s/vB", nSatoshisPerK / 1000, nSatoshisPerK % 1000, CURRENCY_ATOM);
        default:
            return strprintf("%d.%08d %s/kvB", nSatoshisPerK / COIN, nSatoshisPerK % COIN, CURRENCY_UNIT);
        }
    } else {
        switch (mode) {
        case FeeEstimateMode::SAT_VB:
            return strprintf("%d.%03d", nSatoshisPerK / 1000, nSatoshisPerK % 1000);
        default:
            return strprintf("%d.%08d", nSatoshisPerK / COIN, nSatoshisPerK % COIN);
        }
    }
}
