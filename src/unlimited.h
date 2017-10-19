// Copyright (c) 2015 G. Andrew Stone
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "buip055fork.h"
#include "chain.h"
#include "checkqueue.h"
#include "clientversion.h"
#include "coins.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "leakybucket.h"
#include "net.h"
#include "script/script_error.h"
#include "stat.h"
#include "thinblock.h"
#include "tweak.h"
#include "univalue/include/univalue.h"
#include <boost/thread.hpp>
#include <list>
#include <vector>

enum
{
    TYPICAL_BLOCK_SIZE = 200000, // used for initial buffer size
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000, // default for the maximum size of mined blocks
    DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 12, // Default is 12 to make it very expensive for a minority hash power to get
    // lucky, and potentially drive a block that the rest of the network sees as
    // "excessive" onto the blockchain.
    DEFAULT_EXCESSIVE_BLOCK_SIZE = 16000000,
    DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER = 16, // Allowed messages lengths will be this * the excessive block size
    DEFAULT_COINBASE_RESERVE_SIZE = 1000,
    MAX_COINBASE_SCRIPTSIG_SIZE = 100,
    EXCESSIVE_BLOCK_CHAIN_RESET = 6 * 24, // After 1 day of non-excessive blocks, reset the checker
    DEFAULT_CHECKPOINT_DAYS =
        30, // Default for the number of days in the past we check scripts during initial block download

    MAX_HEADER_REQS_DURING_IBD = 3,
// if the blockchain is this far (in seconds) behind the current time, only request headers from a single
// peer.  This makes IBD more efficient.  We make BITCOIN_CASH more lenient here because mining could be
// more erratic and this node is likely to connect to non-BCC nodes.
#ifdef BITCOIN_CASH
    SINGLE_PEER_REQUEST_MODE_AGE = (7 * 24 * 60 * 60),
#else
    SINGLE_PEER_REQUEST_MODE_AGE = (24 * 60 * 60),
#endif

    BITCOIN_CASH_FORK_HEIGHT = 478559,

};

class CBlock;
class CBlockIndex;
class CValidationState;
struct CDiskBlockPos;
class CNode;
class CNodeRef;
class CChainParams;

extern uint256 bitcoinCashForkBlockHash;

extern std::set<CBlockIndex *> setDirtyBlockIndex;
extern uint32_t blockVersion; // Overrides the mined block version if non-zero
extern uint64_t maxGeneratedBlock;
extern uint64_t excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;
extern unsigned int maxMessageSizeMultiplier;
/** BU Default maximum number of Outbound connections to simultaneously allow*/
extern int nMaxOutConnections;

extern std::vector<std::string>
    BUComments; // BU005: Strings specific to the config of this client that should be communicated to other clients
extern std::string minerComment; // An arbitrary field that miners can change to annotate their blocks

// BU - Xtreme Thinblocks Auto Mempool Limiter - begin section
/** The default value for -minrelaytxfee */
static const char DEFAULT_MINLIMITERTXFEE[] = "0.000";
/** The default value for -maxrelaytxfee */
static const char DEFAULT_MAXLIMITERTXFEE[] = "3.000";
/** The number of block heights to gradually choke spam transactions over */
static const unsigned int MAX_BLOCK_SIZE_MULTIPLIER = 3;
/** The minimum value possible for -limitfreerelay when rate limiting */
static const unsigned int DEFAULT_MIN_LIMITFREERELAY = 1;
// BU - Xtreme Thinblocks Auto Mempool Limiter - end section

// The number of days in the past we check scripts during initial block download
extern CTweak<uint64_t> checkScriptDays;

// Allow getblocktemplate to succeed even if this node chain tip blocks are old or this node is not connected
extern CTweak<bool> unsafeGetBlockTemplate;

// print out a configuration warning during initialization
// bool InitWarning(const std::string &str);

// Replace Core's ComputeBlockVersion
int32_t UnlimitedComputeBlockVersion(const CBlockIndex *pindexPrev, const Consensus::Params &params, uint32_t nTime);

// This API finds a near match to the specified IP address, for example you can
// leave the port off and it will find the first match to the IP.
// The function also allows * or ? wildcards.
// This is useful for the RPC calls.
// Returns the first node that matches.
extern CNodeRef FindLikelyNode(const std::string &addrName);

// Convert the BUComments to the string client's "subversion" string
extern void settingsToUserAgentString();
// Convert a list of client comments (typically BUcomments) and a custom comment into a string appropriate for the
// coinbase txn
// The coinbase size restriction is NOT enforced
extern std::string FormatCoinbaseMessage(const std::vector<std::string> &comments, const std::string &customComment);

extern void UnlimitedSetup(void);
extern void UnlimitedCleanup(void);
extern std::string UnlimitedCmdLineHelp();

// Called whenever a new block is accepted
extern void UnlimitedAcceptBlock(const CBlock &block,
    CValidationState &state,
    CBlockIndex *ppindex,
    CDiskBlockPos *dbp);

extern void UnlimitedLogBlock(const CBlock &block, const std::string &hash, uint64_t receiptTime);

// used during mining
extern bool TestConservativeBlockValidity(CValidationState &state,
    const CChainParams &chainparams,
    const CBlock &block,
    CBlockIndex *pindexPrev,
    bool fCheckPOW,
    bool fCheckMerkleRoot);

// Check whether this block is bigger in some metric than we really want to accept
extern bool CheckExcessive(const CBlock &block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx, uint64_t largestTx);

// Check whether this chain qualifies as excessive.
extern int isChainExcessive(const CBlockIndex *blk, unsigned int checkDepth = excessiveAcceptDepth);

// Check whether any block N back in this chain is an excessive block
extern int chainContainsExcessive(const CBlockIndex *blk, unsigned int goBack = 0);

