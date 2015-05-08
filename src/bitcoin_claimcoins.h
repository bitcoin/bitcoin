// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BITCOIN_CLAIMCOINS_H
#define BITCOIN_BITCOIN_CLAIMCOINS_H

#include "bitcoin_core.h"
#include "serialize.h"
#include "uint256.h"

#include <assert.h>
#include <stdint.h>

#include <boost/foreach.hpp>

/** pruned version of CTransaction: only retains metadata and unspent transaction outputs
 *
 * Serialized format:
 * - VARINT(nVersion)
 * - VARINT(nCode)
 * - unspentness bitvector, for vout[2] and further; least significant byte first
 * - the non-spent CTxOuts (via CTxOutCompressor)
 * - VARINT(nHeight)
 *
 * The nCode value consists of:
 * - bit 1: IsCoinBase()
 * - bit 2: vout[0] is not spent
 * - bit 4: vout[1] is not spent
 * - The higher bits encode N, the number of non-zero bytes in the following bitvector.
 *   - In case both bit 2 and bit 4 are unset, they encode N-1, as there must be at
 *     least one non-spent output).
 *
 * Example: 0104835800816115944e077fe7c803cfa57f29b36bf87c1d358bb85e
 *          <><><--------------------------------------------><---->
 *          |  \                  |                             /
 *    version   code             vout[1]                  height
 *
 *    - version = 1
 *    - code = 4 (vout[1] is not spent, and 0 non-zero bytes of bitvector follow)
 *    - unspentness bitvector: as 0 non-zero bytes follow, it has length 0
 *    - vout[1]: 835800816115944e077fe7c803cfa57f29b36bf87c1d35
 *               * 8358: compact amount representation for 60000000000 (600 CRE)
 *               * 00: special txout type pay-to-pubkey-hash
 *               * 816115944e077fe7c803cfa57f29b36bf87c1d35: address uint160
 *    - height = 203998
 *
 *
 * Example: 0109044086ef97d5790061b01caab50f1b8e9c50a5057eb43c2d9563a4eebbd123008c988f1a4a4de2161e0f50aac7f17e7f9555caa486af3b
 *          <><><--><--------------------------------------------------><----------------------------------------------><---->
 *         /  \   \                     |                                                           |                     /
 *  version  code  unspentness       vout[4]                                                     vout[16]           height
 *
 *  - version = 1
 *  - code = 9 (coinbase, neither vout[0] or vout[1] are unspent,
 *                2 (1, +1 because both bit 2 and bit 4 are unset) non-zero bitvector bytes follow)
 *  - unspentness bitvector: bits 2 (0x04) and 14 (0x4000) are set, so vout[2+2] and vout[14+2] are unspent
 *  - vout[4]: 86ef97d5790061b01caab50f1b8e9c50a5057eb43c2d9563a4ee
 *             * 86ef97d579: compact amount representation for 234925952 (2.35 CRE)
 *             * 00: special txout type pay-to-pubkey-hash
 *             * 61b01caab50f1b8e9c50a5057eb43c2d9563a4ee: address uint160
 *  - vout[16]: bbd123008c988f1a4a4de2161e0f50aac7f17e7f9555caa4
 *              * bbd123: compact amount representation for 110397 (0.001 CRE)
 *              * 00: special txout type pay-to-pubkey-hash
 *              * 8c988f1a4a4de2161e0f50aac7f17e7f9555caa4: address uint160
 *  - height = 120891
 */
class Bitcoin_CClaimCoins
{
public:
    // whether transaction is a coinbase
    bool fCoinBase;

    // unspent transaction outputs; spent outputs are .IsNull(); spent outputs at the end of the array are dropped
   std::vector<CTxOutClaim> vout;

    // at which height this transaction was included in the active block chain
    int nHeight;

    // version of the CTransaction; accesses to this value should probably check for nHeight as well,
    // as new tx version will probably only be introduced at certain heights
    int nVersion;

