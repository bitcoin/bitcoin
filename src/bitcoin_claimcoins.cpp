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
bool Bitcoin_CClaimCoins::SpendByClaiming(const COutPoint &out, Credits_CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    if (vout[out.n].nValueClaimable <= 0)
        return false;

    undo = Credits_CTxInUndo(CTxOut(vout[out.n].nValueClaimable, vout[out.n].scriptPubKey));

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


bool Bitcoin_CClaimCoinsView::Claim_GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) { return false; }
bool Bitcoin_CClaimCoinsView::Claim_SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) { return false; }
bool Bitcoin_CClaimCoinsView::Claim_HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcoin_CClaimCoinsView::Claim_GetBestBlock() { return uint256(0); }
bool Bitcoin_CClaimCoinsView::Claim_SetBestBlock(const uint256 &hashBlock) { return false; }
uint256 Bitcoin_CClaimCoinsView::Claim_GetBitcreditClaimTip() { return uint256(0); }
bool Bitcoin_CClaimCoinsView::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return false; }
int64_t Bitcoin_CClaimCoinsView::Claim_GetTotalClaimedCoins() { return int64_t(0); }
bool Bitcoin_CClaimCoinsView::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return false; }
bool Bitcoin_CClaimCoinsView::Claim_BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return false; }
bool Bitcoin_CClaimCoinsView::Claim_GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) { return false;};
bool Bitcoin_CClaimCoinsView::Claim_GetStats(Bitcoin_CClaimCoinsStats &stats) { return false; }


Bitcoin_CClaimCoinsViewBacked::Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn) : base(&viewIn), tmpDb(NULL) { }
Bitcoin_CClaimCoinsViewBacked::Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn, Bitcoin_CClaimCoinsView &tmpDbIn) : base(&viewIn), tmpDb(&tmpDbIn) { }
bool Bitcoin_CClaimCoinsViewBacked::Claim_GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) { return base->Claim_GetCoins(txid, coins); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) { return base->Claim_SetCoins(txid, coins); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_HaveCoins(const uint256 &txid) { return base->Claim_HaveCoins(txid); }
uint256 Bitcoin_CClaimCoinsViewBacked::Claim_GetBestBlock() { return base->Claim_GetBestBlock(); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_SetBestBlock(const uint256 &hashBlock) { return base->Claim_SetBestBlock(hashBlock); }
uint256 Bitcoin_CClaimCoinsViewBacked::Claim_GetBitcreditClaimTip() { return base->Claim_GetBitcreditClaimTip(); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return base->Claim_SetBitcreditClaimTip(hashBitcreditClaimTip); }
int64_t Bitcoin_CClaimCoinsViewBacked::Claim_GetTotalClaimedCoins() { return base->Claim_GetTotalClaimedCoins(); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return base->Claim_SetTotalClaimedCoins(totalClaimedCoins); }
void Bitcoin_CClaimCoinsViewBacked::Claim_SetBackend(Bitcoin_CClaimCoinsView &viewIn) { base = &viewIn; }
bool Bitcoin_CClaimCoinsViewBacked::Claim_BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return base->Claim_BatchWrite(mapCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins); }
bool Bitcoin_CClaimCoinsViewBacked::Claim_GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) { return tmpDb->Claim_GetCoinSlice(mapCoins, size, firstInvocation, fMore); };
bool Bitcoin_CClaimCoinsViewBacked::Claim_GetStats(Bitcoin_CClaimCoinsStats &stats) { return base->Claim_GetStats(stats); }


Bitcoin_CClaimCoinsViewCache::Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, unsigned int coinsCacheSizeIn, bool fDummy) : Bitcoin_CClaimCoinsViewBacked(baseIn), coinsCacheSize(coinsCacheSizeIn), hashBlock(0), hashBitcreditClaimTip(0), totalClaimedCoins(0) { }
Bitcoin_CClaimCoinsViewCache::Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, Bitcoin_CClaimCoinsView& tmpDbIn, unsigned int coinsCacheSizeIn, bool fDummy) : Bitcoin_CClaimCoinsViewBacked(baseIn, tmpDbIn), coinsCacheSize(coinsCacheSizeIn), hashBlock(0), hashBitcreditClaimTip(0), totalClaimedCoins(0) { }

