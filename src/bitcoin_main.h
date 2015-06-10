// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_MAIN_H
#define BITCOIN_BITCOIN_MAIN_H

#if defined(HAVE_CONFIG_H)
#include "bitcredit-config.h"
#endif

#include "chainparams.h"
#include "bitcoin_coins.h"
#include "core.h"
#include "bitcoin_core.h"
#include "net.h"
#include "bitcoin_script.h"
#include "sync.h"
#include "bitcoin_txmempool.h"
#include "uint256.h"

#include "main_common.h"
#include "main.h"

#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class Bitcoin_CBlockIndex;
class Bitcoin_CBlockUndoClaim;
class CBloomFilter;
class CInv;

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int BITCOIN_MAX_BLOCK_SIZE = 1000000;
/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const unsigned int BITCOIN_DEFAULT_BLOCK_MAX_SIZE = 750000;
static const unsigned int BITCOIN_DEFAULT_BLOCK_MIN_SIZE = 0;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int BITCOIN_DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int BITCOIN_MAX_STANDARD_TX_SIZE = 100000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int BITCOIN_MAX_BLOCK_SIGOPS = BITCOIN_MAX_BLOCK_SIZE/50;
/** The maximum number of orphan transactions kept in memory */
static const unsigned int BITCOIN_MAX_ORPHAN_TRANSACTIONS = BITCOIN_MAX_BLOCK_SIZE/100;
/** Default for -maxorphanblocks, maximum number of orphan blocks kept in memory */
static const unsigned int BITCOIN_DEFAULT_MAX_ORPHAN_BLOCKS = 750;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int BITCOIN_MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BITCOIN_BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int BITCOIN_UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int BITCOIN_COINBASE_MATURITY = 100;
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int BITCOIN_LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC
/** Maximum number of script-checking threads allowed */
static const int BITCOIN_MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int BITCOIN_DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int BITCOIN_MAX_BLOCKS_IN_TRANSIT_PER_PEER = 128;
/** Timeout in seconds before considering a block download peer unresponsive. */
static const unsigned int BITCOIN_BLOCK_DOWNLOAD_TIMEOUT = 60;

#ifdef USE_UPNP
static const int bitcoin_fHaveUPnP = true;
#else
static const int bitcoin_fHaveUPnP = false;
#endif

/** "reject" message codes **/
static const unsigned char BITCOIN_REJECT_MALFORMED = 0x01;
static const unsigned char BITCOIN_REJECT_INVALID = 0x10;
static const unsigned char BITCOIN_REJECT_OBSOLETE = 0x11;
static const unsigned char BITCOIN_REJECT_DUPLICATE = 0x12;
static const unsigned char BITCOIN_REJECT_NONSTANDARD = 0x40;
static const unsigned char BITCOIN_REJECT_DUST = 0x41;
static const unsigned char BITCOIN_REJECT_INSUFFICIENTFEE = 0x42;
static const unsigned char BITCOIN_REJECT_CHECKPOINT = 0x43;

extern MainState bitcoin_mainState;

extern CScript BITCOIN_COINBASE_FLAGS;
extern Bitcoin_CTxMemPool bitcoin_mempool;
extern std::map<uint256, Bitcoin_CBlockIndex*> bitcoin_mapBlockIndex;
extern uint64_t bitcoin_nLastBlockTx;
extern uint64_t bitcoin_nLastBlockSize;
extern const std::string bitcoin_strMessageMagic;
extern int64_t bitcoin_nTimeBestReceived;
extern bool bitcoin_fBenchmark;
extern int bitcoin_nScriptCheckThreads;
extern bool bitcoin_fTxIndex;
extern bool bitcoin_fTrimBlockFiles;
extern bool bitcoin_fSimplifiedBlockValidation;
extern unsigned int bitcoin_nCoinCacheSize;

// Minimum disk space required - used in CheckDiskSpace()
static const uint64_t bitcoin_nMinDiskSpace = 52428800;


class Bitcoin_CBlockTreeDB;
class Bitcoin_CTxUndo;
class Bitcoin_CScriptCheck;
class CValidationState;
class Bitcoin_CWalletInterface;

struct Bitcoin_CBlockTemplate;


