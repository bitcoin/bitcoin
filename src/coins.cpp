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

void Claim_CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes, const std::vector<CTxOutClaim>& vout) const {
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

bool Claim_CCoins::Spend(const COutPoint &out, Bitcoin_CTxInUndoClaim &undo) {
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
bool Claim_CCoins::SpendByClaiming(const COutPoint &out, Credits_CTxInUndo &undo) {
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

std::string Claim_CCoins::ToString() const
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

void Claim_CCoins::print() const
{
    LogPrintf("%s", ToString());
}



bool Credits_CCoinsView::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) { return false; }
bool Credits_CCoinsView::Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins) { return false; }
bool Credits_CCoinsView::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) { return false; }
bool Credits_CCoinsView::Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins) { return false; }
bool Credits_CCoinsView::Credits_HaveCoins(const uint256 &txid) { return false; }
bool Credits_CCoinsView::Claim_HaveCoins(const uint256 &txid) { return false; }
uint256 Credits_CCoinsView::Credits_GetBestBlock() { return uint256(0); }
uint256 Credits_CCoinsView::Claim_GetBestBlock() { return uint256(0); }
bool Credits_CCoinsView::Credits_SetBestBlock(const uint256 &hashBlock) { return false; }
bool Credits_CCoinsView::Claim_SetBestBlock(const uint256 &hashBlock) { return false; }
uint256 Credits_CCoinsView::Claim_GetBitcreditClaimTip() { return uint256(0); }
bool Credits_CCoinsView::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return false; }
int64_t Credits_CCoinsView::Claim_GetTotalClaimedCoins() { return int64_t(0); }
bool Credits_CCoinsView::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return false; }
bool Credits_CCoinsView::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock) { return false; }
bool Credits_CCoinsView::Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return false; }
bool Credits_CCoinsView::All_BatchWrite(const std::map<uint256, Credits_CCoins> &credits_mapCoins, const uint256 &credits_hashBlock, const std::map<uint256, Claim_CCoins> &claim_mapCoins, const uint256 &claim_hashBlock, const uint256 &claim_hashBitcreditClaimTip, const int64_t &claim_totalClaimedCoins) { return false; }
bool Credits_CCoinsView::Credits_GetStats(Credits_CCoinsStats &stats) { return false; }
bool Credits_CCoinsView::Claim_GetStats(Claim_CCoinsStats &stats) { return false; }


Credits_CCoinsViewBacked::Credits_CCoinsViewBacked(Credits_CCoinsView &viewIn) : base(&viewIn) { }
bool Credits_CCoinsViewBacked::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) { return base->Credits_GetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins) { return base->Claim_GetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) { return base->Credits_SetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins) { return base->Claim_SetCoins(txid, coins); }
bool Credits_CCoinsViewBacked::Credits_HaveCoins(const uint256 &txid) { return base->Credits_HaveCoins(txid); }
bool Credits_CCoinsViewBacked::Claim_HaveCoins(const uint256 &txid) { return base->Claim_HaveCoins(txid); }
uint256 Credits_CCoinsViewBacked::Credits_GetBestBlock() { return base->Credits_GetBestBlock(); }
uint256 Credits_CCoinsViewBacked::Claim_GetBestBlock() { return base->Claim_GetBestBlock(); }
bool Credits_CCoinsViewBacked::Credits_SetBestBlock(const uint256 &hashBlock) { return base->Credits_SetBestBlock(hashBlock); }
bool Credits_CCoinsViewBacked::Claim_SetBestBlock(const uint256 &hashBlock) { return base->Claim_SetBestBlock(hashBlock); }
uint256 Credits_CCoinsViewBacked::Claim_GetBitcreditClaimTip() { return base->Claim_GetBitcreditClaimTip(); }
bool Credits_CCoinsViewBacked::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip) { return base->Claim_SetBitcreditClaimTip(hashBitcreditClaimTip); }
int64_t Credits_CCoinsViewBacked::Claim_GetTotalClaimedCoins() { return base->Claim_GetTotalClaimedCoins(); }
bool Credits_CCoinsViewBacked::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return base->Claim_SetTotalClaimedCoins(totalClaimedCoins); }
void Credits_CCoinsViewBacked::Credits_SetBackend(Credits_CCoinsView &viewIn) { base = &viewIn; }
bool Credits_CCoinsViewBacked::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock) { return base->Credits_BatchWrite(mapCoins, hashBlock); }
bool Credits_CCoinsViewBacked::Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) { return base->Claim_BatchWrite(mapCoins, hashBlock, hashBitcreditClaimTip, totalClaimedCoins); }
bool Credits_CCoinsViewBacked::All_BatchWrite(const std::map<uint256, Credits_CCoins> &credits_mapCoins, const uint256 &credits_hashBlock, const std::map<uint256, Claim_CCoins> &claim_mapCoins, const uint256 &claim_hashBlock, const uint256 &claim_hashBitcreditClaimTip, const int64_t &claim_totalClaimedCoins) { return base->All_BatchWrite(credits_mapCoins, credits_hashBlock, claim_mapCoins, claim_hashBlock, claim_hashBitcreditClaimTip, claim_totalClaimedCoins); }
bool Credits_CCoinsViewBacked::Credits_GetStats(Credits_CCoinsStats &stats) { return base->Credits_GetStats(stats); }
bool Credits_CCoinsViewBacked::Claim_GetStats(Claim_CCoinsStats &stats) { return base->Claim_GetStats(stats); }


