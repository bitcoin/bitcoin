// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txcheckcache.h"

#include "consensus/validation.h"
#include "uint256.h"
#include "util.h"


CTxChecksCacheEntry::CTxChecksCacheEntry()
    : nTime(0), chRejectCode(0), strRejectReason(""), fAcceptedOnce(false), nDoS(0)
{
}

CTxChecksCache::CTxChecksCache(unsigned int nMaxCacheSizeIn)
    : nMaxCacheSize(nMaxCacheSizeIn)
{
}

CTxChecksCacheEntry& CTxChecksCache::UpdateEntry(const uint256& hash, int64_t nTime)
{
    if (!mTxChecksCache.count(hash)) {
        mTxChecksCache[hash] = CTxChecksCacheEntry();

        if (mTxChecksCache.size() == 1) {
            nOldestTime = nTime;
        } else if (mTxChecksCache.size() > nMaxCacheSize) {
            // Remove the oldest
            std::map<uint256, CTxChecksCacheEntry>::const_iterator it;
            for (it = mTxChecksCache.begin(); it != mTxChecksCache.end(); ++it)
                if (nOldestTime == it->second.nTime) {
                    mTxChecksCache.erase(it->first);
                    break;
                }
            // Recalculate the oldest
            nOldestTime = mTxChecksCache.find(hash)->second.nTime;
            for (it = mTxChecksCache.begin(); it != mTxChecksCache.end(); ++it)
                if (nOldestTime > it->second.nTime)
                    nOldestTime = it->second.nTime;
        }
    }
    mTxChecksCache.find(hash)->second.nTime = nTime;
    return mTxChecksCache.find(hash)->second;
}

bool CTxChecksCache::RejectTx(const uint256& hash, CValidationState& state, int64_t nTime)
{
    UpdateEntry(hash, nTime);
    mTxChecksCache[hash].chRejectCode = state.GetRejectCode();
    mTxChecksCache[hash].strRejectReason = state.GetRejectReason();
    mTxChecksCache[hash].nDoS = state.GetDoS();
    LogPrintf("%s%s: %s", mTxChecksCache.find(hash)->second.chRejectCode == REJECT_INVALID ? "ERROR: ": "", __func__, state.GetRejectReason());
    return false;
}

bool CTxChecksCache::RejectTx(const uint256& hash, CValidationState& state, int64_t nTime, int nDoS, unsigned char chRejectCode, std::string strRejectReason, bool corruption)
{
    state.DoS(nDoS, false, chRejectCode, strRejectReason, corruption);
    return RejectTx(hash, state, nTime);
}


void CTxChecksCache::AcceptTx(const uint256& hash, int64_t nTime)
{
    UpdateEntry(hash, nTime);
    mTxChecksCache[hash].fAcceptedOnce = true;
}

bool CTxChecksCache::WasAcceptedOnce(const uint256& hash) const
{
    return mTxChecksCache.count(hash) && mTxChecksCache.find(hash)->second.fAcceptedOnce;
}

bool CTxChecksCache::RejectedPermanently(const uint256& hash, CValidationState& state) const
{
    if (mTxChecksCache.count(hash)) {
        const CTxChecksCacheEntry& entry = mTxChecksCache.find(hash)->second;
        LogPrintf("Cached tx: reason=%s, time=%d, acceptedOnce=%d, hash=%s", 
                      entry.strRejectReason, entry.nTime, entry.fAcceptedOnce ? 1 : 0, hash.ToString());

        if (entry.chRejectCode == REJECT_INVALID && entry.strRejectReason == "missing-inputs")
            return !state.DoS(entry.nDoS, false, REJECT_INVALID, entry.strRejectReason);
    }
    return false;
}
