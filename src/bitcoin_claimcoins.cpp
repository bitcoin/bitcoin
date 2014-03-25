// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_claimcoins.h"

#include <assert.h>

// calculate number of bytes for the bitmask, and its number of non-zero bytes
// each bit in the bitmask represents the availability of one output, but the
// availabilities of the first two outputs are encoded separately
void Bitcoin_CClaimCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes, const std::vector<CTxOutClaim>& vout) const {
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

bool Bitcoin_CClaimCoins::Spend(const COutPoint &out, Bitcoin_CTxInUndoClaim &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    undo = Bitcoin_CTxInUndoClaim(vout[out.n]);
    vout[out.n].SetNull();
    Cleanup();
    if (vout.size() == 0) {
        undo.nHeight = nHeight;
        undo.fCoinBase = fCoinBase;
        undo.nVersion = this->nVersion;
    }
    return true;
}
bool Bitcoin_CClaimCoins::SpendByClaiming(const COutPoint &out, Bitcredit_CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    if (vout[out.n].nValueClaimable <= 0)
        return false;

    undo = Bitcredit_CTxInUndo(CTxOut(vout[out.n].nValueClaimable, vout[out.n].scriptPubKey));

    vout[out.n].nValueClaimable = 0;

    Cleanup();

    return true;
}

std::string Bitcoin_CClaimCoins::ToString() const
{
    std::string str;
    str += strprintf("Bitcredit_CClaimCoin(ver=%d, height=%d, coinbase=%b)\n",
        nVersion,
        nHeight,
        fCoinBase);
	for (unsigned int i = 0; i < vout.size(); i++) {
		str += "    " + vout[i].ToString() + "\n";
	}

    return str;
}

void Bitcoin_CClaimCoins::print() const
{
    LogPrintf("%s", ToString());
}


bool Bitcoin_CClaimCoinsView::GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) { return false; }
bool Bitcoin_CClaimCoinsView::SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) { return false; }
bool Bitcoin_CClaimCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcoin_CClaimCoinsView::GetBestBlock() { return uint256(0); }
bool Bitcoin_CClaimCoinsView::SetBestBlock(const uint256 &hashBlock) { return false; }
uint256 Bitcoin_CClaimCoinsView::GetBitcreditClaimTip() { return uint256(0); }
bool Bitcoin_CClaimCoinsView::SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return false; }
int64_t Bitcoin_CClaimCoinsView::GetTotalClaimedCoins() { return int64_t(0); }
bool Bitcoin_CClaimCoinsView::SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return false; }
bool Bitcoin_CClaimCoinsView::BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return false; }
bool Bitcoin_CClaimCoinsView::GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) { return false;};
bool Bitcoin_CClaimCoinsView::GetStats(Bitcoin_CClaimCoinsStats &stats) { return false; }


Bitcoin_CClaimCoinsViewBacked::Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn) : base(&viewIn), tmpDb(NULL) { }
Bitcoin_CClaimCoinsViewBacked::Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn, Bitcoin_CClaimCoinsView &tmpDbIn) : base(&viewIn), tmpDb(&tmpDbIn) { }
bool Bitcoin_CClaimCoinsViewBacked::GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) { return base->GetCoins(txid, coins); }
bool Bitcoin_CClaimCoinsViewBacked::SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) { return base->SetCoins(txid, coins); }
bool Bitcoin_CClaimCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 Bitcoin_CClaimCoinsViewBacked::GetBestBlock() { return base->GetBestBlock(); }
bool Bitcoin_CClaimCoinsViewBacked::SetBestBlock(const uint256 &hashBlock) { return base->SetBestBlock(hashBlock); }
uint256 Bitcoin_CClaimCoinsViewBacked::GetBitcreditClaimTip() { return base->GetBitcreditClaimTip(); }
bool Bitcoin_CClaimCoinsViewBacked::SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return base->SetBitcreditClaimTip(hashBitcreditClaimTip); }
int64_t Bitcoin_CClaimCoinsViewBacked::GetTotalClaimedCoins() { return base->GetTotalClaimedCoins(); }
bool Bitcoin_CClaimCoinsViewBacked::SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return base->SetTotalClaimedCoins(totalClaimedCoins); }
void Bitcoin_CClaimCoinsViewBacked::SetBackend(Bitcoin_CClaimCoinsView &viewIn) { base = &viewIn; }
bool Bitcoin_CClaimCoinsViewBacked::BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return base->BatchWrite(mapCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins); }
bool Bitcoin_CClaimCoinsViewBacked::GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) { return tmpDb->GetCoinSlice(mapCoins, size, firstInvocation, fMore); };
bool Bitcoin_CClaimCoinsViewBacked::GetStats(Bitcoin_CClaimCoinsStats &stats) { return base->GetStats(stats); }