Credits_CCoinsViewCache::Credits_CCoinsViewCache(Credits_CCoinsView &baseIn, bool fDummy) : Credits_CCoinsViewBacked(baseIn), credits_hashBlock(0), claim_hashBlock(0), claim_hashBitcreditClaimTip(0), claim_totalClaimedCoins(0){ }

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
bool Credits_CCoinsViewCache::Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins) {
    if (claim_cacheCoins.count(txid)) {
        coins = claim_cacheCoins[txid];
        return true;
    }
    if (base->Claim_GetCoins(txid, coins)) {
    	claim_cacheCoins[txid] = coins;
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
std::map<uint256,Claim_CCoins>::iterator Credits_CCoinsViewCache::Claim_FetchCoins(const uint256 &txid) {
    std::map<uint256,Claim_CCoins>::iterator it = claim_cacheCoins.lower_bound(txid);
    if (it != claim_cacheCoins.end() && it->first == txid)
        return it;
    Claim_CCoins tmp;
    if (base->Claim_GetCoins(txid,tmp)) {
    	std::map<uint256,Claim_CCoins>::iterator ret = claim_cacheCoins.insert(it, std::make_pair(txid, Claim_CCoins()));
    	tmp.swap(ret->second);
    	return ret;
    }
    return claim_cacheCoins.end();
}

Credits_CCoins &Credits_CCoinsViewCache::Credits_GetCoins(const uint256 &txid) {
    std::map<uint256,Credits_CCoins>::iterator it = Credits_FetchCoins(txid);
    assert_with_stacktrace(it != credits_cacheCoins.end(), strprintf("Credits_CCoinsViewCache::GetCoins() tx could not be found, txid: %s", txid.GetHex()));
    return it->second;
}
Claim_CCoins &Credits_CCoinsViewCache::Claim_GetCoins(const uint256 &txid) {
    std::map<uint256,Claim_CCoins>::iterator it = Claim_FetchCoins(txid);
    assert(it != claim_cacheCoins.end());
    return it->second;
}


bool Credits_CCoinsViewCache::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) {
	credits_cacheCoins[txid] = coins;
    return true;
}
bool Credits_CCoinsViewCache::Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins) {
	claim_cacheCoins[txid] = coins;
    return true;
}

bool Credits_CCoinsViewCache::Credits_HaveCoins(const uint256 &txid) {
    return Credits_FetchCoins(txid) != credits_cacheCoins.end();
}
bool Credits_CCoinsViewCache::Claim_HaveCoins(const uint256 &txid) {
    return Claim_FetchCoins(txid) != claim_cacheCoins.end();
}

uint256 Credits_CCoinsViewCache::Credits_GetBestBlock() {
    if (credits_hashBlock == uint256(0))
    	credits_hashBlock = base->Credits_GetBestBlock();
    return credits_hashBlock;
}
uint256 Credits_CCoinsViewCache::Claim_GetBestBlock() {
    if (claim_hashBlock == uint256(0))
    	claim_hashBlock = base->Claim_GetBestBlock();
    return claim_hashBlock;
}

bool Credits_CCoinsViewCache::Credits_SetBestBlock(const uint256 &hashBlockIn) {
	credits_hashBlock = hashBlockIn;
    return true;
}
bool Credits_CCoinsViewCache::Claim_SetBestBlock(const uint256 &hashBlockIn) {
	claim_hashBlock = hashBlockIn;
    return true;
}

uint256 Credits_CCoinsViewCache::Claim_GetBitcreditClaimTip() {
    if (claim_hashBitcreditClaimTip == uint256(0))
    	claim_hashBitcreditClaimTip = base->Claim_GetBitcreditClaimTip();
    return claim_hashBitcreditClaimTip;
}
bool Credits_CCoinsViewCache::Claim_SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTipIn) {
	claim_hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    return true;
}

int64_t Credits_CCoinsViewCache::Claim_GetTotalClaimedCoins() {
    if (claim_totalClaimedCoins == int64_t(0))
    	claim_totalClaimedCoins = base->Claim_GetTotalClaimedCoins();
    return claim_totalClaimedCoins;
}

bool Credits_CCoinsViewCache::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoinsIn) {
	claim_totalClaimedCoins = totalClaimedCoinsIn;
    return true;
}