/** Register a wallet to receive updates from core */
void Bitcoin_RegisterWallet(Bitcoin_CWalletInterface* pwalletIn);
/** Unregister a wallet from core */
void Bitcoin_UnregisterWallet(Bitcoin_CWalletInterface* pwalletIn);
/** Unregister all wallets from core */
void Bitcoin_UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void Bitcoin_SyncWithWallets(const uint256 &hash, const Bitcoin_CTransaction& tx, const Bitcoin_CBlock* pblock = NULL);

/** Register with a network node to receive its signals */
void Bitcoin_RegisterNodeSignals(CNodeSignals& nodeSignals);
/** Unregister a network node */
void Bitcoin_UnregisterNodeSignals(CNodeSignals& nodeSignals);

void Bitcoin_PushGetBlocks(CNode* pnode, Bitcoin_CBlockIndex* pindexBegin, uint256 hashEnd);

/** Process an incoming block */
bool Bitcoin_ProcessBlock(CValidationState &state, CNode* pfrom, Bitcoin_CBlock* pblock, CDiskBlockPos *dbp = NULL);
/** Check whether enough disk space is available for an incoming block */
bool Bitcoin_CheckDiskSpace(uint64_t nAdditionalBytes = 0);
/** Open a block file (blk?????.dat) */
FILE* Bitcoin_OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Open an undo file (rev?????.dat) */
FILE* Bitcoin_OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Open a claim  undo file (cla?????.dat) */
FILE* Bitcoin_OpenUndoFileClaim(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Import blocks from an external file */
bool Bitcoin_LoadExternalBlockFile(FILE* fileIn, CDiskBlockPos *dbp = NULL);
/** Initialize a new block tree database + block data on disk */
bool Bitcoin_InitBlockIndex();
/** Load the block tree and coins database from disk */
bool Bitcoin_LoadBlockIndex();
/** Unload database information */
void Bitcoin_UnloadBlockIndex();
/** Print the loaded block tree */
void Bitcoin_PrintBlockTree();
/** Process protocol messages received from a given node */
bool Bitcoin_ProcessMessages(CNode* pfrom);
/** Send queued protocol messages to be sent to a give node */
bool Bitcoin_SendMessages(CNode* pto, bool fSendTrickle);
/** Run an instance of the script checking thread */
void Bitcoin_ThreadScriptCheck();
/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool Bitcoin_CheckProofOfWork(uint256 hash, unsigned int nBits);
/** Calculate the minimum amount of work a received block needs, without knowing its direct parent */
unsigned int Bitcoin_ComputeMinWork(unsigned int nBase, int64_t nTime);
/** Check whether we are doing an initial block download (synchronizing from disk or network) */
bool Bitcoin_IsInitialBlockDownload();
/** Format a string that describes several potential problems detected by the core */
std::string Bitcoin_GetWarnings(std::string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool Bitcoin_GetTransaction(const uint256 &hash, Bitcoin_CTransaction &tx, uint256 &hashBlock, bool fAllowSlow = false);
/** Find the best known block, and make it the tip of the block chain */
bool Bitcoin_ActivateBestChain(CValidationState &state);
/** Trim all old block files + undo files up to this block */
bool Bitcoin_TrimBlockHistory(const Bitcoin_CBlockIndex * pTrimTo);
/** Move the position of the claim tip (a structure similar to chainstate + undo) */
bool Bitcoin_AlignClaimTip(const Credits_CBlockIndex *expectedCurrentBlockIndex, const Credits_CBlockIndex *palignToBlockIndex, Bitcoin_CCoinsViewCache& view, CValidationState &state, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims);
int64_t Bitcoin_GetBlockValue(int nHeight, int64_t nFees);
unsigned int Bitcoin_GetNextWorkRequired(const Bitcoin_CBlockIndex* pindexLast, const Bitcoin_CBlockHeader *pblock);

//ONLY exposed here for testability
uint64_t Bitcoin_GetTotalMonetaryBase(int nHeight);

void Bitcoin_UpdateTime(Bitcoin_CBlockHeader& block, const Bitcoin_CBlockIndex* pindexPrev);

/** Verify a signature */
bool Bitcoin_VerifySignature(const Claim_CCoins& txFrom, const Bitcoin_CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);
/** Abort with a message */
bool AbortNode(const std::string &msg);
/** Get statistics from node state */
bool Bitcoin_GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Bitcoin_Misbehaving(NodeId nodeid, int howmuch);


/** (try to) add transaction to memory pool **/
bool Bitcoin_AcceptToMemoryPool(Bitcoin_CTxMemPool& pool, CValidationState &state, const Bitcoin_CTransaction &tx, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectInsaneFee=false);



int64_t Bitcoin_GetMinFee(const Bitcoin_CTransaction& tx, unsigned int nBytes, bool fAllowFree, enum GetMinFee_mode mode);

//
// Check transaction inputs, and make sure any
// pay-to-script-hash transactions are evaluating IsStandard scripts
//
// Why bother? To avoid denial-of-service attacks; an attacker
// can submit a standard HASH... OP_EQUAL transaction,
// which will get accepted into blocks. The redemption
// script can be anything; an attacker could use a very
// expensive-to-check-upon-redemption script like:
//   DUP CHECKSIG DROP ... repeated 100 times... OP_1
//

/** Check for standard transaction types
    @param[in] mapInputs    Map of previous transactions that have outputs we're spending
    @return True if all inputs (scriptSigs) use only standard transaction forms
*/
bool Bitcoin_AreInputsStandard(const Bitcoin_CTransaction& tx, Bitcoin_CCoinsViewCache& mapInputs);

/** Count ECDSA signature operations the old-fashioned (pre-0.6) way
    @return number of sigops this transaction's outputs will produce when spent
    @see Bitcoin_CTransaction::FetchInputs
*/
unsigned int Bitcoin_GetLegacySigOpCount(const Bitcoin_CTransaction& tx);

/** Count ECDSA signature operations in pay-to-script-hash inputs.

    @param[in] mapInputs	Map of previous transactions that have outputs we're spending
    @return maximum number of sigops required to validate this transaction's inputs
    @see Bitcoin_CTransaction::FetchInputs
 */
unsigned int Bitcoin_GetP2SHSigOpCount(const Bitcoin_CTransaction& tx, Bitcoin_CCoinsViewCache& mapInputs);


inline bool Bitcoin_AllowFree(double dPriority)
{
    // Large (in bytes) low-priority (new, small-coin) transactions
    // need a fee.
    return dPriority > COIN * 144 / 250;
}

// Check whether all inputs of this transaction are valid (no double spends, scripts & sigs, amounts)
// This does not modify the UTXO set. If pvChecks is not NULL, script checks are pushed onto it
// instead of being performed inline.
bool Bitcoin_CheckInputs(const Bitcoin_CTransaction& tx, CValidationState &state, Bitcoin_CCoinsViewCache &view, bool fScriptChecks = true,
                 unsigned int flags = STANDARD_SCRIPT_VERIFY_FLAGS,
                 std::vector<Bitcoin_CScriptCheck> *pvChecks = NULL);

// Apply the effects of this transaction on the UTXO set represented by view
void Bitcoin_UpdateCoins(const Bitcoin_CTransaction& tx, CValidationState &state, Bitcoin_CCoinsViewCache &inputs, Bitcoin_CTxUndo &txundo, Bitcoin_CTxUndo &claim_txundo,  int nHeight, const uint256 &txhash);

// Context-independent validity checks
bool Bitcoin_CheckTransaction(const Bitcoin_CTransaction& tx, CValidationState& state);

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool Bitcoin_IsStandardTx(const Bitcoin_CTransaction& tx, std::string& reason);

bool Bitcoin_IsFinalTx(const Bitcoin_CTransaction &tx, int nBlockHeight = 0, int64_t nBlockTime = 0);

/** Undo information for a CBlock */
class Bitcoin_CBlockUndoClaim
{
public:
    std::vector<Bitcoin_CTxUndo> vtxundo; // for all but the coinbase

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);
    )

    bool WriteToDisk(FILE * writeToFile, CDiskBlockPos &pos, const uint256 &hashBlock, CNetParams * netParams)
    {
        // Open history file to append
        CAutoFile fileout = CAutoFile(writeToFile, SER_DISK, netParams->ClientVersion());
        if (!fileout)
            return error("Bitcoin: CBlockUndoClaim::WriteToDisk : OpenUndoFile failed");

        // Write index header
        unsigned int nSize = fileout.GetSerializeSize(*this);
        fileout << FLATDATA(netParams->MessageStart()) << nSize;

        // Write undo data
        long fileOutPos = ftell(fileout);
        if (fileOutPos < 0)
            return error("Bitcoin: CBlockUndoClaim::WriteToDisk : ftell failed");
        pos.nPos = (unsigned int)fileOutPos;
        fileout << *this;

        // calculate & write checksum
        CHashWriter hasher(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
        hasher << hashBlock;
        hasher << *this;
        fileout << hasher.GetHash();

        // Flush stdio buffers and commit to disk before returning
        fflush(fileout);
        if (!Bitcoin_IsInitialBlockDownload())
            FileCommit(fileout);

        return true;
    }

    bool ReadFromDisk(FILE * readFromFile, const CDiskBlockPos &pos, const uint256 &hashBlock, CNetParams * netParams)
    {
        // Open history file to read
        CAutoFile filein = CAutoFile(readFromFile, SER_DISK, netParams->ClientVersion());
        if (!filein)
            return error("Bitcoin: CBlockUndoClaim::ReadFromDisk : OpenBlockFile failed");

        // Read block
        uint256 hashChecksum;
        try {
            filein >> *this;
            filein >> hashChecksum;
        }
        catch (std::exception &e) {
            return error("Bitcoin: %s : Deserialize or I/O error - %s", __func__, e.what());
        }

        // Verify checksum
        CHashWriter hasher(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
        hasher << hashBlock;
        hasher << *this;
        if (hashChecksum != hasher.GetHash())
            return error("Bitcoin: CBlockUndoClaim::ReadFromDisk : Checksum mismatch");

        return true;
    }
};


/** Closure representing one script verification
 *  Note that this stores references to the spending transaction */
class Bitcoin_CScriptCheck
{
private:
    CScript scriptPubKey;
    const Bitcoin_CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    int nHashType;

public:
    Bitcoin_CScriptCheck() {}
    Bitcoin_CScriptCheck(const Claim_CCoins& txFromIn, const Bitcoin_CTransaction& txToIn, unsigned int nInIn, unsigned int nFlagsIn, int nHashTypeIn) :
        scriptPubKey(txFromIn.vout[txToIn.vin[nInIn].prevout.n].scriptPubKey),
        ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), nHashType(nHashTypeIn) { }

    bool operator()() const;

    void swap(Bitcoin_CScriptCheck &check) {
        scriptPubKey.swap(check.scriptPubKey);
        std::swap(ptxTo, check.ptxTo);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(nHashType, check.nHashType);
    }
};

/** A transaction with a merkle branch linking it to the block chain. */
class Bitcoin_CMerkleTx : public Bitcoin_CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(Bitcoin_CBlockIndex* &pindexRet) const;

public:
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable bool fMerkleVerified;


    Bitcoin_CMerkleTx()
    {
        Init();
    }

    Bitcoin_CMerkleTx(const Bitcoin_CTransaction& txIn) : Bitcoin_CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        hashBlock = 0;
        nIndex = -1;
        fMerkleVerified = false;
    }


    IMPLEMENT_SERIALIZE
    (
        nSerSize += SerReadWrite(s, *(Bitcoin_CTransaction*)this, nType, nVersion, ser_action);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    )


    int SetMerkleBranch(const Bitcoin_CBlock* pblock=NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(Bitcoin_CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { Bitcoin_CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { Bitcoin_CBlockIndex *pindexRet; return GetDepthInMainChainINTERNAL(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree=true);
};




/** THESE THREE FUNCTIONS MAY FAIL IF THE BITCOIN BLOCKCHAIN IS TRIMMED */
/** Functions for disk access for blocks */
bool Bitcoin_WriteBlockToDisk(Bitcoin_CBlock& block, CDiskBlockPos& pos);
bool Bitcoin_ReadBlockFromDisk(Bitcoin_CBlock& block, const CDiskBlockPos& pos);
bool Bitcoin_ReadBlockFromDisk(Bitcoin_CBlock& block, const Bitcoin_CBlockIndex* pindex);


/** Functions for validating blocks and updating the block tree */

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  In case pfClean is provided, operation will try to be tolerant about errors, and *pfClean
 *  will be true if no problems were found. Otherwise, the return value will be false in case
 *  of problems. Note that in any case, coins may be modified. */
/** THIS FUNCTION MAY FAIL IF THE BITCOIN BLOCKCHAIN IS TRIMMED */
bool Bitcoin_DeleteBlockUndoClaimsFromDisk(CValidationState& state, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims);
bool Bitcoin_DisconnectBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& coins,  bool updateUndo, bool* pfClean = NULL);
bool Bitcoin_DisconnectBlockForClaim(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& coins, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims, bool* pfClean = NULL);

// Apply the effects of this block (with given index) on the UTXO set represented by coins
/** THIS FUNCTION MAY FAIL IF THE BITCOIN BLOCKCHAIN IS TRIMMED */
bool Bitcoin_WriteBlockUndoClaimsToDisk(CValidationState& state, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims);
bool Bitcoin_WriteChainState(CValidationState &state, bool writeBitcoin, bool writeClaim);
bool Bitcoin_ConnectBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& coins, bool updateUndo, bool fJustCheck);
bool Bitcoin_ConnectBlockForClaim(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& coins, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims);

// Add this block to the block index, and if necessary, switch the active block chain to this
bool Bitcoin_AddToBlockIndex(Bitcoin_CBlock& block, CValidationState& state, const CDiskBlockPos& pos);

// Context-independent validity checks
bool Bitcoin_CheckBlockHeader(const Bitcoin_CBlockHeader& block, CValidationState& state, bool fCheckPOW = true);
bool Bitcoin_CheckBlock(const Bitcoin_CBlock& block, CValidationState& state, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

// Store block on disk
// if dbp is provided, the file is known to already reside on disk
bool Bitcoin_AcceptBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex **pindex, CDiskBlockPos* dbp, CNetParams * netParams);
bool Bitcoin_AcceptBlockHeader(Bitcoin_CBlockHeader& block, CValidationState& state, Bitcoin_CBlockIndex **ppindex= NULL);


/** The block chain is a tree shaped structure starting with the
 * genesis block at the root, with each block potentially having multiple
 * candidates to be the next block. A blockindex may have multiple pprev pointing
 * to it, but at most one of them can be part of the currently active branch.
 */
class Bitcoin_CBlockIndex : public CBlockIndexBase
{
public:
    // block header
    int nVersion;
    uint256 hashMerkleRoot;
    unsigned int nNonce;

    // (memory only) Sequencial id assigned to distinguish order in which blocks are received.
    uint32_t nSequenceId;

    Bitcoin_CBlockIndex()
    {
        phashBlock = NULL;
        pprev = NULL;
        nHeight = 0;
        nFile = 0;
        nDataPos = 0;
        nUndoPos = 0;
        nUndoPosClaim = 0;
        nChainWork = 0;
        nTx = 0;
        nChainTx = 0;
        nStatus = 0;
        nSequenceId = 0;

        nVersion       = 0;
        hashMerkleRoot = 0;
        nTime          = 0;
        nBits          = 0;
        nNonce         = 0;
    }

    Bitcoin_CBlockIndex(Bitcoin_CBlockHeader& block)
    {
        phashBlock = NULL;
        pprev = NULL;
        nHeight = 0;
        nFile = 0;
        nDataPos = 0;
        nUndoPos = 0;
        nUndoPosClaim = 0;
        nChainWork = 0;
        nTx = 0;
        nChainTx = 0;
        nStatus = 0;
        nSequenceId = 0;

        nVersion       = block.nVersion;
        hashMerkleRoot = block.hashMerkleRoot;
        nTime          = block.nTime;
        nBits          = block.nBits;
        nNonce         = block.nNonce;
    }

    CDiskBlockPos GetBlockPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos  = nDataPos;
        }
        return ret;
    }

    CDiskBlockPos GetUndoPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos  = nUndoPos;
        }
        return ret;
    }

    CDiskBlockPos GetUndoPosClaim() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_UNDO_CLAIM) {
            ret.nFile = nFile;
            ret.nPos  = nUndoPosClaim;
        }
        return ret;
    }

    Bitcoin_CBlockHeader GetBlockHeader() const
    {
    	Bitcoin_CBlockHeader block;
        block.nVersion       = nVersion;
        if (pprev)
            block.hashPrevBlock = pprev->GetBlockHash();
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    bool CheckIndex() const
    {
        return Bitcoin_CheckProofOfWork(GetBlockHash(), nBits);
    }

    enum { nMedianTimeSpan=11 };

    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndexBase* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
            *(--pbegin) = pindex->GetBlockTime();

        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }

    /**
     * Returns true if there are nRequired or more blocks of minVersion or above
     * in the last nToCheck blocks, starting at pstart and going backwards.
     */
    static bool IsSuperMajority(int minVersion, const Bitcoin_CBlockIndex* pstart,
                                unsigned int nRequired, unsigned int nToCheck);

    std::string ToString() const
    {
        return strprintf("CBlockIndex(pprev=%p, nHeight=%d, merkle=%s, hashBlock=%s)",
            pprev, nHeight,
            hashMerkleRoot.ToString(),
            GetBlockHash().ToString());
    }

    void print() const
    {
        LogPrintf("%s\n", ToString());
    }

    // Check whether this block index entry is valid up to the passed validity level.
    bool IsValid(enum BlockStatus nUpTo = BLOCK_VALID_TRANSACTIONS) const
    {
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        return ((nStatus & BLOCK_VALID_MASK) >= nUpTo);
    }

    // Raise the validity level of this block index entry.
    // Returns true if the validity was changed.
    bool RaiseValidity(enum BlockStatus nUpTo)
    {
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        if ((nStatus & BLOCK_VALID_MASK) < nUpTo) {
            nStatus = (nStatus & ~BLOCK_VALID_MASK) | nUpTo;
            return true;
        }
        return false;
    }
};