Bitcoin_CClaimCoinsViewCache::Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, unsigned int coinsCacheSizeIn, bool fDummy) : Bitcoin_CClaimCoinsViewBacked(baseIn), coinsCacheSize(coinsCacheSizeIn), hashBlock(0), hashBitcreditClaimTip(0), totalClaimedCoins(0) { }
Bitcoin_CClaimCoinsViewCache::Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, Bitcoin_CClaimCoinsView& tmpDbIn, unsigned int coinsCacheSizeIn, bool fDummy) : Bitcoin_CClaimCoinsViewBacked(baseIn, tmpDbIn), coinsCacheSize(coinsCacheSizeIn), hashBlock(0), hashBitcreditClaimTip(0), totalClaimedCoins(0) { }

void Bitcoin_CClaimCoinsViewCache::ClearCacheIfNeeded(bool fForce) {
	if((coinsCacheSize > 0 && GetCacheSize() > coinsCacheSize) || fForce) {
		const std::map<uint256, Bitcoin_CClaimCoins> empty;
		assert(BatchWrite(empty, hashBlock, hashBitcreditClaimTip, totalClaimedCoins));
		cacheCoins.clear();
	}
}

bool Bitcoin_CClaimCoinsViewCache::GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) {
	ClearCacheIfNeeded(false);

    if (cacheCoins.count(txid)) {
        coins = cacheCoins[txid];
        return true;
    }
    if(tmpDb) {
        if (tmpDb->GetCoins(txid, coins)) {
            cacheCoins[txid] = coins;
            return true;
        }
    }
    if (base->GetCoins(txid, coins)) {
        cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

std::map<uint256,Bitcoin_CClaimCoins>::iterator Bitcoin_CClaimCoinsViewCache::FetchCoins(const uint256 &txid) {
	ClearCacheIfNeeded(false);

    std::map<uint256,Bitcoin_CClaimCoins>::iterator it = cacheCoins.lower_bound(txid);
    if (it != cacheCoins.end() && it->first == txid)
        return it;
    if(tmpDb) {
        Bitcoin_CClaimCoins tmp;
        if (tmpDb->GetCoins(txid,tmp)) {
        	std::map<uint256,Bitcoin_CClaimCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CClaimCoins()));
        	tmp.swap(ret->second);
        	return ret;
        }
    }
    Bitcoin_CClaimCoins tmp;
    if (base->GetCoins(txid,tmp)) {
    	std::map<uint256,Bitcoin_CClaimCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CClaimCoins()));
    	tmp.swap(ret->second);
    	return ret;
    }
    return cacheCoins.end();
}

Bitcoin_CClaimCoins &Bitcoin_CClaimCoinsViewCache::GetCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CClaimCoins>::iterator it = FetchCoins(txid);
    assert(it != cacheCoins.end());
    return it->second;
}

bool Bitcoin_CClaimCoinsViewCache::SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) {
    cacheCoins[txid] = coins;
	ClearCacheIfNeeded(false);
    return true;
}

bool Bitcoin_CClaimCoinsViewCache::HaveCoins(const uint256 &txid) {
    return FetchCoins(txid) != cacheCoins.end();
}

uint256 Bitcoin_CClaimCoinsViewCache::GetBestBlock() {
    if(tmpDb) {
    	if (hashBlock == uint256(0))
    		hashBlock = tmpDb->GetBestBlock();
    }
    if (hashBlock == uint256(0))
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

bool Bitcoin_CClaimCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}
uint256 Bitcoin_CClaimCoinsViewCache::GetBitcreditClaimTip() {
    if(tmpDb) {
    	if (hashBitcreditClaimTip == uint256(0))
    		hashBitcreditClaimTip = tmpDb->GetBitcreditClaimTip();
    }
    if (hashBitcreditClaimTip == uint256(0))
    	hashBitcreditClaimTip = base->GetBitcreditClaimTip();
    return hashBitcreditClaimTip;
}

bool Bitcoin_CClaimCoinsViewCache::SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTipIn) {
	hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    return true;
}

int64_t Bitcoin_CClaimCoinsViewCache::GetTotalClaimedCoins() {
    if(tmpDb) {
    	if (totalClaimedCoins == int64_t(0))
    		totalClaimedCoins = tmpDb->GetTotalClaimedCoins();
    }
    if (totalClaimedCoins == int64_t(0))
    	totalClaimedCoins = base->GetTotalClaimedCoins();
    return totalClaimedCoins;
}

