// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_H
#define BITCOIN_CORE_H

#include "script.h"
#include "serialize.h"
#include "uint256.h"

#include "keystore.h"

#include <stdint.h>

class Credits_CTransaction;

/** No amount larger than this (in satoshi) is valid */
static const int64_t BITCOIN_MAX_MONEY = 21000000 * COIN;
static const int64_t BITCREDIT_MAX_MONEY = 30000000 * COIN;
inline bool Credits_MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= BITCREDIT_MAX_MONEY); }
inline bool Bitcoin_MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= BITCOIN_MAX_MONEY); }

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    unsigned int n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { hash = 0; n = (unsigned int) -1; }
    bool IsNull() const { return (hash == 0 && n == (unsigned int) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** An inpoint - a combination of a transaction and an index n into its vin */
class Credits_CInPoint
{
public:
    const Credits_CTransaction* ptx;
    unsigned int n;

    Credits_CInPoint() { SetNull(); }
    Credits_CInPoint(const Credits_CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
    void SetNull() { ptx = NULL; n = (unsigned int) -1; }
    bool IsNull() const { return (ptx == NULL && n == (unsigned int) -1); }
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class Credits_CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;

    Credits_CTxIn() {}

    explicit Credits_CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript());
    Credits_CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript());

    IMPLEMENT_SERIALIZE
    (
        READWRITE(prevout);
        READWRITE(scriptSig);
    )

    friend bool operator==(const Credits_CTxIn& a, const Credits_CTxIn& b)
    {
        return (
        		a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig);
    }

    friend bool operator!=(const Credits_CTxIn& a, const Credits_CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};




/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    int64_t nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(int64_t nValueIn, CScript scriptPubKeyIn);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    )

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
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
        return ((nValue*1000)/(3*((int)GetSerializeSize(SER_DISK,0)+148)) < nMinRelayTxFee);
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOutClaim
{
public:
    int64_t nValueOriginal;
    int64_t nValueClaimable;
    CScript scriptPubKey;
    int nValueOriginalHasBeenSpent;

    CTxOutClaim()
    {
        SetNull();
    }

    CTxOutClaim(int64_t nValueOriginalIn, int64_t nValueClaimableIn, CScript scriptPubKeyIn, int nValueOriginalHasBeenSpentIn);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValueOriginal);
        READWRITE(nValueClaimable);
        READWRITE(scriptPubKey);
        READWRITE(nValueOriginalHasBeenSpent);

        assert_with_stacktrace(Bitcoin_MoneyRange(nValueOriginal), strprintf("CTxOutClaim() : valueOriginal out of range: %d", nValueOriginal));
        assert_with_stacktrace(Bitcoin_MoneyRange(nValueClaimable), strprintf("CTxOutClaim() : valueClaimable out of range: %d", nValueClaimable));
        assert_with_stacktrace(nValueOriginal >= nValueClaimable, strprintf("CTxOutClaim() : valueOriginal less than valueClaimable: %d:%d", nValueOriginal, nValueClaimable));
    )

    void SetNull()
    {
        nValueOriginal = -1;
        nValueClaimable = -1;
        scriptPubKey.clear();
        nValueOriginalHasBeenSpent = 0;
    }

    bool IsNull() const
    {
        return (nValueOriginal == -1);
    }

    uint256 GetHash() const;

    friend bool operator==(const CTxOutClaim& a, const CTxOutClaim& b)
    {
        return (a.nValueOriginal       == b.nValueOriginal &&
        		a.nValueClaimable       == b.nValueClaimable &&
                a.scriptPubKey == b.scriptPubKey &&
                a.nValueOriginalHasBeenSpent == b.nValueOriginalHasBeenSpent);
    }

    friend bool operator!=(const CTxOutClaim& a, const CTxOutClaim& b)
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


