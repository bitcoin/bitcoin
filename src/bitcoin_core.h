// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_CORE_H
#define BITCOIN_BITCOIN_CORE_H

#include "bitcoin_script.h"
#include "serialize.h"
#include "uint256.h"

#include "core.h"

#include <stdint.h>

class Bitcoin_CTransaction;

/** An inpoint - a combination of a transaction and an index n into its vin */
class Bitcoin_CInPoint
{
public:
    const Bitcoin_CTransaction* ptx;
    unsigned int n;

    Bitcoin_CInPoint() { SetNull(); }
    Bitcoin_CInPoint(const Bitcoin_CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
    void SetNull() { ptx = NULL; n = (unsigned int) -1; }
    bool IsNull() const { return (ptx == NULL && n == (unsigned int) -1); }
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class Bitcoin_CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    unsigned int nSequence;

    Bitcoin_CTxIn()
    {
        nSequence = std::numeric_limits<unsigned int>::max();
    }

    explicit Bitcoin_CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max());
    Bitcoin_CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max());

    IMPLEMENT_SERIALIZE
    (
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    )

    bool IsFinal() const
    {
        return (nSequence == std::numeric_limits<unsigned int>::max());
    }

    friend bool operator==(const Bitcoin_CTxIn& a, const Bitcoin_CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const Bitcoin_CTxIn& a, const Bitcoin_CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class Bitcoin_CTxOut
{
public:
    int64_t nValueOriginal;
    int64_t nValueClaimable;
    CScript scriptPubKey;
    int nValueOriginalHasBeenSpent;

    Bitcoin_CTxOut()
    {
        SetNull();
    }

    Bitcoin_CTxOut(int64_t nValueOriginalIn, int64_t nValueClaimableIn, CScript scriptPubKeyIn, int nValueOriginalHasBeenSpentIn);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValueOriginal);
        READWRITE(nValueClaimable);
        READWRITE(scriptPubKey);
        READWRITE(nValueOriginalHasBeenSpent);

        assert_with_stacktrace(Bitcoin_MoneyRange(nValueOriginal), strprintf("Bitcoin_CTxOut() : valueOriginal out of range: %d", nValueOriginal));
        assert_with_stacktrace(Bitcoin_MoneyRange(nValueClaimable), strprintf("Bitcoin_CTxOut() : valueClaimable out of range: %d", nValueClaimable));
        assert_with_stacktrace(nValueOriginal >= nValueClaimable, strprintf("Bitcoin_CTxOut() : valueOriginal less than valueClaimable: %d:%d", nValueOriginal, nValueClaimable));
    )

    void SetNull()
    {
        nValueOriginal = -1;
        nValueClaimable = -1;
        scriptPubKey.clear();
        nValueOriginalHasBeenSpent = -1;
    }
    void SetOriginalSpent(int nValueOriginalHasBeenSpentIn)
    {
        nValueOriginalHasBeenSpent = nValueOriginalHasBeenSpentIn;
    }

    bool IsNull() const
    {
        return nValueOriginal == -1;
    }
    bool IsOriginalSpent() const
    {
        return nValueOriginalHasBeenSpent == 1;
    }

    uint256 GetHash() const;

    bool IsDust(int64_t nMinRelayTxFee) const
    {
        // "Dust" is defined in terms of Credits_CTransaction::nMinRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend,
        // so dust is a txout less than 546 satoshis
        // with default nMinRelayTxFee.
        return ((nValueOriginal*1000)/(3*((int)GetSerializeSize(SER_DISK,0)+148)) < nMinRelayTxFee);
    }

    friend bool operator==(const Bitcoin_CTxOut& a, const Bitcoin_CTxOut& b)
    {
        return (a.nValueOriginal       == b.nValueOriginal &&
        		a.nValueClaimable       == b.nValueClaimable &&
                a.scriptPubKey == b.scriptPubKey &&
                a.nValueOriginalHasBeenSpent == b.nValueOriginalHasBeenSpent);
    }

    friend bool operator!=(const Bitcoin_CTxOut& a, const Bitcoin_CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/*
 * Stores the two different levels of values before spending.
 * These two are used to calculate the difference between the
 * original and the claimable state
 */
class ClaimSum
{
public:
    int64_t nValueOriginalSum;
    int64_t nValueClaimableSum;

    ClaimSum()
    {
        nValueOriginalSum = 0;
        nValueClaimableSum = 0;
    }

    void Validate() {
    	assert_with_stacktrace(Bitcoin_MoneyRange(nValueOriginalSum), strprintf("ClaimSum() : nOriginal out of range: %d", nValueOriginalSum));
    	assert_with_stacktrace(Bitcoin_MoneyRange(nValueClaimableSum), strprintf("ClaimSum() : nClaimable out of range: %d", nValueClaimableSum));

    	assert(nValueOriginalSum >= nValueClaimableSum);
    }
};

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class Bitcoin_CTransaction
{
public:
    static int64_t nMinTxFee;
    static int64_t nMinRelayTxFee;
    static const int CURRENT_VERSION=1;
    int nVersion;
    std::vector<Bitcoin_CTxIn> vin;
    std::vector<CTxOut> vout;
    unsigned int nLockTime;

    Bitcoin_CTransaction()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
    )

    void SetNull()
    {
        nVersion = Bitcoin_CTransaction::CURRENT_VERSION;
        vin.clear();
        vout.clear();
        nLockTime = 0;
    }

    bool IsNull() const
    {
        return (vin.empty() && vout.empty());
    }

    uint256 GetHash() const;

    // Return sum of txouts.
    int64_t GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    //If all outputs are unspendable, the corresponding coin will not be found in chainstate
    bool HasSpendable() const;

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const Bitcoin_CTransaction& a, const Bitcoin_CTransaction& b)
    {
        return (a.nVersion  == b.nVersion &&
                a.vin       == b.vin &&
                a.vout      == b.vout &&
                a.nLockTime == b.nLockTime);
    }

    friend bool operator!=(const Bitcoin_CTransaction& a, const Bitcoin_CTransaction& b)
    {
        return !(a == b);
    }


    std::string ToString() const;
    void print() const;
};

