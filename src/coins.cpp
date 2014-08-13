// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"

#include "random.h"

#include <assert.h>

// calculate number of bytes for the bitmask, and its number of non-zero bytes
// each bit in the bitmask represents the availability of one output, but the
// availabilities of the first two outputs are encoded separately
void CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
    unsigned int nLastUsedByte = 0;
    for (unsigned int b = 0; 2+b*8 < vout.size(); b++) {
        bool fZero = true;
        for (unsigned int i = 0; i < 8 && 2+b*8+i < vout.size(); i++) {
            if (!vout[2+b*8+i].IsNull()) {
                fZero = false;
                continue;
            }
        }
        if (!fZero) {
            nLastUsedByte = b + 1;
            nNonzeroBytes++;
        }
    }
    nBytes += nLastUsedByte;
}

bool CCoins::Spend(const COutPoint &out, CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    undo = CTxInUndo(vout[out.n]);
    vout[out.n].SetNull();
    Cleanup();
    if (vout.size() == 0) {
        undo.nHeight = nHeight;
        undo.fCoinBase = fCoinBase;
        undo.nVersion = this->nVersion;
    }
    return true;
}

bool CCoins::Spend(int nPos) {
    CTxInUndo undo;
    COutPoint out(0, nPos);
    return Spend(out, undo);
}


bool CCoinsView::GetCoins(const uint256 &txid, CCoins &coins) { return false; }
bool CCoinsView::SetCoins(const uint256 &txid, const CCoins &coins) { return false; }
bool CCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 CCoinsView::GetBestBlock() { return uint256(0); }
bool CCoinsView::SetBestBlock(const uint256 &hashBlock) { return false; }
bool CCoinsView::BatchWrite(const CCoinsMap &mapCoins, const uint256 &hashBlock) { return false; }
bool CCoinsView::GetStats(CCoinsStats &stats) { return false; }


CCoinsViewBacked::CCoinsViewBacked(CCoinsView &viewIn) : base(&viewIn) { }
bool CCoinsViewBacked::GetCoins(const uint256 &txid, CCoins &coins) { return base->GetCoins(txid, coins); }
bool CCoinsViewBacked::SetCoins(const uint256 &txid, const CCoins &coins) { return base->SetCoins(txid, coins); }
bool CCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 CCoinsViewBacked::GetBestBlock() { return base->GetBestBlock(); }
bool CCoinsViewBacked::SetBestBlock(const uint256 &hashBlock) { return base->SetBestBlock(hashBlock); }
void CCoinsViewBacked::SetBackend(CCoinsView &viewIn) { base = &viewIn; }
bool CCoinsViewBacked::BatchWrite(const CCoinsMap &mapCoins, const uint256 &hashBlock) { return base->BatchWrite(mapCoins, hashBlock); }
bool CCoinsViewBacked::GetStats(CCoinsStats &stats) { return base->GetStats(stats); }

CCoinsKeyHasher::CCoinsKeyHasher() : salt(GetRandHash()) {}

CCoinsViewCache::CCoinsViewCache(CCoinsView &baseIn, bool fDummy) : CCoinsViewBacked(baseIn), hashBlock(0) { }

const CCoins *CCoinsViewCache::FetchCoins(const uint256 &txid) {
    CCoinsMap::const_iterator it;
    // First look up the coins in the write cache
    it = cacheWrite.find(txid);
    if (it != cacheWrite.end())
    {
        stats.positive_hits++;
        return &it->second;
    }

    // Otherwise look up the coins in the read cache
    it = cacheRead.find(txid);
    if (it != cacheRead.end())
    {
        if(it->second.vout.empty()) // Match negative
        {
            stats.negative_hits++;
            return 0;
        }

        stats.positive_hits++;
        return &it->second;
    }

    // If everything missed, fall back to base
    CCoins tmp;
    if (!base->GetCoins(txid, tmp))
    {
        stats.negative_misses++;
        cacheRead.insert(it, std::make_pair(txid, CCoins())); // Store negative match
        return 0;
    }
    stats.positive_misses++;
    CCoinsMap::iterator itnew = cacheRead.insert(it, std::make_pair(txid, CCoins()));
    tmp.swap(itnew->second);
    return &itnew->second;
}

bool CCoinsViewCache::GetCoins(const uint256 &txid, CCoins &coins) {
    const CCoins *pcoins = FetchCoins(txid);
    if (pcoins) {
        coins = *pcoins;
        return true;
    }
    return false;
}

const CCoins &CCoinsViewCache::GetCoins(const uint256 &txid) {
    const CCoins *pcoins = FetchCoins(txid);
    assert(pcoins);
    return *pcoins;
}

