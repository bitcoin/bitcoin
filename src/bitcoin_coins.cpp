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


bool Bitcoin_CCoinsView::Bitcoin_GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::Bitcoin_SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::Bitcoin_HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcoin_CCoinsView::Bitcoin_GetBestBlock() { return uint256(0); }
bool Bitcoin_CCoinsView::Bitcoin_SetBestBlock(const uint256 &hashBlock) { return false; }
bool Bitcoin_CCoinsView::Bitcoin_BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock) { return false; }
bool Bitcoin_CCoinsView::Bitcoin_GetStats(Bitcoin_CCoinsStats &stats) { return false; }


Bitcoin_CCoinsViewBacked::Bitcoin_CCoinsViewBacked(Bitcoin_CCoinsView &viewIn) : base(&viewIn) { }
bool Bitcoin_CCoinsViewBacked::Bitcoin_GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return base->Bitcoin_GetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::Bitcoin_SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return base->Bitcoin_SetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::Bitcoin_HaveCoins(const uint256 &txid) { return base->Bitcoin_HaveCoins(txid); }
uint256 Bitcoin_CCoinsViewBacked::Bitcoin_GetBestBlock() { return base->Bitcoin_GetBestBlock(); }
bool Bitcoin_CCoinsViewBacked::Bitcoin_SetBestBlock(const uint256 &hashBlock) { return base->Bitcoin_SetBestBlock(hashBlock); }
void Bitcoin_CCoinsViewBacked::Bitcoin_SetBackend(Bitcoin_CCoinsView &viewIn) { base = &viewIn; }
Bitcoin_CCoinsView *Bitcoin_CCoinsViewBacked::Bitcoin_GetBackend() { return base;}
bool Bitcoin_CCoinsViewBacked::Bitcoin_BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock) { return base->Bitcoin_BatchWrite(mapCoins, hashBlock); }
bool Bitcoin_CCoinsViewBacked::Bitcoin_GetStats(Bitcoin_CCoinsStats &stats) { return base->Bitcoin_GetStats(stats); }

Bitcoin_CCoinsViewCache::Bitcoin_CCoinsViewCache(Bitcoin_CCoinsView &baseIn, bool fDummy) : Bitcoin_CCoinsViewBacked(baseIn), bitcoin_hashBlock(0) { }

bool Bitcoin_CCoinsViewCache::Bitcoin_GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) {
    if (bitcoin_cacheCoins.count(txid)) {
        coins = bitcoin_cacheCoins[txid];
        return true;
    }
    if (base->Bitcoin_GetCoins(txid, coins)) {
    	bitcoin_cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

std::map<uint256,Bitcoin_CCoins>::iterator Bitcoin_CCoinsViewCache::Bitcoin_FetchCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CCoins>::iterator it = bitcoin_cacheCoins.lower_bound(txid);
    if (it != bitcoin_cacheCoins.end() && it->first == txid)
        return it;
    Bitcoin_CCoins tmp;
    if (!base->Bitcoin_GetCoins(txid,tmp))
        return bitcoin_cacheCoins.end();
    std::map<uint256,Bitcoin_CCoins>::iterator ret = bitcoin_cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CCoins()));
    tmp.swap(ret->second);
    return ret;
}

Bitcoin_CCoins &Bitcoin_CCoinsViewCache::Bitcoin_GetCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CCoins>::iterator it = Bitcoin_FetchCoins(txid);
    assert_with_stacktrace(it != bitcoin_cacheCoins.end(), strprintf("Bitcoin_CCoinsViewCache::GetCoins() tx could not be found, txid: %s", txid.GetHex()));
    return it->second;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) {
	bitcoin_cacheCoins[txid] = coins;
    return true;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_HaveCoins(const uint256 &txid) {
    return Bitcoin_FetchCoins(txid) != bitcoin_cacheCoins.end();
}

uint256 Bitcoin_CCoinsViewCache::Bitcoin_GetBestBlock() {
    if (bitcoin_hashBlock == uint256(0))
    	bitcoin_hashBlock = base->Bitcoin_GetBestBlock();
    return bitcoin_hashBlock;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_SetBestBlock(const uint256 &hashBlockIn) {
	bitcoin_hashBlock = hashBlockIn;
    return true;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlockIn) {
    for (std::map<uint256, Bitcoin_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	bitcoin_cacheCoins[it->first] = it->second;
    bitcoin_hashBlock = hashBlockIn;
    return true;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_Flush() {
    bool fOk = base->Bitcoin_BatchWrite(bitcoin_cacheCoins, bitcoin_hashBlock);
    if (fOk)
    	bitcoin_cacheCoins.clear();
    return fOk;
}

unsigned int Bitcoin_CCoinsViewCache::Bitcoin_GetCacheSize() {
    return bitcoin_cacheCoins.size();
}

const CTxOut &Bitcoin_CCoinsViewCache::Bitcoin_GetOutputFor(const Bitcoin_CTxIn& input)
{
    const Bitcoin_CCoins &coins = Bitcoin_GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t Bitcoin_CCoinsViewCache::Bitcoin_GetValueIn(const Bitcoin_CTransaction& tx)
{
    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += Bitcoin_GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_HaveInputs(const Bitcoin_CTransaction& tx)
{
    if (!tx.IsCoinBase()) {
        // first check whether information about the prevout hash is available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            if (!Bitcoin_HaveCoins(prevout.hash))
                return false;
        }

        // then check whether the actual outputs are available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const Bitcoin_CCoins &coins = Bitcoin_GetCoins(prevout.hash);
            if (!coins.IsAvailable(prevout.n))
                return false;
        }
    }
    return true;
}

double Bitcoin_CCoinsViewCache::Bitcoin_GetPriority(const Bitcoin_CTransaction &tx, int nHeight)
{
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        const Bitcoin_CCoins &coins = Bitcoin_GetCoins(txin.prevout.hash);
        if (!coins.IsAvailable(txin.prevout.n)) continue;
        if (coins.nHeight < nHeight) {
            dResult += coins.vout[txin.prevout.n].nValue * (nHeight-coins.nHeight);
        }
    }
    return tx.ComputePriority(dResult);
}
