// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"

#include <assert.h>

// calculate number of bytes for the bitmask, and its number of non-zero bytes
// each bit in the bitmask represents the availability of one output, but the
// availabilities of the first two outputs are encoded separately
void Credits_CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
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

bool Credits_CCoins::Spend(const COutPoint &out, Credits_CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    undo = Credits_CTxInUndo(vout[out.n]);
    vout[out.n].SetNull();
    Cleanup();
    if (vout.size() == 0) {
    	undo.nMetaData = nMetaData;
        undo.nHeight = nHeight;
        undo.fCoinBase = fCoinBase;
        undo.nVersion = this->nVersion;
    }
    return true;
}

bool Credits_CCoins::Spend(int nPos) {
	Credits_CTxInUndo undo;
    COutPoint out(0, nPos);
    return Spend(out, undo);
}


bool Credits_CCoinsView::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) { return false; }
bool Credits_CCoinsView::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) { return false; }
bool Credits_CCoinsView::Credits_HaveCoins(const uint256 &txid) { return false; }
uint256 Credits_CCoinsView::Credits_GetBestBlock() { return uint256(0); }
bool Credits_CCoinsView::Credits_SetBestBlock(const uint256 &hashBlock) { return false; }
bool Credits_CCoinsView::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock) { return false; }
bool Credits_CCoinsView::Credits_GetStats(Credits_CCoinsStats &stats) { return false; }

Credits_CCoinsViewBacked::Credits_CCoinsViewBacked(Credits_CCoinsView &viewIn) : base(&viewIn) { }
bool Credits_CCoinsViewBacked::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) { return base->Credits_GetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) { return base->Credits_SetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Credits_HaveCoins(const uint256 &txid) { return base->Credits_HaveCoins(txid); }
uint256 Credits_CCoinsViewBacked::Credits_GetBestBlock() { return base->Credits_GetBestBlock(); }
bool Credits_CCoinsViewBacked::Credits_SetBestBlock(const uint256 &hashBlock) { return base->Credits_SetBestBlock(hashBlock); }
void Credits_CCoinsViewBacked::Credits_SetBackend(Credits_CCoinsView &viewIn) { base = &viewIn; }
bool Credits_CCoinsViewBacked::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock) { return base->Credits_BatchWrite(mapCoins, hashBlock); }
bool Credits_CCoinsViewBacked::Credits_GetStats(Credits_CCoinsStats &stats) { return base->Credits_GetStats(stats); }

Credits_CCoinsViewCache::Credits_CCoinsViewCache(Credits_CCoinsView &baseIn, bool fDummy) : Credits_CCoinsViewBacked(baseIn), credits_hashBlock(0) { }

bool Credits_CCoinsViewCache::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) {
    if (credits_cacheCoins.count(txid)) {
        coins = credits_cacheCoins[txid];
        return true;
    }
    if (base->Credits_GetCoins(txid, coins)) {
    	credits_cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

std::map<uint256,Credits_CCoins>::iterator Credits_CCoinsViewCache::Credits_FetchCoins(const uint256 &txid) {
    std::map<uint256,Credits_CCoins>::iterator it = credits_cacheCoins.lower_bound(txid);
    if (it != credits_cacheCoins.end() && it->first == txid)
        return it;
    Credits_CCoins tmp;
    if (!base->Credits_GetCoins(txid,tmp))
        return credits_cacheCoins.end();
    std::map<uint256,Credits_CCoins>::iterator ret = credits_cacheCoins.insert(it, std::make_pair(txid, Credits_CCoins()));
    tmp.swap(ret->second);
    return ret;
}

Credits_CCoins &Credits_CCoinsViewCache::Credits_GetCoins(const uint256 &txid) {
    std::map<uint256,Credits_CCoins>::iterator it = Credits_FetchCoins(txid);
    assert_with_stacktrace(it != credits_cacheCoins.end(), strprintf("Credits_CCoinsViewCache::GetCoins() tx could not be found, txid: %s", txid.GetHex()));
    return it->second;
}


bool Credits_CCoinsViewCache::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) {
	credits_cacheCoins[txid] = coins;
    return true;
}

bool Credits_CCoinsViewCache::Credits_HaveCoins(const uint256 &txid) {
    return Credits_FetchCoins(txid) != credits_cacheCoins.end();
}

uint256 Credits_CCoinsViewCache::Credits_GetBestBlock() {
    if (credits_hashBlock == uint256(0))
    	credits_hashBlock = base->Credits_GetBestBlock();
    return credits_hashBlock;
}

bool Credits_CCoinsViewCache::Credits_SetBestBlock(const uint256 &hashBlockIn) {
	credits_hashBlock = hashBlockIn;
    return true;
}

bool Credits_CCoinsViewCache::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlockIn) {
    for (std::map<uint256, Credits_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	credits_cacheCoins[it->first] = it->second;
    credits_hashBlock = hashBlockIn;
    return true;
}

bool Credits_CCoinsViewCache::Credits_Flush() {
    bool fOk = base->Credits_BatchWrite(credits_cacheCoins, credits_hashBlock);
    if (fOk)
    	credits_cacheCoins.clear();
    return fOk;
}

unsigned int Credits_CCoinsViewCache::GetCacheSize() {
    return credits_cacheCoins.size();
}


const CTxOut &Credits_CCoinsViewCache::Credits_GetOutputFor(const Credits_CTxIn& input)
{
    const Credits_CCoins &coins = Credits_GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t Credits_CCoinsViewCache::Credits_GetValueIn(const Credits_CTransaction& tx)
{
	assert(!tx.IsClaim());

    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		nResult += Credits_GetOutputFor(tx.vin[i]).nValue;
	}

    return nResult;
}

bool Credits_CCoinsViewCache::Credits_HaveInputs(const Credits_CTransaction& tx)
{
	assert(!tx.IsClaim());

    if (!tx.IsCoinBase()) {
        // first check whether information about the prevout hash is available
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			const Credits_CTxIn &ctxIn = tx.vin[i];
			const COutPoint &prevout = ctxIn.prevout;
			if (!Credits_HaveCoins(prevout.hash))
				return false;
		}

        // then check whether the actual outputs are available
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			const Credits_CTxIn &ctxIn = tx.vin[i];
			const COutPoint &prevout = ctxIn.prevout;
			const Credits_CCoins &coins = Credits_GetCoins(prevout.hash);
			if (!coins.IsAvailable(prevout.n))
				return false;
		}
    }
    return true;
}

double Credits_CCoinsViewCache::Credits_GetPriority(const Credits_CTransaction &tx, int nHeight)
{
	assert(!tx.IsClaim());

    if (tx.IsCoinBase())
        return 0.0;

    double dResult = 0.0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &ctxIn = tx.vin[i];
		const Credits_CCoins &coins = Credits_GetCoins(ctxIn.prevout.hash);
		if (!coins.IsAvailable(ctxIn.prevout.n)) continue;
		if (coins.nHeight < nHeight) {
			dResult += coins.vout[ctxIn.prevout.n].nValue * (nHeight-coins.nHeight);
		}
	}
    return tx.ComputePriority(dResult);
}
