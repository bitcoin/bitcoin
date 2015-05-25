// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_coins.h"

#include <assert.h>

// calculate number of bytes for the bitmask, and its number of non-zero bytes
// each bit in the bitmask represents the availability of one output, but the
// availabilities of the first two outputs are encoded separately
void Bitcoin_CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
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

bool Bitcoin_CCoins::Spend(const COutPoint &out, Bitcoin_CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    undo = Bitcoin_CTxInUndo(vout[out.n]);
    vout[out.n].SetNull();
    Cleanup();
    if (vout.size() == 0) {
        undo.nHeight = nHeight;
        undo.fCoinBase = fCoinBase;
        undo.nVersion = this->nVersion;
    }
    return true;
}

bool Bitcoin_CCoins::Spend(int nPos) {
	Bitcoin_CTxInUndo undo;
    COutPoint out(0, nPos);
    return Spend(out, undo);
}


bool Bitcoin_CCoinsView::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcoin_CCoinsView::GetBestBlock() { return uint256(0); }
bool Bitcoin_CCoinsView::SetBestBlock(const uint256 &hashBlock) { return false; }
bool Bitcoin_CCoinsView::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock) { return false; }
bool Bitcoin_CCoinsView::GetStats(Bitcoin_CCoinsStats &stats) { return false; }


Bitcoin_CCoinsViewBacked::Bitcoin_CCoinsViewBacked(Bitcoin_CCoinsView &viewIn) : base(&viewIn) { }
bool Bitcoin_CCoinsViewBacked::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return base->GetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return base->SetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 Bitcoin_CCoinsViewBacked::GetBestBlock() { return base->GetBestBlock(); }
bool Bitcoin_CCoinsViewBacked::SetBestBlock(const uint256 &hashBlock) { return base->SetBestBlock(hashBlock); }
void Bitcoin_CCoinsViewBacked::SetBackend(Bitcoin_CCoinsView &viewIn) { base = &viewIn; }
Bitcoin_CCoinsView *Bitcoin_CCoinsViewBacked::GetBackend() { return base;}
bool Bitcoin_CCoinsViewBacked::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock) { return base->BatchWrite(mapCoins, hashBlock); }
bool Bitcoin_CCoinsViewBacked::GetStats(Bitcoin_CCoinsStats &stats) { return base->GetStats(stats); }

Bitcoin_CCoinsViewCache::Bitcoin_CCoinsViewCache(Bitcoin_CCoinsView &baseIn, bool fDummy) : Bitcoin_CCoinsViewBacked(baseIn), hashBlock(0) { }

bool Bitcoin_CCoinsViewCache::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) {
    if (cacheCoins.count(txid)) {
        coins = cacheCoins[txid];
        return true;
    }
    if (base->GetCoins(txid, coins)) {
        cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

std::map<uint256,Bitcoin_CCoins>::iterator Bitcoin_CCoinsViewCache::FetchCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CCoins>::iterator it = cacheCoins.lower_bound(txid);
    if (it != cacheCoins.end() && it->first == txid)
        return it;
    Bitcoin_CCoins tmp;
    if (!base->GetCoins(txid,tmp))
        return cacheCoins.end();
    std::map<uint256,Bitcoin_CCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CCoins()));
    tmp.swap(ret->second);
    return ret;
}

Bitcoin_CCoins &Bitcoin_CCoinsViewCache::GetCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CCoins>::iterator it = FetchCoins(txid);
    assert_with_stacktrace(it != cacheCoins.end(), strprintf("Bitcoin_CCoinsViewCache::GetCoins() tx could not be found, txid: %s", txid.GetHex()));
    return it->second;
}

bool Bitcoin_CCoinsViewCache::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) {
    cacheCoins[txid] = coins;
    return true;
}

bool Bitcoin_CCoinsViewCache::HaveCoins(const uint256 &txid) {
    return FetchCoins(txid) != cacheCoins.end();
}

uint256 Bitcoin_CCoinsViewCache::GetBestBlock() {
    if (hashBlock == uint256(0))
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

bool Bitcoin_CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}

bool Bitcoin_CCoinsViewCache::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlockIn) {
    for (std::map<uint256, Bitcoin_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        cacheCoins[it->first] = it->second;
    hashBlock = hashBlockIn;
    return true;
}

bool Bitcoin_CCoinsViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, hashBlock);
    if (fOk)
        cacheCoins.clear();
    return fOk;
}

unsigned int Bitcoin_CCoinsViewCache::GetCacheSize() {
    return cacheCoins.size();
}

const CTxOut &Bitcoin_CCoinsViewCache::GetOutputFor(const Bitcoin_CTxIn& input)
{
    const Bitcoin_CCoins &coins = GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t Bitcoin_CCoinsViewCache::GetValueIn(const Bitcoin_CTransaction& tx)
{
    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool Bitcoin_CCoinsViewCache::HaveInputs(const Bitcoin_CTransaction& tx)
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
            const Bitcoin_CCoins &coins = GetCoins(prevout.hash);
            if (!coins.IsAvailable(prevout.n))
                return false;
        }
    }
    return true;
}

double Bitcoin_CCoinsViewCache::GetPriority(const Bitcoin_CTransaction &tx, int nHeight)
{
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        const Bitcoin_CCoins &coins = GetCoins(txin.prevout.hash);
        if (!coins.IsAvailable(txin.prevout.n)) continue;
        if (coins.nHeight < nHeight) {
            dResult += coins.vout[txin.prevout.n].nValue * (nHeight-coins.nHeight);
        }
    }
    return tx.ComputePriority(dResult);
}