/** Used to marshal pointers into hashes for db storage. */
class Bitcoin_CDiskBlockIndex : public Bitcoin_CBlockIndex
{
public:
    uint256 hashPrev;

    Bitcoin_CDiskBlockIndex() {
        hashPrev = 0;
    }

    explicit Bitcoin_CDiskBlockIndex(Bitcoin_CBlockIndex* pindex) : Bitcoin_CBlockIndex(*pindex) {
        hashPrev = (pprev ? pprev->GetBlockHash() : 0);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(VARINT(nVersion));

        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nStatus));
        READWRITE(VARINT(nTx));
        if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO | BLOCK_HAVE_UNDO_CLAIM))
            READWRITE(VARINT(nFile));
        if (nStatus & BLOCK_HAVE_DATA)
            READWRITE(VARINT(nDataPos));
        if (nStatus & BLOCK_HAVE_UNDO)
            READWRITE(VARINT(nUndoPos));
        if (nStatus & BLOCK_HAVE_UNDO_CLAIM)
            READWRITE(VARINT(nUndoPosClaim));

        // block header
        READWRITE(this->nVersion);
        READWRITE(hashPrev);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    )

    uint256 GetBlockHash() const
    {
    	Bitcoin_CBlockHeader block;
        block.nVersion        = nVersion;
        block.hashPrevBlock   = hashPrev;
        block.hashMerkleRoot  = hashMerkleRoot;
        block.nTime           = nTime;
        block.nBits           = nBits;
        block.nNonce          = nNonce;
        return block.GetHash();
    }


    std::string ToString() const
    {
        std::string str = "CDiskBlockIndex(";
        str += Bitcoin_CBlockIndex::ToString();
        str += strprintf("\n                hashBlock=%s, hashPrev=%s)",
            GetBlockHash().ToString(),
            hashPrev.ToString());
        return str;
    }

    void print() const
    {
        LogPrintf("%s\n", ToString());
    }
};

