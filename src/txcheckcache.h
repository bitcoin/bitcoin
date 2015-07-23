// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TX_CHECKS_CACHE_H
#define BITCOIN_TX_CHECKS_CACHE_H

#include "amount.h"

class CValidationState;
class uint256;

struct CTxChecksCacheEntry
{
    int64_t nTime;
    unsigned char chRejectCode;
    std::string strRejectReason;
    bool fAcceptedOnce;
    int nDoS;

    CTxChecksCacheEntry();
};

class CTxChecksCache
{
    std::map<uint256, CTxChecksCacheEntry> mTxChecksCache;
    unsigned int nMaxCacheSize;
    int64_t nOldestTime;
public:
    CTxChecksCache(unsigned int nMaxCacheSizeIn=10000);

    CTxChecksCacheEntry& UpdateEntry(const uint256& hash, int64_t nTime);
    bool RejectTx(const uint256& hash, CValidationState& state, int64_t nTime);
    bool RejectTx(const uint256& hash, CValidationState& state, int64_t nTime, int levelDoS, unsigned char chRejectCodeIn=0, std::string strRejectReasonIn="", bool corruptionIn=false);
    void AcceptTx(const uint256& hash, int64_t nTime);
    bool WasAcceptedOnce(const uint256& hash) const;
    bool RejectedPermanently(const uint256& hash, CValidationState& state) const;    
};

#endif // BITCOIN_TX_CHECKS_CACHE_H
