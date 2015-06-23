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

void Bitcoin_CCoins::DeactivateClaimable() {
    for (unsigned int i = 0; i < vout.size(); i++) {
    	vout[i].nValueClaimable = 0;
    }
}

bool Bitcoin_CCoins::Bitcoin_Spend(const COutPoint &out) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    if (vout[out.n].IsOriginalSpent())
        return false;

    vout[out.n].SetOriginalSpent(1);

    Cleanup();
    return true;
}
bool Bitcoin_CCoins::Claim_Spend(const COutPoint &out, Bitcoin_CTxInUndo &undo) {
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
bool Bitcoin_CCoins::Credits_Spend(const COutPoint &out, Credits_CTxInUndo &undo) {
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

std::string Bitcoin_CCoins::ToString() const
{
    std::string str;
    str += strprintf("Bitcoin_CCoins(ver=%d, height=%d, coinbase=%b)\n",
        nVersion,
        nHeight,
        fCoinBase);
	for (unsigned int i = 0; i < vout.size(); i++) {
		str += "    " + vout[i].ToString() + "\n";
	}

    return str;
}

void Bitcoin_CCoins::print() const
{
    LogPrintf("%s", ToString());
}


bool Bitcoin_CCoinsView::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return false; }
bool Bitcoin_CCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 Bitcoin_CCoinsView::Bitcoin_GetBestBlock() { return uint256(0); }
bool Bitcoin_CCoinsView::Bitcoin_SetBestBlock(const uint256 &hashBlock) { return false; }
uint256 Bitcoin_CCoinsView::Claim_GetBestBlock() { return uint256(0); }
bool Bitcoin_CCoinsView::Claim_SetBestBlock(const uint256 &hashBlock) { return false; }
uint256 Bitcoin_CCoinsView::Claim_GetCreditsClaimTip() { return uint256(0); }
bool Bitcoin_CCoinsView::Claim_SetCreditsClaimTip(const uint256 &hashCreditsClaimTip) { return false; }
int64_t Bitcoin_CCoinsView::Claim_GetTotalClaimedCoins() { return int64_t(0); }
bool Bitcoin_CCoinsView::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return false; }
bool Bitcoin_CCoinsView::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &bitcoin_hashBlock, const uint256 &claim_hashBlock, const uint256 &claim_hashCreditsClaimTip, const int64_t &claim_totalClaimedCoins) { return false; }
bool Bitcoin_CCoinsView::GetStats(Bitcoin_CCoinsStats &stats) { return false; }

Bitcoin_CCoinsViewBacked::Bitcoin_CCoinsViewBacked(Bitcoin_CCoinsView &viewIn) : base(&viewIn) { }
bool Bitcoin_CCoinsViewBacked::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) { return base->GetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) { return base->SetCoins(txid, coins); }
bool Bitcoin_CCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 Bitcoin_CCoinsViewBacked::Bitcoin_GetBestBlock() { return base->Bitcoin_GetBestBlock(); }
bool Bitcoin_CCoinsViewBacked::Bitcoin_SetBestBlock(const uint256 &hashBlock) { return base->Bitcoin_SetBestBlock(hashBlock); }
bool Bitcoin_CCoinsViewBacked::Claim_SetBestBlock(const uint256 &hashBlock) { return base->Claim_SetBestBlock(hashBlock); }
uint256 Bitcoin_CCoinsViewBacked::Claim_GetBestBlock() { return base->Claim_GetBestBlock(); }
uint256 Bitcoin_CCoinsViewBacked::Claim_GetCreditsClaimTip() { return base->Claim_GetCreditsClaimTip(); }
bool Bitcoin_CCoinsViewBacked::Claim_SetCreditsClaimTip(const uint256 &hashCreditsClaimTip) { return base->Claim_SetCreditsClaimTip(hashCreditsClaimTip); }
int64_t Bitcoin_CCoinsViewBacked::Claim_GetTotalClaimedCoins() { return base->Claim_GetTotalClaimedCoins(); }
bool Bitcoin_CCoinsViewBacked::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) { return base->Claim_SetTotalClaimedCoins(totalClaimedCoins); }
void Bitcoin_CCoinsViewBacked::SetBackend(Bitcoin_CCoinsView &viewIn) { base = &viewIn; }
Bitcoin_CCoinsView *Bitcoin_CCoinsViewBacked::GetBackend() { return base;}
bool Bitcoin_CCoinsViewBacked::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &bitcoin_hashBlock, const uint256 &claim_hashBlock, const uint256 &claim_hashCreditsClaimTip, const int64_t &claim_totalClaimedCoins) { return base->BatchWrite(mapCoins, bitcoin_hashBlock, claim_hashBlock, claim_hashCreditsClaimTip, claim_totalClaimedCoins); }
bool Bitcoin_CCoinsViewBacked::GetStats(Bitcoin_CCoinsStats &stats) { return base->GetStats(stats); }