    //TODO - test this fractional calculation.
    // construct a Bitcoin_CClaimCoins from a CTransaction, at a given height
    Bitcoin_CClaimCoins(const Bitcoin_CTransaction &tx, int nHeightIn, const ClaimSum& claimSum) : fCoinBase(tx.IsCoinBase()), nHeight(nHeightIn), nVersion(tx.nVersion) {

		if(fCoinBase) {
			const int64_t nTotalReduceFees = claimSum.nValueOriginalSum - claimSum.nValueClaimableSum;
			const int64_t nTotalValueOut = tx.GetValueOut();
			BOOST_FOREACH(const CTxOut & txout, tx.vout) {
				const int64_t nValue = txout.nValue;
				const int64_t nFractionReducedFee = ReduceByFraction(nTotalReduceFees, nValue, nTotalValueOut);
				int64_t nValueClaimable = 0;
				if(nValue > nFractionReducedFee) {
					nValueClaimable = nValue - nFractionReducedFee;
				}
				assert(nValueClaimable >= 0 && nValueClaimable <= nValue);

				vout.push_back(CTxOutClaim(nValue, nValueClaimable, txout.scriptPubKey));
			}
		} else {
			BOOST_FOREACH(const CTxOut & txout, tx.vout) {
				const int64_t nValue = txout.nValue;
				const int64_t nValueClaimable = ReduceByFraction(nValue, claimSum.nValueClaimableSum, claimSum.nValueOriginalSum);
				assert(nValueClaimable >= 0 && nValueClaimable <= nValue);

				vout.push_back(CTxOutClaim(nValue, nValueClaimable, txout.scriptPubKey));
			}
		}

    	ClearUnspendable();
    }

    //NOTE! Only use this constructor with caution and make sure that you understand wath it is doing.
    //The copying of nValueOriginal to nValueClaimable could cause trouble.
    //There are only two use cases for this constructor:
    //1. To create an object that can be compared with equalsExcludingClaimable
    //2. In Bitcoin block connect and disconnect when fast forwarding the claim chainstate
    Bitcoin_CClaimCoins(const Bitcoin_CTransaction &tx, int nHeightIn) : fCoinBase(tx.IsCoinBase()), nHeight(nHeightIn), nVersion(tx.nVersion) {
    	BOOST_FOREACH(const CTxOut & txout, tx.vout) {
    		vout.push_back(CTxOutClaim(txout.nValue, txout.nValue, txout.scriptPubKey));
    	}

    	ClearUnspendable();
    }
    bool equalsExcludingClaimable(const Bitcoin_CClaimCoins &b) {
         // Empty Bitcoin_CClaimCoins objects are always equal.
         if (IsPruned() && b.IsPruned()) {
             return true;
         }

         if (fCoinBase != b.fCoinBase ||
                nHeight != b.nHeight ||
                nVersion != b.nVersion) {
        	 return false;
         }

         if(vout.size() != b.vout.size()) {
        	 return false;
         }

     	for (unsigned int i = 0; i < vout.size(); i++) {
     		const CTxOutClaim &txout = vout[i];
     		const CTxOutClaim &btxout = b.vout[i];
             if (txout.nValueOriginal != btxout.nValueOriginal ||
            	txout.scriptPubKey != btxout.scriptPubKey) {
                 return false;
             }
     	}
     	return true;
    }

    // empty constructor
    Bitcoin_CClaimCoins() : fCoinBase(false), vout(0), nHeight(0), nVersion(0) { }

    // remove spent outputs at the end of vout
    void Cleanup() {
        while (vout.size() > 0 && vout.back().IsNull())
        	vout.pop_back();
        if (vout.empty())
            std::vector<CTxOutClaim>().swap(vout);
    }

    void ClearUnspendable() {
    	for (unsigned int i = 0; i < vout.size(); i++) {
    		CTxOutClaim &txout = vout[i];
            if (txout.scriptPubKey.IsUnspendable()) {
                txout.SetNull();
                vout[i].SetNull();
            }
    	}
        Cleanup();
    }

    void swap(Bitcoin_CClaimCoins &to) {
        std::swap(to.fCoinBase, fCoinBase);
        to.vout.swap(vout);
        std::swap(to.nHeight, nHeight);
        std::swap(to.nVersion, nVersion);
    }

