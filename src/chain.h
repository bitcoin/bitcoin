// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAIN_H
#define BITCOIN_CHAIN_H

#include <arith_uint256.h>
#include <consensus/params.h>
#include <flatfile.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/time.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

/**
 * Maximum amount of time that a block timestamp is allowed to exceed the
 * current time before the block will be accepted.
 */
static constexpr int64_t MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60;

/**
 * Timestamp window used as a grace period by code that compares external
 * timestamps (such as timestamps passed to RPCs, or wallet key creation times)
 * to block timestamps. This should be set at least as high as
 * MAX_FUTURE_BLOCK_TIME.
 */
static constexpr int64_t TIMESTAMP_WINDOW = MAX_FUTURE_BLOCK_TIME;

/**
 * Maximum gap between node time and block time used
 * for the "Catching up..." mode in GUI.
 *
 * Ref: https://github.com/bitcoin/bitcoin/pull/1026
 */
static constexpr int64_t MAX_BLOCK_TIME_GAP = 90 * 60;

class CBlockFileInfo
{
public:
    unsigned int nBlocks{};      //!< number of blocks stored in file
    unsigned int nSize{};        //!< number of used bytes of block file
    unsigned int nUndoSize{};    //!< number of used bytes in the undo file
    unsigned int nHeightFirst{}; //!< lowest height of block in file
    unsigned int nHeightLast{};  //!< highest height of block in file
    uint64_t nTimeFirst{};       //!< earliest time of block in file
    uint64_t nTimeLast{};        //!< latest time of block in file

    SERIALIZE_METHODS(CBlockFileInfo, obj)
    {
        READWRITE(VARINT(obj.nBlocks));
        READWRITE(VARINT(obj.nSize));
        READWRITE(VARINT(obj.nUndoSize));
        READWRITE(VARINT(obj.nHeightFirst));
        READWRITE(VARINT(obj.nHeightLast));
        READWRITE(VARINT(obj.nTimeFirst));
        READWRITE(VARINT(obj.nTimeLast));
    }

    CBlockFileInfo() = default;

    std::string ToString() const;

    /** update statistics (does not update nSize) */
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn)
    {
        if (nBlocks == 0 || nHeightFirst > nHeightIn)
            nHeightFirst = nHeightIn;
        if (nBlocks == 0 || nTimeFirst > nTimeIn)
            nTimeFirst = nTimeIn;
        nBlocks++;
        if (nHeightIn > nHeightLast)
            nHeightLast = nHeightIn;
        if (nTimeIn > nTimeLast)
            nTimeLast = nTimeIn;
    }
};

enum BlockStatus : uint32_t {
    //! Unused.
    BLOCK_VALID_UNKNOWN      =    0,

    //! Reserved (was BLOCK_VALID_HEADER).
    BLOCK_VALID_RESERVED     =    1,

    //! All parent headers found, difficulty matches, timestamp >= median previous, checkpoint. Implies all parents
    //! are also at least TREE.
    BLOCK_VALID_TREE         =    2,

    /**
     * Only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids,
     * sigops, size, merkle root. Implies all parents are at least TREE but not necessarily TRANSACTIONS.
     *
     * If a block's validity is at least VALID_TRANSACTIONS, CBlockIndex::nTx will be set. If a block and all previous
     * blocks back to the genesis block or an assumeutxo snapshot block are at least VALID_TRANSACTIONS,
     * CBlockIndex::m_chain_tx_count will be set.
     */
    BLOCK_VALID_TRANSACTIONS =    3,

    //! Outputs do not overspend inputs, no double spends, coinbase output ok, no immature coinbase spends, BIP30.
    //! Implies all previous blocks back to the genesis block or an assumeutxo snapshot block are at least VALID_CHAIN.
    BLOCK_VALID_CHAIN        =    4,

    //! Scripts & signatures ok. Implies all previous blocks back to the genesis block or an assumeutxo snapshot block
    //! are at least VALID_SCRIPTS.
    BLOCK_VALID_SCRIPTS      =    5,

    //! All validity bits.
    BLOCK_VALID_MASK         =   BLOCK_VALID_RESERVED | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
                                 BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,

    BLOCK_HAVE_DATA          =    8, //!< full block available in blk*.dat
    BLOCK_HAVE_UNDO          =   16, //!< undo data available in rev*.dat
    BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,

    BLOCK_FAILED_VALID       =   32, //!< stage after last reached validness failed
    BLOCK_FAILED_CHILD       =   64, //!< descends from failed block
    BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,

    BLOCK_OPT_WITNESS        =   128, //!< block data in blk*.dat was received with a witness-enforcing client

    BLOCK_STATUS_RESERVED    =   256, //!< Unused flag that was previously set on assumeutxo snapshot blocks and their
                                      //!< ancestors before they were validated, and unset when they were validated.
};