CCoins &CCoinsViewCache::ModifyCoins(const uint256 &txid) {
    // If already in the write cache, return a direct reference
    CCoinsMap::iterator itw = cacheWrite.find(txid);
    if (itw != cacheWrite.end())
    {
        stats.positive_hits++;
        return itw->second;
    }

    // If in the read cache, swap the entry to the write cache,
    // evict it from the read cache
    CCoinsMap::iterator itr = cacheRead.find(txid);
    if (itr != cacheRead.end())
    {
        assert(!itr->second.vout.empty()); // No negative hits allowed here
        stats.positive_hits++;
        itw = cacheWrite.insert(itw, std::make_pair(txid, CCoins()));
        itw->second.swap(itr->second);
        cacheRead.erase(itr);
        return itw->second;
    }

    // If everything missed, fall back to base. Load the entry
    // directly into the write cache. Croak if the coins do not
    // exist.
    stats.positive_misses++;
    CCoins tmp;
    bool have = base->GetCoins(txid, tmp);
    assert(have);
    itw = cacheWrite.insert(itw, std::make_pair(txid, CCoins()));
    tmp.swap(itw->second);
    return itw->second;
}

bool CCoinsViewCache::SetCoins(const uint256 &txid, const CCoins &coins) {
    // Evict from read cache (if present there), then write to write cache
    // (overwriting anything already there)
    cacheRead.erase(txid);
    cacheWrite[txid] = coins;
    return true;
}

bool CCoinsViewCache::HaveCoins(const uint256 &txid) {
    const CCoins *pcoins = FetchCoins(txid);
    // We're using vtx.empty() instead of IsPruned here for performance reasons,
    // as we only care about the case where an transaction was replaced entirely
    // in a reorganization (which wipes vout entirely, as opposed to spending
    // which just cleans individual outputs).
    return (pcoins && !pcoins->vout.empty());
}

uint256 CCoinsViewCache::GetBestBlock() {
    if (hashBlock == uint256(0))
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

bool CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::BatchWrite(const CCoinsMap &mapCoins, const uint256 &hashBlockIn) {
    LogPrint("coinscache", "%p: BatchWrite before %i in read cache, %i in write cache\n", this, cacheRead.size(), cacheWrite.size());
    for (CCoinsMap::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    {
        cacheRead.erase(it->first);
        cacheWrite[it->first] = it->second;
    }
    hashBlock = hashBlockIn;
    LogPrint("coinscache", "%p: BatchWrite after %i in read cache, %i in write cache\n", this, cacheRead.size(), cacheWrite.size());
    return true;
}

bool CCoinsViewCache::Flush(unsigned int flags) {
    LogPrint("coinscache", "%p: Flush before (%i) %i in read cache, %i in write cache\n", this, flags, cacheRead.size(), cacheWrite.size());
    bool fOk = true;
    if (flags & WRITE)
    {
        stats.writes += cacheWrite.size();
        fOk = base->BatchWrite(cacheWrite, hashBlock);
        // Flush write cache only if batch write succeeded, but always flush read cache
        if (fOk)
        {
            // Move entries from write cache to read cache
            // Possible optimization is to not do this if we're flushing the
            // READ cache as well, but this functionality is not used at the moment
            // so we don't optimize for it.
            for (CCoinsMap::iterator it = cacheWrite.begin(); it != cacheWrite.end(); )
            {
                CCoinsMap::iterator itOld = it++;
                std::pair<CCoinsMap::iterator,bool> itr = cacheRead.insert(std::make_pair(itOld->first, CCoins()));
                itr.first->second.swap(itOld->second);
                cacheWrite.erase(itOld);
            }
        }
    }
    if (flags & READ)
    {
        cacheRead.clear();
    }
    LogPrint("coinscache", "%p: Flush after %i in read cache, %i in write cache\n", this, cacheRead.size(), cacheWrite.size());
    return fOk;
}

unsigned int CCoinsViewCache::GetCacheSize(unsigned int flags) {
    unsigned int total = 0;
    if (flags & WRITE)
        total += cacheWrite.size();
    if (flags & READ)
        total += cacheRead.size();
    return total;
}

const CTxOut &CCoinsViewCache::GetOutputFor(const CTxIn& input)
{
    const CCoins &coins = GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t CCoinsViewCache::GetValueIn(const CTransaction& tx)
{
    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool CCoinsViewCache::HaveInputs(const CTransaction& tx)
{
    if (!tx.IsCoinBase()) {
        // first check whether information about the prevout hash is available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            if (!HaveCoins(prevout.hash))
                return false;
        }

        // then check whether the actual outputs are available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins &coins = GetCoins(prevout.hash);
            if (!coins.IsAvailable(prevout.n))
                return false;
        }
    }
    return true;
}

double CCoinsViewCache::GetPriority(const CTransaction &tx, int nHeight)
{
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        const CCoins &coins = GetCoins(txin.prevout.hash);
        if (!coins.IsAvailable(txin.prevout.n)) continue;
        if (coins.nHeight < nHeight) {
            dResult += coins.vout[txin.prevout.n].nValue * (nHeight-coins.nHeight);
        }
    }
    return tx.ComputePriority(dResult);
}