    void CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes, const std::vector<CTxOutClaim>& vouts) const;

    bool IsCoinBase() const {
        return fCoinBase;
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        unsigned int nSize = 0;
        // version
        nSize += ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion);

        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode, vout);

        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // size of header code
        nSize += ::GetSerializeSize(VARINT(nCode), nType, nVersion);
        // spentness bitmask
        nSize += nMaskSize;
        // txouts themself
        for (unsigned int i = 0; i < vout.size(); i++)
            if (!vout[i].IsNull())
                nSize += ::GetSerializeSize(CTxOutClaimCompressor(REF(vout[i])), nType, nVersion);

        // height
        nSize += ::GetSerializeSize(VARINT(nHeight), nType, nVersion);
        return nSize;
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        // version
        ::Serialize(s, VARINT(this->nVersion), nType, nVersion);

        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode, vout);

        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // header code
        ::Serialize(s, VARINT(nCode), nType, nVersion);
        // spentness bitmask
        for (unsigned int b = 0; b<nMaskSize; b++) {
            unsigned char chAvail = 0;
            for (unsigned int i = 0; i < 8 && 2+b*8+i < vout.size(); i++)
                if (!vout[2+b*8+i].IsNull())
                    chAvail |= (1 << i);
            ::Serialize(s, chAvail, nType, nVersion);
        }
        // txouts themself
        for (unsigned int i = 0; i < vout.size(); i++) {
            if (!vout[i].IsNull())
                ::Serialize(s, CTxOutClaimCompressor(REF(vout[i])), nType, nVersion);
        }

        // coinbase height
        ::Serialize(s, VARINT(nHeight), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        // version
        ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);

        unsigned int nCode = 0;
        // header code
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        fCoinBase = nCode & 1;
        std::vector<bool> vAvail(2, false);
        vAvail[0] = nCode & 2;
        vAvail[1] = nCode & 4;
        unsigned int nMaskCode = (nCode / 8) + ((nCode & 6) != 0 ? 0 : 1);
        // spentness bitmask
        while (nMaskCode > 0) {
            unsigned char chAvail = 0;
            ::Unserialize(s, chAvail, nType, nVersion);
            for (unsigned int p = 0; p < 8; p++) {
                bool f = (chAvail & (1 << p)) != 0;
                vAvail.push_back(f);
            }
            if (chAvail != 0)
                nMaskCode--;
        }
        // txouts themself
        vout.assign(vAvail.size(), CTxOutClaim());
        for (unsigned int i = 0; i < vAvail.size(); i++) {
            if (vAvail[i])
                ::Unserialize(s, REF(CTxOutClaimCompressor(vout[i])), nType, nVersion);
        }

        // coinbase height
        ::Unserialize(s, VARINT(nHeight), nType, nVersion);

        Cleanup();
    }

    // mark an outpoint spent, and construct undo information
    bool Spend(const COutPoint &out, Bitcoin_CTxInUndoClaim &undo);
    bool SpendByClaiming(const COutPoint &out, Credits_CTxInUndo &undo);

    // check whether a particular output is still available
    bool IsAvailable(unsigned int nPos) const {
        return (nPos < vout.size() && !vout[nPos].IsNull());
    }
    // check whether there are any bitcredits left to claim
    bool HasClaimable(unsigned int nPos) const {
        return IsAvailable(nPos) && vout[nPos].nValueClaimable > 0;
    }

    // check whether the entire Bitcoin_CClaimCoins is spent
    // note that only !IsPruned() Bitcoin_CClaimCoins can be serialized
    bool IsPruned() const {
        BOOST_FOREACH(const CTxOutClaim &out, vout)
            if (!out.IsNull())
                return false;
        return true;
    }

    std::string ToString() const;
    void print() const;
};


struct Bitcoin_CClaimCoinsStats
{
    int nHeight;
    uint256 hashBlock;
    uint256 hashBitcreditClaimTip;
    int64_t totalClaimedCoins;
    uint64_t nTransactions;
    uint64_t nTransactionOutputsOriginal;
    uint64_t nTransactionOutputsClaimable;
    uint64_t nSerializedSize;
    uint256 hashSerialized;
    int64_t nTotalAmountOriginal;
    int64_t nTotalAmountClaimable;

    Bitcoin_CClaimCoinsStats() : nHeight(0), hashBlock(0), hashBitcreditClaimTip(0), totalClaimedCoins(0), nTransactions(0), nTransactionOutputsOriginal(0), nTransactionOutputsClaimable(0), nSerializedSize(0), hashSerialized(0), nTotalAmountOriginal(0), nTotalAmountClaimable(0){}
};


