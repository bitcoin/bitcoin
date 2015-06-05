// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_COINS_H
#define BITCOIN_COINS_H

#include "core.h"
#include "bitcoin_core.h"
#include "serialize.h"
#include "uint256.h"

#include <assert.h>
#include <stdint.h>

#include <boost/foreach.hpp>

/** pruned version of CTransaction: only retains metadata and unspent transaction outputs
 *
 * Serialized format:
 * - VARINT(nMetadata)
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
class Credits_CCoins
{
public:
    // whether transaction is a coinbase
    bool fCoinBase;

    // unspent transaction outputs; spent outputs are .IsNull(); spent outputs at the end of the array are dropped
    std::vector<CTxOut> vout;

    // at which height this transaction was included in the active block chain
    int nHeight;

    //For the moment this field only holds information regarding if the tx is deposit.
    int nMetaData;

    // version of the CTransaction; accesses to this value should probably check for nHeight as well,
    // as new tx version will probably only be introduced at certain heights
    int nVersion;

    // construct a Credits_CCoins from a CTransaction, at a given height
    Credits_CCoins(const Credits_CTransaction &tx, int nHeightIn) : fCoinBase(tx.IsCoinBase()), vout(tx.vout), nHeight(nHeightIn), nMetaData(tx.IsDeposit() ? 1 : 0), nVersion(tx.nVersion) {
        ClearUnspendable();
    }

    // empty constructor
    Credits_CCoins() : fCoinBase(false), vout(0), nHeight(0), nMetaData(0), nVersion(0) { }

    // remove spent outputs at the end of vout
    void Cleanup() {
        while (vout.size() > 0 && vout.back().IsNull())
            vout.pop_back();
        if (vout.empty())
            std::vector<CTxOut>().swap(vout);
    }

    void ClearUnspendable() {
        BOOST_FOREACH(CTxOut &txout, vout) {
            if (txout.scriptPubKey.IsUnspendable())
                txout.SetNull();
        }
        Cleanup();
    }

    void swap(Credits_CCoins &to) {
        std::swap(to.fCoinBase, fCoinBase);
        to.vout.swap(vout);
        std::swap(to.nHeight, nHeight);
        std::swap(to.nMetaData, nMetaData);
        std::swap(to.nVersion, nVersion);
    }

    // equality test
    friend bool operator==(const Credits_CCoins &a, const Credits_CCoins &b) {
         // Empty Credits_CCoins objects are always equal.
         if (a.IsPruned() && b.IsPruned())
             return true;
         return a.fCoinBase == b.fCoinBase &&
                a.nHeight == b.nHeight &&
                a.nMetaData == b.nMetaData &&
                a.nVersion == b.nVersion &&
                a.vout == b.vout;
    }
    friend bool operator!=(const Credits_CCoins &a, const Credits_CCoins &b) {
        return !(a == b);
    }

    void CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const;

    bool IsCoinBase() const {
        return fCoinBase;
    }
    bool IsDeposit() const {
        return nMetaData == 1;
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        unsigned int nSize = 0;
        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode);
        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // meta data
        nSize += ::GetSerializeSize(VARINT(this->nMetaData), nType, nVersion);
        // version
        nSize += ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion);
        // size of header code
        nSize += ::GetSerializeSize(VARINT(nCode), nType, nVersion);
        // spentness bitmask
        nSize += nMaskSize;
        // txouts themself
        for (unsigned int i = 0; i < vout.size(); i++)
            if (!vout[i].IsNull())
                nSize += ::GetSerializeSize(CTxOutCompressor(REF(vout[i])), nType, nVersion);
        // height
        nSize += ::GetSerializeSize(VARINT(nHeight), nType, nVersion);
        return nSize;
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode);
        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // metadata
        ::Serialize(s, VARINT(this->nMetaData), nType, nVersion);
        // version
        ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
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
                ::Serialize(s, CTxOutCompressor(REF(vout[i])), nType, nVersion);
        }
        // coinbase height
        ::Serialize(s, VARINT(nHeight), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        // meta data
        ::Unserialize(s, VARINT(this->nMetaData), nType, nVersion);
        // version
        ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
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
        vout.assign(vAvail.size(), CTxOut());
        for (unsigned int i = 0; i < vAvail.size(); i++) {
            if (vAvail[i])
                ::Unserialize(s, REF(CTxOutCompressor(vout[i])), nType, nVersion);
        }
        // coinbase height
        ::Unserialize(s, VARINT(nHeight), nType, nVersion);
        Cleanup();
    }

    // mark an outpoint spent, and construct undo information
    bool Spend(const COutPoint &out, Credits_CTxInUndo &undo);

    // mark a vout spent
    bool Spend(int nPos);

    // check whether a particular output is still available
    bool IsAvailable(unsigned int nPos) const {
        return (nPos < vout.size() && !vout[nPos].IsNull());
    }

    // check whether the entire Credits_CCoins is spent
    // note that only !IsPruned() Credits_CCoins can be serialized
    bool IsPruned() const {
        BOOST_FOREACH(const CTxOut &out, vout)
            if (!out.IsNull())
                return false;
        return true;
    }

    std::string ToString() const
    {
        std::string str;
        str += strprintf("Credits_CCoins(coinbase=%d, height=%d, metadata=%d, ver=%d, vout.size=%u)\n",
            fCoinBase,
            nHeight,
            nMetaData,
            nVersion,
            vout.size());
        for (unsigned int i = 0; i < vout.size(); i++)
            str += "    " + vout[i].ToString() + "\n";
        return str;
    }
};


struct Credits_CCoinsStats
{
    int nHeight;
    uint256 hashBlock;
    uint64_t nTransactions;
    uint64_t nTransactionOutputs;
    uint64_t nSerializedSize;
    uint256 hashSerialized;
    int64_t nTotalAmount;

    Credits_CCoinsStats() : nHeight(0), hashBlock(0), nTransactions(0), nTransactionOutputs(0), nSerializedSize(0), hashSerialized(0), nTotalAmount(0) {}
};


/** Abstract view on the open txout dataset. */
class Credits_CCoinsView
{
public:
    // Retrieve the Credits_CCoins (unspent transaction outputs) for a given txid
    virtual bool Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins);

    // Modify the Credits_CCoins for a given txid
    virtual bool Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins);

    // Just check whether we have data for a given txid.
    // This may (but cannot always) return true for fully spent transactions
    virtual bool Credits_HaveCoins(const uint256 &txid);

    // Retrieve the block hash whose state this Credits_CCoinsView currently represents
    virtual uint256 Credits_GetBestBlock();

    // Modify the currently active block hash
    virtual bool Credits_SetBestBlock(const uint256 &hashBlock);

    // Do a bulk modification (multiple SetCoins + one SetBestBlock)
    virtual bool Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock);

    // Calculate statistics about the unspent transaction output set
    virtual bool Credits_GetStats(Credits_CCoinsStats &stats);

    // As we use Credits_CCoinsViews polymorphically, have a virtual destructor
    virtual ~Credits_CCoinsView() {}
};


