// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"

#include <assert.h>

// calculate number of bytes for the bitmask, and its number of non-zero bytes
// each bit in the bitmask represents the availability of one output, but the
// availabilities of the first two outputs are encoded separately
void Bitcredit_CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
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

bool Bitcredit_CCoins::Spend(const COutPoint &out, Credits_CTxInUndo &undo) {
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

bool Bitcredit_CCoins::Spend(int nPos) {
	Credits_CTxInUndo undo;
    COutPoint out(0, nPos);
    return Spend(out, undo);
}


bool Bitcredit_CCoinsView::GetCoins(const uint256 &txid, Bitcredit_CCoins &coins) { return false; }
bool Bitcredit_CCoinsView::SetCoins(const uint256 &txid, const Bitcredit_CCoins &coins) { return false; }
bool Bitcredit_CCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcredit_CCoinsView::GetBestBlock() { return uint256(0); }
bool Bitcredit_CCoinsView::SetBestBlock(const uint256 &hashBlock) { return false; }
bool Bitcredit_CCoinsView::BatchWrite(const std::map<uint256, Bitcredit_CCoins> &mapCoins, const uint256 &hashBlock) { return false; }
bool Bitcredit_CCoinsView::GetStats(Bitcredit_CCoinsStats &stats) { return false; }


Bitcredit_CCoinsViewBacked::Bitcredit_CCoinsViewBacked(Bitcredit_CCoinsView &viewIn) : base(&viewIn) { }
bool Bitcredit_CCoinsViewBacked::GetCoins(const uint256 &txid, Bitcredit_CCoins &coins) { return base->GetCoins(txid, coins); }
bool Bitcredit_CCoinsViewBacked::SetCoins(const uint256 &txid, const Bitcredit_CCoins &coins) { return base->SetCoins(txid, coins); }
bool Bitcredit_CCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 Bitcredit_CCoinsViewBacked::GetBestBlock() { return base->GetBestBlock(); }
bool Bitcredit_CCoinsViewBacked::SetBestBlock(const uint256 &hashBlock) { return base->SetBestBlock(hashBlock); }
void Bitcredit_CCoinsViewBacked::SetBackend(Bitcredit_CCoinsView &viewIn) { base = &viewIn; }
bool Bitcredit_CCoinsViewBacked::BatchWrite(const std::map<uint256, Bitcredit_CCoins> &mapCoins, const uint256 &hashBlock) { return base->BatchWrite(mapCoins, hashBlock); }
bool Bitcredit_CCoinsViewBacked::GetStats(Bitcredit_CCoinsStats &stats) { return base->GetStats(stats); }

Credits_CCoinsViewCache::Credits_CCoinsViewCache(Bitcredit_CCoinsView &baseIn, bool fDummy) : Bitcredit_CCoinsViewBacked(baseIn), hashBlock(0) { }

bool Credits_CCoinsViewCache::GetCoins(const uint256 &txid, Bitcredit_CCoins &coins) {
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

std::map<uint256,Bitcredit_CCoins>::iterator Credits_CCoinsViewCache::FetchCoins(const uint256 &txid) {
    std::map<uint256,Bitcredit_CCoins>::iterator it = cacheCoins.lower_bound(txid);
    if (it != cacheCoins.end() && it->first == txid)
        return it;
    Bitcredit_CCoins tmp;
    if (!base->GetCoins(txid,tmp))
        return cacheCoins.end();
    std::map<uint256,Bitcredit_CCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcredit_CCoins()));
    tmp.swap(ret->second);
    return ret;
}

Bitcredit_CCoins &Credits_CCoinsViewCache::GetCoins(const uint256 &txid) {
    std::map<uint256,Bitcredit_CCoins>::iterator it = FetchCoins(txid);
    assert_with_stacktrace(it != cacheCoins.end(), strprintf("Credits_CCoinsViewCache::GetCoins() tx could not be found, txid: %s", txid.GetHex()));
    return it->second;
}

bool Credits_CCoinsViewCache::SetCoins(const uint256 &txid, const Bitcredit_CCoins &coins) {
    cacheCoins[txid] = coins;
    return true;
}

bool Credits_CCoinsViewCache::HaveCoins(const uint256 &txid) {
    return FetchCoins(txid) != cacheCoins.end();
}

uint256 Credits_CCoinsViewCache::GetBestBlock() {
    if (hashBlock == uint256(0))
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

bool Credits_CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}

bool Credits_CCoinsViewCache::BatchWrite(const std::map<uint256, Bitcredit_CCoins> &mapCoins, const uint256 &hashBlockIn) {
    for (std::map<uint256, Bitcredit_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        cacheCoins[it->first] = it->second;
    hashBlock = hashBlockIn;
    return true;
}

bool Credits_CCoinsViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, hashBlock);
    if (fOk)
        cacheCoins.clear();
    return fOk;
}

unsigned int Credits_CCoinsViewCache::GetCacheSize() {
    return cacheCoins.size();
}

const CTxOut &Credits_CCoinsViewCache::GetOutputFor(const Credits_CTxIn& input)
{
    const Bitcredit_CCoins &coins = GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t Credits_CCoinsViewCache::GetValueIn(const Credits_CTransaction& tx)
{
	assert(!tx.IsClaim());

    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		nResult += GetOutputFor(tx.vin[i]).nValue;
	}

    return nResult;
}

bool Credits_CCoinsViewCache::HaveInputs(const Credits_CTransaction& tx)
{
	assert(!tx.IsClaim());

    if (!tx.IsCoinBase()) {
        // first check whether information about the prevout hash is available
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			const Credits_CTxIn &ctxIn = tx.vin[i];
			const COutPoint &prevout = ctxIn.prevout;
			if (!HaveCoins(prevout.hash))
				return false;
		}

        // then check whether the actual outputs are available
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			const Credits_CTxIn &ctxIn = tx.vin[i];
			const COutPoint &prevout = ctxIn.prevout;
			const Bitcredit_CCoins &coins = GetCoins(prevout.hash);
			if (!coins.IsAvailable(prevout.n))
				return false;
		}
    }
    return true;
}

double Credits_CCoinsViewCache::GetPriority(const Credits_CTransaction &tx, int nHeight)
{
	assert(!tx.IsClaim());

    if (tx.IsCoinBase())
        return 0.0;

    double dResult = 0.0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &ctxIn = tx.vin[i];
		const Bitcredit_CCoins &coins = GetCoins(ctxIn.prevout.hash);
		if (!coins.IsAvailable(ctxIn.prevout.n)) continue;
		if (coins.nHeight < nHeight) {
			dResult += coins.vout[ctxIn.prevout.n].nValue * (nHeight-coins.nHeight);
		}
	}
    return tx.ComputePriority(dResult);
}