/** wrapper for CTxOut that provides a more compact serialization */
class Bitcoin_CTxOutCompressor
{
private:
    Bitcoin_CTxOut &txout;

public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);

    Bitcoin_CTxOutCompressor(Bitcoin_CTxOut &txoutIn) : txout(txoutIn) { }

    IMPLEMENT_SERIALIZE(({
        if (!fRead) {
            uint64_t nValOriginal = CompressAmount(txout.nValueOriginal);
            READWRITE(VARINT(nValOriginal));
            uint64_t nValClaimable = CompressAmount(txout.nValueClaimable);
            READWRITE(VARINT(nValClaimable));
        } else {
            uint64_t nValOriginal = 0;
            READWRITE(VARINT(nValOriginal));
            txout.nValueOriginal = DecompressAmount(nValOriginal);
            uint64_t nValClaimable = 0;
            READWRITE(VARINT(nValClaimable));
            txout.nValueClaimable = DecompressAmount(nValClaimable);
        }
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
        READWRITE(VARINT(txout.nValueOriginalHasBeenSpent));
    });)
};


/** Undo information for a CTxIn
 *
 *  Contains the prevout's CTxOut being spent, and if this was the
 *  last output of the affected transaction, its metadata as well
 *  (coinbase or not, height, transaction version)
 */
class Bitcoin_CTxInUndo
{
public:
    Bitcoin_CTxOut txout;         // the txout data before being spent
    bool fCoinBase;       // if the outpoint was the last unspent: whether it belonged to a coinbase
    unsigned int nHeight; // if the outpoint was the last unspent: its height
    int nVersion;         // if the outpoint was the last unspent: its version

    Bitcoin_CTxInUndo() : txout(), fCoinBase(false), nHeight(0), nVersion(0) {}
    Bitcoin_CTxInUndo(const Bitcoin_CTxOut &txoutIn, bool fCoinBaseIn = false, unsigned int nHeightIn = 0, int nVersionIn = 0) : txout(txoutIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn), nVersion(nVersionIn) { }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        return ::GetSerializeSize(VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion) : 0) +
               ::GetSerializeSize(Bitcoin_CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        ::Serialize(s, VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion);
        if (nHeight > 0)
            ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Serialize(s, Bitcoin_CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        nHeight = nCode / 2;
        fCoinBase = nCode & 1;
        if (nHeight > 0)
            ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Unserialize(s, REF(Bitcoin_CTxOutCompressor(REF(txout))), nType, nVersion);
    }
};

/** Undo information for a Bitcoin_CTransaction */
class Bitcoin_CTxUndo
{
public:
    // undo information for all txins
    std::vector<Bitcoin_CTxInUndo> vprevout;

    IMPLEMENT_SERIALIZE(
        READWRITE(vprevout);
    )
};


/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class Bitcoin_CBlockHeader
{
public:
    // header
    static const int CURRENT_VERSION=2;
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;

    Bitcoin_CBlockHeader()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    )

    void SetNull()
    {
        nVersion = Bitcoin_CBlockHeader::CURRENT_VERSION;
        hashPrevBlock = 0;
        hashMerkleRoot = 0;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class Bitcoin_CBlock : public Bitcoin_CBlockHeader
{
public:
    // network and disk
    std::vector<Bitcoin_CTransaction> vtx;

    // memory only
    mutable std::vector<uint256> vMerkleTree;

    Bitcoin_CBlock()
    {
        SetNull();
    }

    Bitcoin_CBlock(const Bitcoin_CBlockHeader &header)
    {
        SetNull();
        *((Bitcoin_CBlockHeader*)this) = header;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(*(Bitcoin_CBlockHeader*)this);
        READWRITE(vtx);
    )

    void SetNull()
    {
        Bitcoin_CBlockHeader::SetNull();
        vtx.clear();
        vMerkleTree.clear();
    }

    Bitcoin_CBlockHeader GetBlockHeader() const
    {
        Bitcoin_CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    uint256 BuildMerkleTree() const;

    const uint256 &GetTxHash(unsigned int nIndex) const {
        assert(vMerkleTree.size() > 0); // BuildMerkleTree must have been called first
        assert(nIndex < vtx.size());
        return vMerkleTree[nIndex];
    }

    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    void print() const;
};

#endif