bool Bitcoin_CClaimCoinsViewCache::SetTotalClaimedCoins(const int64_t &totalClaimedCoinsIn) {
	totalClaimedCoins = totalClaimedCoinsIn;
    return true;
}

bool Bitcoin_CClaimCoinsViewCache::BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlockIn, const uint256 &hashBitcreditClaimTipIn, const int64_t &totalClaimedCoinsIn) {
    for (std::map<uint256, Bitcoin_CClaimCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        cacheCoins[it->first] = it->second;
    hashBlock = hashBlockIn;
    hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    totalClaimedCoins = totalClaimedCoinsIn;
    //Push updates all the way down
    bool fOk = false;
    if(tmpDb) {
    	fOk = tmpDb->BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
		if (fOk)
			cacheCoins.clear();
    } else {
		fOk = base->BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
		if (fOk)
			cacheCoins.clear();
    }
    return fOk;
}

//This function not implemented yet. For the moment coin slices are only available from db
bool Bitcoin_CClaimCoinsViewCache::GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) {
   assert(false);
   return false;
}

bool Bitcoin_CClaimCoinsViewCache::Flush() {
    bool fOk = true;
	if(tmpDb) {
		ClearCacheIfNeeded(true);

		bool fFirst = true;
		bool fMore = true;
		while(fMore) {
			std::map<uint256, Bitcoin_CClaimCoins> tmpCoins;
			fOk = tmpDb->GetCoinSlice(tmpCoins, coinsCacheSize, fFirst, fMore);
			if(!fOk) {
				return fOk;
			}

			LogPrintf("Committing %d transactions from tmp db. Writing to base...\n", tmpCoins.size());
			fOk = base->BatchWrite(tmpCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
			if(!fOk) {
				return fOk;
			}

			fFirst = false;
		}
	} else {
	    fOk = base->BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
	    if (fOk)
	        cacheCoins.clear();
	}
    return fOk;
}

unsigned int Bitcoin_CClaimCoinsViewCache::GetCacheSize() {
    return cacheCoins.size();
}

const CTxOutClaim& Bitcoin_CClaimCoinsViewCache::GetOut(const COutPoint &outpoint) {
	const Bitcoin_CClaimCoins &coins = GetCoins(outpoint.hash);
	assert(coins.IsAvailable(outpoint.n));
	return coins.vout[outpoint.n];
}

const CScript &Bitcoin_CClaimCoinsViewCache::GetOutputScriptFor(const Bitcredit_CTxIn& input) {
	return GetOut(input.prevout).scriptPubKey;
}

void Bitcoin_CClaimCoinsViewCache::GetValueIn(const Bitcoin_CTransaction& tx, ClaimSum& claimSum)
{
    if (tx.IsCoinBase())
        return;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Bitcoin_CClaimCoins &coins = GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		claimSum.nValueOriginalSum += coins.vout[prevout.n].nValueOriginal;

		if(coins.vout[prevout.n].nValueClaimable > 0) {
			claimSum.nValueClaimableSum += coins.vout[prevout.n].nValueClaimable;
		}
    }

    return;
}
int64_t Bitcoin_CClaimCoinsViewCache::GetValueIn(const Bitcredit_CTransaction& tx) {
	assert(tx.IsClaim());

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Bitcoin_CClaimCoins &coins = GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		nResult += coins.vout[prevout.n].nValueClaimable;
	}

    return nResult;
}

bool Bitcoin_CClaimCoinsViewCache::HaveInputs(const Bitcredit_CTransaction& tx) {
	assert(tx.IsClaim());

	// first check whether information about the prevout hash is available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Bitcredit_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		if (!HaveCoins(prevout.hash))
			return false;
	}

	// then check whether the actual outputs are available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Bitcredit_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		const Bitcoin_CClaimCoins &coins = GetCoins(prevout.hash);
		if (!coins.HasClaimable(prevout.n))
			return false;
	}

    return true;
}

double Bitcoin_CClaimCoinsViewCache::GetPriority(const Bitcredit_CTransaction &tx, int nHeight) {
	assert(tx.IsClaim());

    double dResult = 0.0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Bitcredit_CTxIn &txIn = tx.vin[i];
		const Bitcoin_CClaimCoins &coins = GetCoins(txIn.prevout.hash);

		if(coins.HasClaimable(txIn.prevout.n)) {
			if (coins.nHeight < nHeight) {
				dResult += coins.vout[txIn.prevout.n].nValueClaimable * (nHeight-coins.nHeight);
			}
		}
	}
    return tx.ComputePriority(dResult);
}