/** Abstract view on the open txout dataset. */
class Bitcoin_CClaimCoinsView
{
public:
    // Retrieve the Bitcoin_CClaimCoins (unspent transaction outputs) for a given txid
    virtual bool GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins);

    // Modify the Bitcoin_CClaimCoins for a given txid
    virtual bool SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins);

    // Just check whether we have data for a given txid.
    // This may (but cannot always) return true for fully spent transactions
    virtual bool HaveCoins(const uint256 &txid);

    // Retrieve the block hash whose state this Bitcoin_CClaimCoinsView currently represents
    virtual uint256 GetBestBlock();

    // Modify the currently active block hash
    virtual bool SetBestBlock(const uint256 &hashBlock);

    // Retrieve the block hash whose state this Bitcoin_CClaimCoinsView currently represents
    virtual uint256 GetBitcreditClaimTip();

    // Modify the currently active block hash
    virtual bool SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip);

    // Get the total (sum) number of bitcoins that have been claimed
    virtual int64_t GetTotalClaimedCoins();

    // Modify the currently total claimed coins value
    virtual bool SetTotalClaimedCoins(const int64_t &totalClaimedCoins);

    // Do a bulk modification (multiple SetCoins + one SetBestBlock)
    virtual bool BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);

    // Used to do move data from one cache or db and another in slices
    virtual bool GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore);

    // Calculate statistics about the unspent transaction output set
    virtual bool GetStats(Bitcoin_CClaimCoinsStats &stats);

    // As we use Bitcoin_CClaimCoinsViews polymorphically, have a virtual destructor
    virtual ~Bitcoin_CClaimCoinsView() {}
};


/** Bitcoin_CClaimCoinsView backed by another Bitcoin_CClaimCoinsView */
class Bitcoin_CClaimCoinsViewBacked : public Bitcoin_CClaimCoinsView
{
protected:
    Bitcoin_CClaimCoinsView *base;
	Bitcoin_CClaimCoinsView *tmpDb;

public:
    Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn);
    Bitcoin_CClaimCoinsViewBacked(Bitcoin_CClaimCoinsView &viewIn, Bitcoin_CClaimCoinsView& tmpDbIn);
    bool GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins);
    bool SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins);
    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    uint256 GetBitcreditClaimTip();
    bool SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip);
    int64_t GetTotalClaimedCoins();
    bool SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    void SetBackend(Bitcoin_CClaimCoinsView &viewIn);
    bool BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore);
    bool GetStats(Bitcoin_CClaimCoinsStats &stats);
};


/** Bitcoin_CClaimCoinsView that adds a memory cache for transactions to another Bitcoin_CClaimCoinsView */
class Bitcoin_CClaimCoinsViewCache : public Bitcoin_CClaimCoinsViewBacked
{
protected:
    unsigned int coinsCacheSize;
    uint256 hashBlock;
    uint256 hashBitcreditClaimTip;
    int64_t totalClaimedCoins;
    std::map<uint256,Bitcoin_CClaimCoins> cacheCoins;


public:
    Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, unsigned int coinsCacheSizeIn, bool fDummy = false);
    Bitcoin_CClaimCoinsViewCache(Bitcoin_CClaimCoinsView &baseIn, Bitcoin_CClaimCoinsView& tmpDbIn, unsigned int coinsCacheSizeIn, bool fDummy = false);

    // Standard Bitcoin_CClaimCoinsView methods
    bool GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins);
    bool SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins);
    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    uint256 GetBitcreditClaimTip();
    bool SetBitcreditClaimTip(const uint256 &hashBitcreditClaimTip);
    int64_t GetTotalClaimedCoins();
    bool SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    bool BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore);

    // Return a modifiable reference to a Bitcoin_CClaimCoins. Check HaveCoins first.
    // Many methods explicitly require a Bitcoin_CClaimCoinsViewCache because of this method, to reduce
    // copying.
    Bitcoin_CClaimCoins &GetCoins(const uint256 &txid);

    // Push the modifications applied to this cache to its base.
    // Failure to call this method before destruction will cause the changes to be forgotten.
    bool Flush();

    // Calculate the size of the cache (in number of transactions)
    unsigned int GetCacheSize();

    /** Amount of bitcoins coming in to a transaction
        Note that lightweight clients may not know anything besides the hash of previous transactions,
        so may not be able to calculate this.

        @param[in] tx	transaction for which we are checking input total
        @return	Sum of value of all inputs (scriptSigs)
     */
    void GetValueIn(const Bitcoin_CTransaction& tx, ClaimSum& claimSum);
    int64_t GetValueIn(const Credits_CTransaction& tx);

    // Check whether all prevouts of the transaction are present in the UTXO set represented by this view
    bool HaveInputs(const Credits_CTransaction& tx);

    // Return pvoutty of tx at height nHeight
    double GetPriority(const Credits_CTransaction &tx, int nHeight);

    const CScript &GetOutputScriptFor(const Credits_CTxIn& input);

private:
	//If we are over cache coins limit, write everything in cache down to lower levels
    void ClearCacheIfNeeded(bool fForce);
    std::map<uint256,Bitcoin_CClaimCoins>::iterator FetchCoins(const uint256 &txid);
    const CTxOutClaim& GetOut(const COutPoint &outpoint);
};

#endif