void Bitcoin_CClaimCoinsViewCache::Claim_ClearCacheIfNeeded(bool fForce) {
	if((coinsCacheSize > 0 && Claim_GetCacheSize() > coinsCacheSize) || fForce) {
		const std::map<uint256, Bitcoin_CClaimCoins> empty;
		assert(Claim_BatchWrite(empty, hashBlock, hashBitcreditClaimTip, totalClaimedCoins));
		cacheCoins.clear();
	}
}

bool Bitcoin_CClaimCoinsViewCache::Claim_GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) {
	Claim_ClearCacheIfNeeded(false);

    if (cacheCoins.count(txid)) {
        coins = cacheCoins[txid];
        return true;
    }
    if(tmpDb) {
        if (tmpDb->Claim_GetCoins(txid, coins)) {
            cacheCoins[txid] = coins;
            return true;
        }
    }
    if (base->Claim_GetCoins(txid, coins)) {
        cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

std::map<uint256,Bitcoin_CClaimCoins>::iterator Bitcoin_CClaimCoinsViewCache::Claim_FetchCoins(const uint256 &txid) {
	Claim_ClearCacheIfNeeded(false);

    std::map<uint256,Bitcoin_CClaimCoins>::iterator it = cacheCoins.lower_bound(txid);
    if (it != cacheCoins.end() && it->first == txid)
        return it;
    if(tmpDb) {
        Bitcoin_CClaimCoins tmp;
        if (tmpDb->Claim_GetCoins(txid,tmp)) {
        	std::map<uint256,Bitcoin_CClaimCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CClaimCoins()));
        	tmp.swap(ret->second);
        	return ret;
        }
    }
    Bitcoin_CClaimCoins tmp;
    if (base->Claim_GetCoins(txid,tmp)) {
    	std::map<uint256,Bitcoin_CClaimCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, Bitcoin_CClaimCoins()));
    	tmp.swap(ret->second);
    	return ret;
    }
    return cacheCoins.end();
}