Bitcoin_CCoinsViewCache::Bitcoin_CCoinsViewCache(Bitcoin_CCoinsView &baseIn, bool fDummy) : Bitcoin_CCoinsViewBacked(baseIn), bitcoin_hashBlock(0), claim_hashBlock(0), claim_hashCreditsClaimTip(0), claim_totalClaimedCoins(0){ }

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

uint256 Bitcoin_CCoinsViewCache::Bitcoin_GetBestBlock() {
    if (bitcoin_hashBlock == uint256(0))
    	bitcoin_hashBlock = base->Bitcoin_GetBestBlock();
    return bitcoin_hashBlock;
}
uint256 Bitcoin_CCoinsViewCache::Claim_GetBestBlock() {
    if (claim_hashBlock == uint256(0))
    	claim_hashBlock = base->Claim_GetBestBlock();
    return claim_hashBlock;
}

bool Bitcoin_CCoinsViewCache::Bitcoin_SetBestBlock(const uint256 &hashBlockIn) {
	bitcoin_hashBlock = hashBlockIn;
    return true;
}
bool Bitcoin_CCoinsViewCache::Claim_SetBestBlock(const uint256 &hashBlockIn) {
	claim_hashBlock = hashBlockIn;
    return true;
}

uint256 Bitcoin_CCoinsViewCache::Claim_GetCreditsClaimTip() {
    if (claim_hashCreditsClaimTip == uint256(0))
    	claim_hashCreditsClaimTip = base->Claim_GetCreditsClaimTip();
    return claim_hashCreditsClaimTip;
}
bool Bitcoin_CCoinsViewCache::Claim_SetCreditsClaimTip(const uint256 &hashCreditsClaimTipIn) {
	claim_hashCreditsClaimTip = hashCreditsClaimTipIn;
    return true;
}

int64_t Bitcoin_CCoinsViewCache::Claim_GetTotalClaimedCoins() {
    if (claim_totalClaimedCoins == int64_t(0))
    	claim_totalClaimedCoins = base->Claim_GetTotalClaimedCoins();
    return claim_totalClaimedCoins;
}

bool Bitcoin_CCoinsViewCache::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoinsIn) {
	claim_totalClaimedCoins = totalClaimedCoinsIn;
    return true;
}