/** The block chain is a tree shaped structure starting with the
 * genesis block at the root, with each block potentially having multiple
 * candidates to be the next block. A blockindex may have multiple pprev pointing
 * to it, but at most one of them can be part of the currently active branch.
 */
class CBlockIndex
{
public:
    //! pointer to the hash of the block, if any. Memory is owned by this CBlockIndex
    const uint256* phashBlock{nullptr};

    //! pointer to the index of the predecessor of this block
    CBlockIndex* pprev{nullptr};

    //! pointer to the index of some further predecessor of this block
    CBlockIndex* pskip{nullptr};

    //! height of the entry in the chain. The genesis block has height 0
    int nHeight{0};

    //! Which # file this block is stored in (blk?????.dat)
    int nFile GUARDED_BY(::cs_main){0};

    //! Byte offset within blk?????.dat where this block's data is stored
    unsigned int nDataPos GUARDED_BY(::cs_main){0};

    //! Byte offset within rev?????.dat where this block's undo data is stored
    unsigned int nUndoPos GUARDED_BY(::cs_main){0};

    //! (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    arith_uint256 nChainWork{};

    //! Number of transactions in this block. This will be nonzero if the block
    //! reached the VALID_TRANSACTIONS level, and zero otherwise.
    //! Note: in a potential headers-first mode, this number cannot be relied upon
    unsigned int nTx{0};

    //! (memory only) Number of transactions in the chain up to and including this block.
    //! This value will be non-zero if this block and all previous blocks back
    //! to the genesis block or an assumeutxo snapshot block have reached the
    //! VALID_TRANSACTIONS level.
    uint64_t m_chain_tx_count{0};

    //! Verification status of this block. See enum BlockStatus
    //!
    //! Note: this value is modified to show BLOCK_OPT_WITNESS during UTXO snapshot
    //! load to avoid a spurious startup failure requiring -reindex.
    //! @sa NeedsRedownload
    //! @sa ActivateSnapshot
    uint32_t nStatus GUARDED_BY(::cs_main){0};

    //! block header
    int32_t nVersion{0};
    uint256 hashMerkleRoot{};
    uint32_t nTime{0};
    uint32_t nBits{0};
    uint32_t nNonce{0};

    //! (memory only) Sequential id assigned to distinguish order in which blocks are received.
    int32_t nSequenceId{0};

    //! (memory only) Maximum nTime in the chain up to and including this block.
    unsigned int nTimeMax{0};

    explicit CBlockIndex(const CBlockHeader& block)
        : nVersion{block.nVersion},
          hashMerkleRoot{block.hashMerkleRoot},
          nTime{block.nTime},
          nBits{block.nBits},
          nNonce{block.nNonce}
    {
    }

    FlatFilePos GetBlockPos() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        FlatFilePos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos = nDataPos;
        }
        return ret;
    }

    FlatFilePos GetUndoPos() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        FlatFilePos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos = nUndoPos;
        }
        return ret;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion = nVersion;
        if (pprev)
            block.hashPrevBlock = pprev->GetBlockHash();
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        return block;
    }

    uint256 GetBlockHash() const
    {
        assert(phashBlock != nullptr);
        return *phashBlock;
    }

    /**
     * Check whether this block and all previous blocks back to the genesis block or an assumeutxo snapshot block have
     * reached VALID_TRANSACTIONS and had transactions downloaded (and stored to disk) at some point.
     *
     * Does not imply the transactions are consensus-valid (ConnectTip might fail)
     * Does not imply the transactions are still stored on disk. (IsBlockPruned might return true)
     *
     * Note that this will be true for the snapshot base block, if one is loaded, since its m_chain_tx_count value will have
     * been set manually based on the related AssumeutxoData entry.
     */
    bool HaveNumChainTxs() const { return m_chain_tx_count != 0; }

    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    int64_t GetBlockTimeMax() const
    {
        return (int64_t)nTimeMax;
    }

    static constexpr int nMedianTimeSpan = 11;

    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
            *(--pbegin) = pindex->GetBlockTime();

        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin) / 2];
    }

    std::string ToString() const;

    //! Check whether this block index entry is valid up to the passed validity level.
    bool IsValid(enum BlockStatus nUpTo = BLOCK_VALID_TRANSACTIONS) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        return ((nStatus & BLOCK_VALID_MASK) >= nUpTo);
    }

    //! Raise the validity level of this block index entry.
    //! Returns true if the validity was changed.
    bool RaiseValidity(enum BlockStatus nUpTo) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK) return false;

        if ((nStatus & BLOCK_VALID_MASK) < nUpTo) {
            nStatus = (nStatus & ~BLOCK_VALID_MASK) | nUpTo;
            return true;
        }
        return false;
    }

    //! Build the skiplist pointer for this entry.
    void BuildSkip();

    //! Efficiently find an ancestor of this block.
    CBlockIndex* GetAncestor(int height);
    const CBlockIndex* GetAncestor(int height) const;

    CBlockIndex() = default;
    ~CBlockIndex() = default;

