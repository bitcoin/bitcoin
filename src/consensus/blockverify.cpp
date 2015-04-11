// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/consensus.h"

#include "chain.h"

#include <algorithm>  

static const unsigned int MEDIAN_TIME_SPAN = 11;

int64_t GetMedianTimePast(const CBlockIndex* pindex)
{
    int64_t pmedian[MEDIAN_TIME_SPAN];
    int64_t* pbegin = &pmedian[MEDIAN_TIME_SPAN];
    int64_t* pend = &pmedian[MEDIAN_TIME_SPAN];

    for (unsigned int i = 0; i < MEDIAN_TIME_SPAN && pindex; i++, pindex = pindex->pprev)
        *(--pbegin) = (int64_t)pindex->nTime;

    std::sort(pbegin, pend);
    return pbegin[(pend - pbegin)/2];
}
