// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "feerate.h"

#include "tinyformat.h"

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nWeightUnits_)
{
    assert(nWeightUnits_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nWeightUnits_);

    if (nSize > 0)
        nSatoshisPerWU = nFeePaid * nSize;
    else
        nSatoshisPerWU = 0;
}

CAmount CFeeRate::GetFee(size_t nWeightUnits_) const
{
    assert(nWeightUnits_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nWeightUnits_);

    CAmount nFee = nSatoshisPerWU * nSize;

    if (nFee == 0 && nSize != 0) {
        if (nSatoshisPerWU > 0)
            nFee = CAmount(1);
        if (nSatoshisPerWU < 0)
            nFee = CAmount(-1);
    }

    return nFee;
}

std::string CFeeRate::ToString() const
{
    return strprintf("%d.%08d Sat/WU", nSatoshisPerWU, nSatoshisPerWU);
}