protected:
    //! CBlockIndex should not allow public copy construction because equality
    //! comparison via pointer is very common throughout the codebase, making
    //! use of copy a footgun. Also, use of copies do not have the benefit
    //! of simplifying lifetime considerations due to attributes like pprev and
    //! pskip, which are at risk of becoming dangling pointers in a copied
    //! instance.
    //!
    //! We declare these protected instead of simply deleting them so that
    //! CDiskBlockIndex can reuse copy construction.
    CBlockIndex(const CBlockIndex&) = default;
    CBlockIndex& operator=(const CBlockIndex&) = delete;
    CBlockIndex(CBlockIndex&&) = delete;
    CBlockIndex& operator=(CBlockIndex&&) = delete;
};

arith_uint256 GetBlockProof(const CBlockIndex& block);
/** Return the time it would take to redo the work difference between from and to, assuming the current hashrate corresponds to the difficulty at tip, in seconds. */
int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params&);
/** Find the forking point between two chain tips. */
const CBlockIndex* LastCommonAncestor(const CBlockIndex* pa, const CBlockIndex* pb);


/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex : public CBlockIndex
{
    /** Historically CBlockLocator's version field has been written to disk
     * streams as the client version, but the value has never been used.
     *
     * Hard-code to the highest client version ever written.
     * SerParams can be used if the field requires any meaning in the future.
     **/
    static constexpr int DUMMY_VERSION = 259900;

public:
    uint256 hashPrev;

    CDiskBlockIndex()
    {
        hashPrev = uint256();
    }

    explicit CDiskBlockIndex(const CBlockIndex* pindex) : CBlockIndex(*pindex)
    {
        hashPrev = (pprev ? pprev->GetBlockHash() : uint256());
    }

    SERIALIZE_METHODS(CDiskBlockIndex, obj)
    {
        LOCK(::cs_main);
        int _nVersion = DUMMY_VERSION;
        READWRITE(VARINT_MODE(_nVersion, VarIntMode::NONNEGATIVE_SIGNED));

        READWRITE(VARINT_MODE(obj.nHeight, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(VARINT(obj.nStatus));
        READWRITE(VARINT(obj.nTx));
        if (obj.nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO)) READWRITE(VARINT_MODE(obj.nFile, VarIntMode::NONNEGATIVE_SIGNED));
        if (obj.nStatus & BLOCK_HAVE_DATA) READWRITE(VARINT(obj.nDataPos));
        if (obj.nStatus & BLOCK_HAVE_UNDO) READWRITE(VARINT(obj.nUndoPos));

        // block header
        READWRITE(obj.nVersion);
        READWRITE(obj.hashPrev);
        READWRITE(obj.hashMerkleRoot);
        READWRITE(obj.nTime);
        READWRITE(obj.nBits);
        READWRITE(obj.nNonce);
    }

    uint256 ConstructBlockHash() const
    {
        CBlockHeader block;
        block.nVersion = nVersion;
        block.hashPrevBlock = hashPrev;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        return block.GetHash();
    }

    uint256 GetBlockHash() = delete;
    std::string ToString() = delete;
};

/** An in-memory indexed chain of blocks. */
class CChain
{
private:
    std::vector<CBlockIndex*> vChain;

public:
    CChain() = default;
    CChain(const CChain&) = delete;
    CChain& operator=(const CChain&) = delete;

    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex* Genesis() const
    {
        return vChain.size() > 0 ? vChain[0] : nullptr;
    }

    /** Returns the index entry for the tip of this chain, or nullptr if none. */
    CBlockIndex* Tip() const
    {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : nullptr;
    }

    /** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
    CBlockIndex* operator[](int nHeight) const
    {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return nullptr;
        return vChain[nHeight];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex* pindex) const
    {
        return (*this)[pindex->nHeight] == pindex;
    }

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex* Next(const CBlockIndex* pindex) const
    {
        if (Contains(pindex))
            return (*this)[pindex->nHeight + 1];
        else
            return nullptr;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->nHeight : -1. */
    int Height() const
    {
        return int(vChain.size()) - 1;
    }

    /** Set/initialize a chain with a given tip. */
    void SetTip(CBlockIndex& block);

    /** Return a CBlockLocator that refers to the tip in of this chain. */
    CBlockLocator GetLocator() const;

    /** Find the last common block between this chain and a block index entry. */
    const CBlockIndex* FindFork(const CBlockIndex* pindex) const;

    /** Find the earliest block with timestamp equal or greater than the given time and height equal or greater than the given height. */
    CBlockIndex* FindEarliestAtLeast(int64_t nTime, int height) const;
};

/** Get a locator for a block index entry. */
CBlockLocator GetLocator(const CBlockIndex* index);

/** Construct a list of hash entries to put in a locator.  */
std::vector<uint256> LocatorEntries(const CBlockIndex* index);

#endif // BITCOIN_CHAIN_H