/** Credits_CCoinsView backed by another Credits_CCoinsView */
class Credits_CCoinsViewBacked : public Credits_CCoinsView
{
protected:
    Credits_CCoinsView *base;

public:
    Credits_CCoinsViewBacked(Credits_CCoinsView &viewIn);
    bool Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins);
    bool Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins);
    bool Credits_HaveCoins(const uint256 &txid);
    uint256 Credits_GetBestBlock();
    bool Credits_SetBestBlock(const uint256 &hashBlock);
    void Credits_SetBackend(Credits_CCoinsView &viewIn);
    bool Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock);
    bool Credits_GetStats(Credits_CCoinsStats &stats);
};


/** Credits_CCoinsView that adds a memory cache for transactions to another Credits_CCoinsView */
class Credits_CCoinsViewCache : public Credits_CCoinsViewBacked
{
protected:
    uint256 credits_hashBlock;
    std::map<uint256,Credits_CCoins> credits_cacheCoins;

public:
    Credits_CCoinsViewCache(Credits_CCoinsView &baseIn, bool fDummy = false);

    // Standard Credits_CCoinsView methods
    bool Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins);
    bool Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins);
    bool Credits_HaveCoins(const uint256 &txid);
    uint256 Credits_GetBestBlock();
    bool Credits_SetBestBlock(const uint256 &hashBlock);
    bool Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock);

    // Return a modifiable reference to a Credits_CCoins. Check HaveCoins first.
    // Many methods explicitly require a Credits_CCoinsViewCache because of this method, to reduce
    // copying.
    Credits_CCoins &Credits_GetCoins(const uint256 &txid);

    // Push the modifications applied to this cache to its base.
    // Failure to call this method before destruction will cause the changes to be forgotten.
    bool Credits_Flush();

    // Calculate the size of the cache (in number of transactions)
    unsigned int GetCacheSize();

    /** Amount of bitcoins coming in to a transaction
        Note that lightweight clients may not know anything besides the hash of previous transactions,
        so may not be able to calculate this.

        @param[in] tx	transaction for which we are checking input total
        @return	Sum of value of all inputs (scriptSigs)
     */
    int64_t Credits_GetValueIn(const Credits_CTransaction& tx);

    // Check whether all prevouts of the transaction are present in the UTXO set represented by this view
    bool Credits_HaveInputs(const Credits_CTransaction& tx);

    // Return priority of tx at height nHeight
    double Credits_GetPriority(const Credits_CTransaction &tx, int nHeight);

    const CTxOut &Credits_GetOutputFor(const Credits_CTxIn& input);

private:
    std::map<uint256,Credits_CCoins>::iterator Credits_FetchCoins(const uint256 &txid);
};

#endif