/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,   // everything ok
        MODE_INVALID, // network rule violation (DoS value may be set)
        MODE_ERROR,   // run-time error
    } mode;
    int nDoS;
    std::string strRejectReason;
    unsigned char chRejectCode;
    bool corruptionPossible;
public:
    CValidationState() : mode(MODE_VALID), nDoS(0), corruptionPossible(false) {}
    bool DoS(int level, bool ret = false,
             unsigned char chRejectCodeIn=0, std::string strRejectReasonIn="",
             bool corruptionIn=false) {
        chRejectCode = chRejectCodeIn;
        strRejectReason = strRejectReasonIn;
        corruptionPossible = corruptionIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret = false,
                 unsigned char _chRejectCode=0, std::string _strRejectReason="") {
        return DoS(0, ret, _chRejectCode, _strRejectReason);
    }
    bool Error(std::string strRejectReasonIn="") {
        if (mode == MODE_VALID)
            strRejectReason = strRejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool Abort(const std::string &msg) {
        AbortNode(msg);
        return Error(msg);
    }
    bool IsValid() const {
        return mode == MODE_VALID;
    }
    bool IsInvalid() const {
        return mode == MODE_INVALID;
    }
    bool IsError() const {
        return mode == MODE_ERROR;
    }
    bool IsInvalid(int &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const {
        return corruptionPossible;
    }
    unsigned char GetRejectCode() const { return chRejectCode; }
    std::string GetRejectReason() const { return strRejectReason; }
};

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class Bitcoin_CVerifyDB {
public:

	Bitcoin_CVerifyDB();
    ~Bitcoin_CVerifyDB();
    bool VerifyDB(int nCheckLevel, int nCheckDepth);
};

/** An in-memory indexed chain of blocks. */
class Bitcoin_CChain : public CChain {
public:
    /** Returns the index entry for the genesis block of this chain, or NULL if none. */
    Bitcoin_CBlockIndex *Genesis() const {
        return (Bitcoin_CBlockIndex*)(vChain.size() > 0 ? vChain[0] : NULL);
    }

    /** Returns the index entry at a particular height in this chain, or NULL if no such height exists. */
    Bitcoin_CBlockIndex *operator[](int nHeight) const {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return NULL;
        return (Bitcoin_CBlockIndex*)vChain[nHeight];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const Bitcoin_CChain &a, const Bitcoin_CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.vChain.size() - 1] == b.vChain[b.vChain.size() - 1];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndexBase *pindex) const {
        return (*this)[pindex->nHeight] == pindex;
    }

    /** Find the successor of a block in this chain, or NULL if the given index is not found or is the tip. */
    Bitcoin_CBlockIndex *Next(const Bitcoin_CBlockIndex *pindex) const {
        if (Contains(pindex))
            return (*this)[pindex->nHeight + 1];
        else
            return NULL;
    }

    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndexBase *SetTip(CBlockIndexBase *pindex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndexBase *pindex = NULL) const;

    /** Find the last common block between this chain and a locator. */
    Bitcoin_CBlockIndex *FindFork(const CBlockLocator &locator) const;
};