/** Tx types */
enum
{
    TX_TYPE_STANDARD = 1,
    TX_TYPE_COINBASE = 2,
    TX_TYPE_DEPOSIT = 3,
    TX_TYPE_EXTERNAL_BITCOIN = 4,
    TX_TYPE_SIGNATURE_VERIFIED = 5,
    TX_TYPE_COLORED = 6,
};
/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class Credits_CTransaction
{
public:
    static int64_t nMinTxFee;
    static int64_t nMinRelayTxFee;
    static const int CURRENT_VERSION=1;
    int nVersion;
	unsigned int nTxType;
    std::vector<Credits_CTxIn> vin;
    std::vector<CTxOut> vout;
    CKeyID signingKeyId;
    unsigned int nLockTime;

    Credits_CTransaction(unsigned char nTypeIn)
    {
        SetNull();
        nTxType = nTypeIn;
    }
    Credits_CTransaction()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nTxType);
        READWRITE(vin);
        READWRITE(vout);
        if(IsDeposit()) {
        	READWRITE(signingKeyId);
        	assert(signingKeyId != CKeyID(0));
        }
        READWRITE(nLockTime);
    )

    void SetNull()
    {
        nVersion = Credits_CTransaction::CURRENT_VERSION;
    	nTxType = TX_TYPE_STANDARD;
        vin.clear();
        vout.clear();
        signingKeyId = CKeyID();
        nLockTime = 0;
    }

    bool IsNull() const
    {
        return (vin.empty() && vout.empty());
    }

    uint256 GetHash() const;

    // Return sum of txouts.
    int64_t GetValueOut() const;
    int64_t GetDepositValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    bool IsCoinBase() const
    {
        return nTxType == TX_TYPE_COINBASE;
    }
    bool IsValidCoinBase() const
    {
        return IsCoinBase() && vin.size() == 1 && vin[0].prevout.hash != 0 && vout.size() > 0 && vout.size() <= 11;
    }
    bool IsDeposit() const
    {
        return nTxType == TX_TYPE_DEPOSIT;
    }
    bool IsValidDeposit() const
    {
        return IsDeposit() && vin.size() == 1 && (vout.size() == 1 || vout.size() == 2);
    }

    bool IsClaim() const
    {
        return nTxType == TX_TYPE_EXTERNAL_BITCOIN;
    }
    bool IsStandard() const
    {
        return nTxType == TX_TYPE_STANDARD;
    }

    friend bool operator==(const Credits_CTransaction& a, const Credits_CTransaction& b)
    {
        return (a.nVersion  == b.nVersion &&
        		a.nTxType   == b.nTxType &&
                a.vin       == b.vin &&
                a.vout      == b.vout &&
                a.signingKeyId      == b.signingKeyId &&
                a.nLockTime == b.nLockTime);
    }

    friend bool operator!=(const Credits_CTransaction& a, const Credits_CTransaction& b)
    {
        return !(a == b);
    }


    std::string ToString() const;
    void print() const;
};

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutCompressor
{
private:
    CTxOut &txout;

public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);

    CTxOutCompressor(CTxOut &txoutIn) : txout(txoutIn) { }

    IMPLEMENT_SERIALIZE(({
        if (!fRead) {
            uint64_t nVal = CompressAmount(txout.nValue);
            READWRITE(VARINT(nVal));
        } else {
            uint64_t nVal = 0;
            READWRITE(VARINT(nVal));
            txout.nValue = DecompressAmount(nVal);
        }
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
    });)
};

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutClaimCompressor
{
private:
    CTxOutClaim &txout;

public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);

    CTxOutClaimCompressor(CTxOutClaim &txoutIn) : txout(txoutIn) { }

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
class Credits_CTxInUndo
{
public:
    CTxOut txout;         // the txout data before being spent
    bool fCoinBase;       // if the outpoint was the last unspent: whether it belonged to a coinbase
    unsigned int nHeight; // if the outpoint was the last unspent: its height
    int nMetaData; // if the outpoint was the last unspent: its metadata
    int nVersion;         // if the outpoint was the last unspent: its version

    Credits_CTxInUndo() : txout(), fCoinBase(false), nHeight(0), nMetaData(0), nVersion(0) {}
    Credits_CTxInUndo(const CTxOut &txoutIn, bool fCoinBaseIn = false, unsigned int nHeightIn = 0, int nMetaDataIn = 0, int nVersionIn = 0) : txout(txoutIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn), nMetaData(nMetaDataIn), nVersion(nVersionIn) { }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        return ::GetSerializeSize(VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nMetaData), nType, nVersion) : 0) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion) : 0) +
               ::GetSerializeSize(CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        ::Serialize(s, VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion);
        if (nHeight > 0) {
            ::Serialize(s, VARINT(this->nMetaData), nType, nVersion);
            ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        }
        ::Serialize(s, CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        nHeight = nCode / 2;
        fCoinBase = nCode & 1;
        if (nHeight > 0) {
            ::Unserialize(s, VARINT(this->nMetaData), nType, nVersion);
            ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        }
        ::Unserialize(s, REF(CTxOutCompressor(REF(txout))), nType, nVersion);
    }
};

/** Undo information for a Credits_CTransaction */
class Credits_CTxUndo
{
public:
    // undo information for all txins
    std::vector<Credits_CTxInUndo> vprevout;

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
class Credits_CBlockHeader
{
public:
    // header
    static const int CURRENT_VERSION=1;
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint256 hashLinkedBitcoinBlock;
    uint256 hashSigMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;
    uint64_t nTotalMonetaryBase;
    uint64_t nTotalDepositBase;
    uint64_t nDepositAmount;