Bitcoin_CClaimCoins &Bitcoin_CClaimCoinsViewCache::Claim_GetCoins(const uint256 &txid) {
    std::map<uint256,Bitcoin_CClaimCoins>::iterator it = Claim_FetchCoins(txid);
    assert(it != cacheCoins.end());
    return it->second;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) {
    cacheCoins[txid] = coins;
    Claim_ClearCacheIfNeeded(false);
    return true;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_HaveCoins(const uint256 &txid) {
    return Claim_FetchCoins(txid) != cacheCoins.end();
}

uint256 Bitcoin_CClaimCoinsViewCache::Claim_GetBestBlock() {
    if(tmpDb) {
    	if (hashBlock == uint256(0))
    		hashBlock = tmpDb->Claim_GetBestBlock();
    }
    if (hashBlock == uint256(0))
        hashBlock = base->Claim_GetBestBlock();
    return hashBlock;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}
uint256 Bitcoin_CClaimCoinsViewCache::Claim_GetBitcreditClaimTip() {
    if(tmpDb) {
    	if (hashBitcreditClaimTip == uint256(0))
    		hashBitcreditClaimTip = tmpDb->Claim_GetBitcreditClaimTip();
    }
    if (hashBitcreditClaimTip == uint256(0))
    	hashBitcreditClaimTip = base->Claim_GetBitcreditClaimTip();
    return hashBitcreditClaimTip;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTipIn) {
	hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    return true;
}

int64_t Bitcoin_CClaimCoinsViewCache::Claim_GetTotalClaimedCoins() {
    if(tmpDb) {
    	if (totalClaimedCoins == int64_t(0))
    		totalClaimedCoins = tmpDb->Claim_GetTotalClaimedCoins();
    }
    if (totalClaimedCoins == int64_t(0))
    	totalClaimedCoins = base->Claim_GetTotalClaimedCoins();
    return totalClaimedCoins;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoinsIn) {
	totalClaimedCoins = totalClaimedCoinsIn;
    return true;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlockIn, const uint256 &hashBitcreditClaimTipIn, const int64_t &totalClaimedCoinsIn) {
    for (std::map<uint256, Bitcoin_CClaimCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        cacheCoins[it->first] = it->second;
    hashBlock = hashBlockIn;
    hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    totalClaimedCoins = totalClaimedCoinsIn;
    //Push updates all the way down
    bool fOk = false;
    if(tmpDb) {
    	fOk = tmpDb->Claim_BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
		if (fOk)
			cacheCoins.clear();
    } else {
		fOk = base->Claim_BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
		if (fOk)
			cacheCoins.clear();
    }
    return fOk;
}

//This function not implemented yet. For the moment coin slices are only available from db
bool Bitcoin_CClaimCoinsViewCache::Claim_GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) {
   assert(false);
   return false;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_Flush() {
    bool fOk = true;
	if(tmpDb) {
		Claim_ClearCacheIfNeeded(true);

		bool fFirst = true;
		bool fMore = true;
		while(fMore) {
			std::map<uint256, Bitcoin_CClaimCoins> tmpCoins;
			fOk = tmpDb->Claim_GetCoinSlice(tmpCoins, coinsCacheSize, fFirst, fMore);
			if(!fOk) {
				return fOk;
			}

			LogPrintf("Committing %d transactions from tmp db. Writing to base...\n", tmpCoins.size());
			fOk = base->Claim_BatchWrite(tmpCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
			if(!fOk) {
				return fOk;
			}

			fFirst = false;
		}
	} else {
	    fOk = base->Claim_BatchWrite(cacheCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins);
	    if (fOk)
	        cacheCoins.clear();
	}
    return fOk;
}

unsigned int Bitcoin_CClaimCoinsViewCache::Claim_GetCacheSize() {
    return cacheCoins.size();
}

const CTxOutClaim& Bitcoin_CClaimCoinsViewCache::Claim_GetOut(const COutPoint &outpoint) {
	const Bitcoin_CClaimCoins &coins = Claim_GetCoins(outpoint.hash);
	assert(coins.IsAvailable(outpoint.n));
	return coins.vout[outpoint.n];
}

const CScript &Bitcoin_CClaimCoinsViewCache::Claim_GetOutputScriptFor(const Credits_CTxIn& input) {
	return Claim_GetOut(input.prevout).scriptPubKey;
}

void Bitcoin_CClaimCoinsViewCache::Claim_GetValueIn(const Bitcoin_CTransaction& tx, ClaimSum& claimSum)
{
    if (tx.IsCoinBase())
        return;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Bitcoin_CClaimCoins &coins = Claim_GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		claimSum.nValueOriginalSum += coins.vout[prevout.n].nValueOriginal;

		if(coins.vout[prevout.n].nValueClaimable > 0) {
			claimSum.nValueClaimableSum += coins.vout[prevout.n].nValueClaimable;
		}
    }

    return;
}
int64_t Bitcoin_CClaimCoinsViewCache::Claim_GetValueIn(const Credits_CTransaction& tx) {
	assert(tx.IsClaim());

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Bitcoin_CClaimCoins &coins = Claim_GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		nResult += coins.vout[prevout.n].nValueClaimable;
	}

    return nResult;
}

bool Bitcoin_CClaimCoinsViewCache::Claim_HaveInputs(const Credits_CTransaction& tx) {
	assert(tx.IsClaim());

	// first check whether information about the prevout hash is available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		if (!Claim_HaveCoins(prevout.hash))
			return false;
	}

	// then check whether the actual outputs are available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		const Bitcoin_CClaimCoins &coins = Claim_GetCoins(prevout.hash);
		if (!coins.HasClaimable(prevout.n))
			return false;
	}

    return true;
}

double Bitcoin_CClaimCoinsViewCache::Claim_GetPriority(const Credits_CTransaction &tx, int nHeight) {
	assert(tx.IsClaim());

    double dResult = 0.0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const Bitcoin_CClaimCoins &coins = Claim_GetCoins(txIn.prevout.hash);

		if(coins.HasClaimable(txIn.prevout.n)) {
			if (coins.nHeight < nHeight) {
				dResult += coins.vout[txIn.prevout.n].nValueClaimable * (nHeight-coins.nHeight);
			}
		}
	}
    return tx.ComputePriority(dResult);
}