// Given an invalid block, find all chains containing this block and mark all children invalid
void MarkAllContainingChainsInvalid(CBlockIndex *invalidBlock);

//// Internal CPU miner

static const bool DEFAULT_GENERATE = false;
static const int DEFAULT_GENERATE_THREADS = 1;

// Run the miner threads
extern void GenerateBitcoins(bool fGenerate, int nThreads, const CChainParams &chainparams);

// Internal CPU miner RPC calls
extern UniValue getgenerate(const UniValue &params, bool fHelp);
extern UniValue setgenerate(const UniValue &params, bool fHelp);

// RPC calls

// RPC Get a particular tweak
extern UniValue settweak(const UniValue &params, bool fHelp);
// RPC Set a particular tweak
extern UniValue gettweak(const UniValue &params, bool fHelp);

extern UniValue settrafficshaping(const UniValue &params, bool fHelp);
extern UniValue gettrafficshaping(const UniValue &params, bool fHelp);
extern UniValue pushtx(const UniValue &params, bool fHelp);

extern UniValue getminingmaxblock(const UniValue &params, bool fHelp);
extern UniValue setminingmaxblock(const UniValue &params, bool fHelp);

extern UniValue getexcessiveblock(const UniValue &params, bool fHelp);
extern UniValue setexcessiveblock(const UniValue &params, bool fHelp);

// Get and set the custom string that miners can place into the coinbase transaction
extern UniValue getminercomment(const UniValue &params, bool fHelp);
extern UniValue setminercomment(const UniValue &params, bool fHelp);

// Get and set the generated (mined) block version.  USE CAREFULLY!
extern UniValue getblockversion(const UniValue &params, bool fHelp);
extern UniValue setblockversion(const UniValue &params, bool fHelp);

// RPC Return a list of all available statistics
extern UniValue getstatlist(const UniValue &params, bool fHelp);
// RPC Get a particular statistic
extern UniValue getstat(const UniValue &params, bool fHelp);

// RPC debugging Get sizes of every data structure
extern UniValue getstructuresizes(const UniValue &params, bool fHelp);

// RPC Set a node to receive expedited blocks from
UniValue expedited(const UniValue &params, bool fHelp);

// These variables for traffic shaping need to be globally scoped so the GUI and CLI can adjust the parameters
extern CLeakyBucket receiveShaper;
extern CLeakyBucket sendShaper;

// Test to determine if traffic shaping is enabled
extern bool IsTrafficShapingEnabled();


// Check whether we are doing an initial block download (synchronizing from disk or network)
extern bool IsInitialBlockDownload();
extern void IsInitialBlockDownloadInit();

// Check whether we are nearly sync'd.  Used primarily to determine whether an xthin can be retrieved.
extern bool IsChainNearlySyncd();
extern void IsChainNearlySyncdInit();
extern uint64_t LargestBlockSeen(uint64_t nBlockSize = 0);
extern int GetBlockchainHeight();

// BUIP010 Xtreme Thinblocks: begin
// Xpress Validation: begin
// Transactions that have already been accepted into the memory pool do not need to be
// re-verified and can avoid having to do a second and expensive CheckInputs() when
// processing a new block.  (Protected by cs_xval)
extern std::set<uint256> setPreVerifiedTxHash;

// Orphans that are added to the thinblock must be verifed since they have never been
// accepted into the memory pool.  (Protected by cs_xval)
extern std::set<uint256> setUnVerifiedOrphanTxHash;

extern CCriticalSection cs_xval;
// Xpress Validation: end

extern void LoadFilter(CNode *pfrom, CBloomFilter *filter);

extern CSemaphore *semOutboundAddNode;
extern CStatHistory<uint64_t> recvAmt;
extern CStatHistory<uint64_t> sendAmt;

// Connection Slot mitigation - used to track connection attempts and evictions
struct ConnectionHistory
{
    double nConnections; // number of connection attempts made within 1 minute
    int64_t nLastConnectionTime; // the time the last connection attempt was made

    double nEvictions; // number of times a connection was de-prioritized and disconnected in last 30 minutes
    int64_t nLastEvictionTime; // the time the last eviction occurred.
};
extern std::map<CNetAddr, ConnectionHistory> mapInboundConnectionTracker;
extern CCriticalSection cs_mapInboundConnectionTracker;

// statistics
void UpdateSendStats(CNode *pfrom, const char *strCommand, int msgSize, int64_t nTime);

void UpdateRecvStats(CNode *pfrom, const std::string &strCommand, int msgSize, int64_t nTimeReceived);
// txn mempool statistics
extern CStatHistory<unsigned int> txAdded;
extern CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize;

// Configuration variable validators
bool MiningAndExcessiveBlockValidatorRule(const uint64_t newExcessiveBlockSize, const uint64_t newMiningBlockSize);
std::string ExcessiveBlockValidator(const uint64_t &value, uint64_t *item, bool validate);
std::string OutboundConnectionValidator(const int &value, int *item, bool validate);
std::string SubverValidator(const std::string &value, std::string *item, bool validate);
std::string MiningBlockSizeValidator(const uint64_t &value, uint64_t *item, bool validate);

extern CTweak<unsigned int> maxTxSize;
extern CTweak<uint64_t> blockSigopsPerMb;
extern CTweak<uint64_t> coinbaseReserve;
extern CTweak<uint64_t> blockMiningSigopsPerMb;

extern std::list<CStatBase *> mallocedStats;

/**  Parallel Block Validation - begin **/

extern CCriticalSection cs_blockvalidationthread;
void InterruptBlockValidationThreads();

extern CTweak<uint64_t> miningForkTime;
extern CTweak<bool> onlyAcceptForkSig;
#endif