bool Credits_CCoinsViewCache::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlockIn) {
    for (std::map<uint256, Credits_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	credits_cacheCoins[it->first] = it->second;
    credits_hashBlock = hashBlockIn;
    return true;
}
bool Credits_CCoinsViewCache::Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlockIn, const uint256 &hashBitcreditClaimTipIn, const int64_t &totalClaimedCoinsIn) {
    for (std::map<uint256, Claim_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	claim_cacheCoins[it->first] = it->second;
    claim_hashBlock = hashBlockIn;
    claim_hashBitcreditClaimTip = hashBitcreditClaimTipIn;
    claim_totalClaimedCoins = totalClaimedCoinsIn;
    return true;
}
bool Credits_CCoinsViewCache::All_BatchWrite(const std::map<uint256, Credits_CCoins> &credits_mapCoins, const uint256 &credits_hashBlockIn, const std::map<uint256, Claim_CCoins> &claim_mapCoins, const uint256 &claim_hashBlockIn, const uint256 &claim_hashBitcreditClaimTipIn, const int64_t &claim_totalClaimedCoinsIn) {
    for (std::map<uint256, Credits_CCoins>::const_iterator it = credits_mapCoins.begin(); it != credits_mapCoins.end(); it++)
    	credits_cacheCoins[it->first] = it->second;
    credits_hashBlock = credits_hashBlockIn;

    for (std::map<uint256, Claim_CCoins>::const_iterator it = claim_mapCoins.begin(); it != claim_mapCoins.end(); it++)
    	claim_cacheCoins[it->first] = it->second;
    claim_hashBlock = claim_hashBlockIn;
    claim_hashBitcreditClaimTip = claim_hashBitcreditClaimTipIn;
    claim_totalClaimedCoins = claim_totalClaimedCoinsIn;
    return true;
}

bool Credits_CCoinsViewCache::Credits_Flush() {
    bool fOk = base->Credits_BatchWrite(credits_cacheCoins, credits_hashBlock);
    if (fOk)
    	credits_cacheCoins.clear();
    return fOk;
}
bool Credits_CCoinsViewCache::Claim_Flush() {
    bool  fOk = base->Claim_BatchWrite(claim_cacheCoins, claim_hashBlock, claim_hashBitcreditClaimTip, claim_totalClaimedCoins);
	if (fOk)
		claim_cacheCoins.clear();
    return fOk;
}
bool Credits_CCoinsViewCache::All_Flush() {
    bool  fOk = base->All_BatchWrite(credits_cacheCoins, credits_hashBlock, claim_cacheCoins, claim_hashBlock, claim_hashBitcreditClaimTip, claim_totalClaimedCoins);
	if (fOk) {
    	credits_cacheCoins.clear();
		claim_cacheCoins.clear();
	}
    return fOk;
}

unsigned int Credits_CCoinsViewCache::GetCacheSize() {
    return credits_cacheCoins.size() + claim_cacheCoins.size();
}


const CTxOut &Credits_CCoinsViewCache::Credits_GetOutputFor(const Credits_CTxIn& input)
{
    const Credits_CCoins &coins = Credits_GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}
const CTxOutClaim& Credits_CCoinsViewCache::Claim_GetOut(const COutPoint &outpoint) {
	const Claim_CCoins &coins = Claim_GetCoins(outpoint.hash);
	assert(coins.IsAvailable(outpoint.n));
	return coins.vout[outpoint.n];
}
const CScript &Credits_CCoinsViewCache::Claim_GetOutputScriptFor(const Credits_CTxIn& input) {
	return Claim_GetOut(input.prevout).scriptPubKey;
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
void Credits_CCoinsViewCache::Claim_GetValueIn(const Bitcoin_CTransaction& tx, ClaimSum& claimSum)
{
    if (tx.IsCoinBase())
        return;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Claim_CCoins &coins = Claim_GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		claimSum.nValueOriginalSum += coins.vout[prevout.n].nValueOriginal;

		if(coins.vout[prevout.n].nValueClaimable > 0) {
			claimSum.nValueClaimableSum += coins.vout[prevout.n].nValueClaimable;
		}
    }

    return;
}
int64_t Credits_CCoinsViewCache::Claim_GetValueIn(const Credits_CTransaction& tx) {
	assert(tx.IsClaim());

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const COutPoint &prevout = tx.vin[i].prevout;
		const Claim_CCoins &coins = Claim_GetCoins(prevout.hash);

		assert(coins.IsAvailable(prevout.n));

		nResult += coins.vout[prevout.n].nValueClaimable;
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
bool Credits_CCoinsViewCache::Claim_HaveInputs(const Credits_CTransaction& tx) {
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
		const Claim_CCoins &coins = Claim_GetCoins(prevout.hash);
		if (!coins.HasClaimable(prevout.n))
			return false;
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
double Credits_CCoinsViewCache::Claim_GetPriority(const Credits_CTransaction &tx, int nHeight) {
	assert(tx.IsClaim());

    double dResult = 0.0;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const Claim_CCoins &coins = Claim_GetCoins(txIn.prevout.hash);

		if(coins.HasClaimable(txIn.prevout.n)) {
			if (coins.nHeight < nHeight) {
				dResult += coins.vout[txIn.prevout.n].nValueClaimable * (nHeight-coins.nHeight);
			}
		}
	}
    return tx.ComputePriority(dResult);
}