    Credits_CBlockHeader()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(hashLinkedBitcoinBlock);
        READWRITE(hashSigMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        READWRITE(nTotalMonetaryBase);
        READWRITE(nTotalDepositBase);
        READWRITE(nDepositAmount);
    )

    void SetNull()
    {
        nVersion = Credits_CBlockHeader::CURRENT_VERSION;
        hashPrevBlock = 0;
        hashMerkleRoot = 0;
        hashLinkedBitcoinBlock = 0;
        hashSigMerkleRoot = 0;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        nTotalMonetaryBase = 0;
        nTotalDepositBase = 0;
        nDepositAmount = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;
    uint256 GetLockHash() const;
    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class Credits_CBlock : public Credits_CBlockHeader
{
public:
    // network and disk
    std::vector<CCompactSignature> vsig;
    std::vector<Credits_CTransaction> vtx;

    // memory only
    mutable std::vector<uint256> vSigMerkleTree;
    mutable std::vector<uint256> vMerkleTree;

    Credits_CBlock()
    {
        SetNull();
    }

    Credits_CBlock(const Credits_CBlockHeader &header)
    {
        SetNull();
        *((Credits_CBlockHeader*)this) = header;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(*(Credits_CBlockHeader*)this);
        READWRITE(vsig);
        READWRITE(vtx);
    )

    void SetNull()
    {
        Credits_CBlockHeader::SetNull();
        vsig.clear();
        vtx.clear();
        vSigMerkleTree.clear();
        vMerkleTree.clear();
    }

    Credits_CBlockHeader GetBlockHeader() const
    {
        Credits_CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.hashLinkedBitcoinBlock         = hashLinkedBitcoinBlock;
        block.hashSigMerkleRoot         = hashSigMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.nTotalMonetaryBase = nTotalMonetaryBase;
        block.nTotalDepositBase = nTotalDepositBase;
        block.nDepositAmount = nDepositAmount;
        return block;
    }

    uint256 BuildSigMerkleTree() const;
    uint256 BuildMerkleTree() const;
    void RecalcLockHashAndMerkleRoot();
    bool UpdateSignatures(const CKeyStore &deposit_keyStore);
    const uint256 &GetTxHash(unsigned int nIndex) const {
        assert(vMerkleTree.size() > 0); // BuildMerkleTree must have been called first
        assert(nIndex < vtx.size());
        return vMerkleTree[nIndex];
    }

    uint64_t GetDepositAmount() const {
        uint64_t totalValueOut = 0;
        for (unsigned int i = 1; i < vtx.size(); i++) {
        	if(vtx[i].IsDeposit()) {
        		totalValueOut += vtx[i].GetDepositValueOut();
        	} else {
        		break;
        	}
        }
    	return totalValueOut;
    }
    uint64_t GetTotalValueOut() const {
        uint64_t totalValueOut = 0;
        for (unsigned int i = 0; i < vtx.size(); i++) {
        	totalValueOut += vtx[i].GetValueOut();
        }
    	return totalValueOut;
    }
    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    void print() const;
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull()
    {
        return vHave.empty();
    }
};

class CBlockIndexBase {
public:
    // pointer to the hash of the block, if any. memory is owned by this CBlockIndex
    const uint256* phashBlock;

    // pointer to the index of the predecessor of this block
    CBlockIndexBase* pprev;

    // height of the entry in the chain. The genesis block has height 0
    int nHeight;

    // Which # file this block is stored in (blk?????.dat)
    int nFile;

    // Byte offset within blk?????.dat where this block's data is stored
    unsigned int nDataPos;

    // Byte offset within rev?????.dat where this block's undo data is stored
    unsigned int nUndoPos;

    // Byte offset within cla?????.dat where this block's undo claim data is stored
    unsigned int nUndoPosClaim;

    // (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    uint256 nChainWork;

    // Number of transactions in this block.
    // Note: in a potential headers-first mode, this number cannot be relied upon
    unsigned int nTx;

    // (memory only) Number of transactions in the chain up to and including this block
    unsigned int nChainTx; // change to 64-bit type when necessary; won't happen before 2030

    // Verification status of this block. See enum BlockStatus
    int nStatus;

    // block header
    unsigned int nTime;
    unsigned int nBits;

    uint256 GetBlockHash() const
    {
        return *phashBlock;
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    uint256 GetBlockWork() const
    {
        uint256 bnTarget;
        bool fNegative;
        bool fOverflow;
        bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
        if (fNegative || fOverflow || bnTarget == 0)
            return 0;
        // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
        // as it's too large for a uint256. However, as 2**256 is at least as large
        // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
        // or ~bnTarget / (nTarget+1) + 1.
        return (~bnTarget / (bnTarget + 1)) + 1;
    }
};

#endif