/** The currently-connected chain of blocks. */
extern Bitcoin_CChain bitcoin_chainActive;

/** The currently best known chain of headers (some of which may be invalid). */
extern Bitcoin_CChain bitcoin_chainMostWork;

/** Global variable that points to the active Bitcoin_CCoinsView (protected by cs_main) */
extern Bitcoin_CCoinsViewCache *bitcoin_pcoinsTip;

/** Global variable that points to the active block tree (protected by cs_main) */
extern Bitcoin_CBlockTreeDB *bitcoin_pblocktree;

struct Bitcoin_CBlockTemplate
{
	Bitcoin_CBlock block;
    std::vector<int64_t> vTxFees;
    std::vector<int64_t> vTxSigOps;
};






/** Used to relay blocks as header + vector<merkle branch>
 * to filtered nodes.
 */
class Bitcoin_CMerkleBlock
{
public:
    // Public only for unit testing
	Bitcoin_CBlockHeader header;
    CPartialMerkleTree txn;

public:
    // Public only for unit testing and relay testing
    // (not relayed)
    std::vector<std::pair<unsigned int, uint256> > vMatchedTxn;

    // Create from a CBlock, filtering transactions according to filter
    // Note that this will call IsRelevantAndUpdate on the filter for each transaction,
    // thus the filter will likely be modified.
    Bitcoin_CMerkleBlock(const Bitcoin_CBlock& block, CBloomFilter& filter);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(header);
        READWRITE(txn);
    )
};


class Bitcoin_CWalletInterface {
protected:
    virtual void SyncTransaction(const uint256 &hash, const Bitcoin_CTransaction &tx, const Bitcoin_CBlock *pblock) =0;
    virtual void EraseFromWallet(const uint256 &hash) =0;
    virtual void SetBestChain(const CBlockLocator &locator) =0;
    virtual void UpdatedTransaction(const uint256 &hash) =0;
    virtual void Inventory(const uint256 &hash) =0;
    virtual void ResendWalletTransactions() =0;
    friend void ::Bitcoin_RegisterWallet(Bitcoin_CWalletInterface*);
    friend void ::Bitcoin_UnregisterWallet(Bitcoin_CWalletInterface*);
    friend void ::Bitcoin_UnregisterAllWallets();
};

#endif
