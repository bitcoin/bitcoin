// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"

#include "tinyformat.h"

const std::string CURRENCY_UNIT = "BTC";

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nSize)
{
    if (nSize > 0)
        nSatoshisPerK = nFeePaid * PRECISION_MULTIPLIER / nSize;
    else
        nSatoshisPerK = 0;
}

CAmount CFeeRate::GetFee(size_t nSize, size_t nPerByteDivisor) const
{
    size_t extra_divisor = 1;
    size_t extra_multiplier = 1;

    // Example for thought: PRECISION_MULTIPLIER == KB, but nPerByteDivisor=MB
    // We divide less as requested by the caller
    if (nPerByteDivisor > PRECISION_MULTIPLIER) {
        assert(false); // TODO Unittest required for this execution branch to be executed
        assert(nPerByteDivisor % PRECISION_MULTIPLIER == 0); // nPerByteDivisor must be multiple of PRECISION_MULTIPLIER
        extra_multiplier = nPerByteDivisor / PRECISION_MULTIPLIER;
    }
    // Example for thought: PRECISION_MULTIPLIER == MB, but nPerByteDivisor=KB
    // In this case we truncate more of what wasn't truncated previously
    else if (nPerByteDivisor < PRECISION_MULTIPLIER) {
        assert(PRECISION_MULTIPLIER % nPerByteDivisor == 0); // PRECISION_MULTIPLIER must be multiple of nPerByteDivisor
        extra_divisor = PRECISION_MULTIPLIER / nPerByteDivisor;
    }

    CAmount nFee = nSatoshisPerK * nSize / (PRECISION_MULTIPLIER * extra_multiplier / extra_divisor);

    // Removing this special case and handling zeroes uniformingly
    // from the caller would allow some simplifications in this function
    if (nFee == 0 && nSize != 0) {
        const size_t nSatoshisPerByteDivisor = nSatoshisPerK * extra_multiplier / extra_divisor;
        if (nSatoshisPerByteDivisor > 0)
            nFee = CAmount(1);
    }
    // if (nFee == 0 && nSize != 0 && nSatoshisPerK > 0)
    //     nFee = CAmount(1);

    return nFee;
}

std::string CFeeRate::ToString() const
{
    CAmount nFee = GetFee(KB, KB);
    return strprintf("%d.%08d %s/kB", nFee / COIN, nFee % COIN, CURRENCY_UNIT);
}