bool Bitcoin_CCoinsViewCache::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &bitcoin_hashBlockIn, const uint256 &claim_hashBlockIn, const uint256 &claim_hashCreditsClaimTipIn, const int64_t &claim_totalClaimedCoinsIn) {
    for (std::map<uint256, Bitcoin_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	cacheCoins[it->first] = it->second;

    if(bitcoin_hashBlockIn != uint256(0)) {
    	bitcoin_hashBlock = bitcoin_hashBlockIn;
    }

    if(claim_hashBlockIn != uint256(0)) {
    	claim_hashBlock = claim_hashBlockIn;
    }
    if(claim_hashCreditsClaimTipIn != uint256(0)) {
    	claim_hashCreditsClaimTip = claim_hashCreditsClaimTipIn;
    }
    if(claim_totalClaimedCoinsIn != uint256(0)) {
    	claim_totalClaimedCoins = claim_totalClaimedCoinsIn;
    }
    return true;
}

bool Bitcoin_CCoinsViewCache::Flush() {
    bool  fOk = base->BatchWrite(cacheCoins, bitcoin_hashBlock, claim_hashBlock, claim_hashCreditsClaimTip, claim_totalClaimedCoins);
	if (fOk) {
    	cacheCoins.clear();
	}
    return fOk;
}

unsigned int Bitcoin_CCoinsViewCache::GetCacheSize() {
    return cacheCoins.size();
}

const Bitcoin_CTxOut &Bitcoin_CCoinsViewCache::GetOutputFor(const COutPoint& outPoint, int validationType){
    const Bitcoin_CCoins &coins = GetCoins(outPoint.hash);
    if(validationType == 1) {
    	assert(coins.Original_IsAvailable(outPoint.n));
    } else if(validationType == 2) {
    	assert(coins.Claim_IsAvailable(outPoint.n));
    } else {
    	assert(coins.IsAvailable(outPoint.n));
    }
    return coins.vout[outPoint.n];
}

int64_t Bitcoin_CCoinsViewCache::Bitcoin_GetValueIn(const Bitcoin_CTransaction& tx)
{
    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i].prevout, 1).nValueOriginal;

    return nResult;
}
void Bitcoin_CCoinsViewCache::Claim_GetValueIn(const Bitcoin_CTransaction& tx, ClaimSum& claimSum)
{
    if (tx.IsCoinBase())
        return;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Bitcoin_CTxOut & txout = GetOutputFor(tx.vin[i].prevout, 0);

		claimSum.nValueOriginalSum += txout.nValueOriginal;
		if(txout.nValueClaimable > 0) {
			claimSum.nValueClaimableSum += txout.nValueClaimable;
		}
    }

    return;
}
int64_t Bitcoin_CCoinsViewCache::Claim_GetValueIn(const Credits_CTransaction& tx) {
	assert(tx.IsClaim());

    int64_t nResult = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++)
		nResult += GetOutputFor(tx.vin[i].prevout, 2).nValueClaimable;

    return nResult;
}


bool Bitcoin_CCoinsViewCache::Bitcoin_HaveInputs(const Bitcoin_CTransaction& tx)
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
            if (!coins.Original_IsAvailable(prevout.n))
                return false;
        }
    }
    return true;
}
bool Bitcoin_CCoinsViewCache::Claim_HaveInputs(const Credits_CTransaction& tx) {
	assert(tx.IsClaim());

	// first check whether information about the prevout hash is available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		if (!HaveCoins(prevout.hash))
			return false;
	}

	// then check whether the actual outputs are available
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Credits_CTxIn &txIn = tx.vin[i];
		const COutPoint &prevout = txIn.prevout;
		const Bitcoin_CCoins &coins = GetCoins(prevout.hash);
		if (!coins.Claim_IsAvailable(prevout.n))
			return false;
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
        const Bitcoin_CCoins &coins = GetCoins(txin.prevout.hash);
        if (!coins.Original_IsAvailable(txin.prevout.n)) continue;
        if (coins.nHeight < nHeight) {
            dResult += coins.vout[txin.prevout.n].nValueOriginal * (nHeight-coins.nHeight);
        }
    }
    return tx.ComputePriority(dResult);
}
double Bitcoin_CCoinsViewCache::Claim_GetPriority(const Credits_CTransaction &tx, int nHeight) {
	assert(tx.IsClaim());

    double dResult = 0.0;
    BOOST_FOREACH(const Credits_CTxIn& txin, tx.vin)
    {
		const Bitcoin_CCoins &coins = GetCoins(txin.prevout.hash);
		if(!coins.Claim_IsAvailable(txin.prevout.n)) continue;
		if (coins.nHeight < nHeight) {
			dResult += coins.vout[txin.prevout.n].nValueClaimable * (nHeight-coins.nHeight);
		}
	}
    return tx.ComputePriority(dResult);
}
