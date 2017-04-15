// Copyright (c) 2015 G. Andrew Stone
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "tweak.h"
#include "leakybucket.h"
#include "net.h"
#include "stat.h"
#include "thinblock.h"
#include "consensus/validation.h"
#include "consensus/params.h"
#include "requestManager.h"
#include <univalue.h>
#include <vector>

enum {
    TYPICAL_BLOCK_SIZE = 200000,   // used for initial buffer size
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000,  // default for the maximum size of mined blocks
    DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 12,  // Default is 12 to make it very expensive for a minority hash power to get lucky, and potentially drive a block that the rest of the network sees as "excessive" onto the blockchain.
    DEFAULT_EXCESSIVE_BLOCK_SIZE = 16000000,
    DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER = 16,    // Allowed messages lengths will be this * the excessive block size
    DEFAULT_COINBASE_RESERVE_SIZE = 1000,
    MAX_COINBASE_SCRIPTSIG_SIZE = 100,
    EXCESSIVE_BLOCK_CHAIN_RESET = 6*24,  // After 1 day of non-excessive blocks, reset the checker
};

class CBlock;
class CBlockIndex;
class CValidationState;
struct CDiskBlockPos;
class CNode;
class CChainParams;


extern uint32_t blockVersion;  // Overrides the mined block version if non-zero
extern uint64_t maxGeneratedBlock;
extern unsigned int excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;
extern unsigned int maxMessageSizeMultiplier;
/** BU Default maximum number of Outbound connections to simultaneously allow*/
extern int nMaxOutConnections;

extern std::vector<std::string> BUComments;  // BU005: Strings specific to the config of this client that should be communicated to other clients
extern std::string minerComment;  // An arbitrary field that miners can change to annotate their blocks

// BU - Xtreme Thinblocks Auto Mempool Limiter - begin section
/** The default value for -minrelaytxfee */
static const char DEFAULT_MINLIMITERTXFEE[] = "0.0";
/** The default value for -maxrelaytxfee */
static const char DEFAULT_MAXLIMITERTXFEE[] = "3.0";
/** The number of block heights to gradually choke spam transactions over */
static const unsigned int MAX_BLOCK_SIZE_MULTIPLIER = 3;
/** The minimum value possible for -limitfreerelay when rate limiting */
static const unsigned int DEFAULT_MIN_LIMITFREERELAY = 1;
// BU - Xtreme Thinblocks Auto Mempool Limiter - end section

// print out a configuration warning during initialization
// bool InitWarning(const std::string &str);

// Replace Core's ComputeBlockVersion
int32_t UnlimitedComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params,uint32_t nTime);

// This API finds a near match to the specified IP address, for example you can
// leave the port off and it will find the first match to the IP.
// The function also allows * or ? wildcards.
// This is useful for the RPC calls.
// Returns the first node that matches.
CNode* FindLikelyNode(const std::string& addrName);

// process incoming unsolicited block
bool HandleExpeditedBlock(CDataStream& vRecv,CNode* pfrom);

// Convert the BUComments to the string client's "subversion" string
extern void settingsToUserAgentString();
// Convert a list of client comments (typically BUcomments) and a custom comment into a string appropriate for the coinbase txn
// The coinbase size restriction is NOT enforced
extern std::string FormatCoinbaseMessage(const std::vector<std::string>& comments,const std::string& customComment);  

extern void UnlimitedSetup(void);
extern std::string UnlimitedCmdLineHelp();

// Called whenever a new block is accepted
extern void UnlimitedAcceptBlock(const CBlock& block, CValidationState& state, CBlockIndex* ppindex, CDiskBlockPos* dbp);

extern void UnlimitedLogBlock(const CBlock& block, const std::string& hash, uint64_t receiptTime);

// used during mining
extern bool TestConservativeBlockValidity(CValidationState& state, const CChainParams& chainparams, const CBlock& block, CBlockIndex* pindexPrev, bool fCheckPOW, bool fCheckMerkleRoot);

// Check whether this block is bigger in some metric than we really want to accept
extern bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx,uint64_t largestTx);

// Check whether this chain qualifies as excessive.
extern int isChainExcessive(const CBlockIndex* blk, unsigned int checkDepth = excessiveAcceptDepth);

// Check whether any block N back in this chain is an excessive block
extern int chainContainsExcessive(const CBlockIndex* blk, unsigned int goBack=0);

// RPC calls

// RPC Get a particular tweak
extern UniValue settweak(const UniValue& params, bool fHelp);
// RPC Set a particular tweak
extern UniValue gettweak(const UniValue& params, bool fHelp);

extern UniValue settrafficshaping(const UniValue& params, bool fHelp);
extern UniValue gettrafficshaping(const UniValue& params, bool fHelp);
extern UniValue pushtx(const UniValue& params, bool fHelp);

extern UniValue getminingmaxblock(const UniValue& params, bool fHelp);
extern UniValue setminingmaxblock(const UniValue& params, bool fHelp);

extern UniValue getexcessiveblock(const UniValue& params, bool fHelp);
extern UniValue setexcessiveblock(const UniValue& params, bool fHelp);

// Get and set the custom string that miners can place into the coinbase transaction
extern UniValue getminercomment(const UniValue& params, bool fHelp);
extern UniValue setminercomment(const UniValue& params, bool fHelp);

// Get and set the generated (mined) block version.  USE CAREFULLY!
extern UniValue getblockversion(const UniValue& params, bool fHelp);
extern UniValue setblockversion(const UniValue& params, bool fHelp);

// RPC Return a list of all available statistics
extern UniValue getstatlist(const UniValue& params, bool fHelp);
// RPC Get a particular statistic
extern UniValue getstat(const UniValue& params, bool fHelp);

// RPC Set a node to receive expedited blocks from
UniValue expedited(const UniValue& params, bool fHelp);

// These variables for traffic shaping need to be globally scoped so the GUI and CLI can adjust the parameters
extern CLeakyBucket receiveShaper;
extern CLeakyBucket sendShaper;

// Test to determine if traffic shaping is enabled
extern bool IsTrafficShapingEnabled();

extern CCriticalSection cs_ischainnearlysyncd;

extern bool IsChainNearlySyncd();
extern void IsChainNearlySyncdInit();
extern uint64_t LargestBlockSeen(uint64_t nBlockSize = 0);
extern void LoadFilter(CNode *pfrom, CBloomFilter *filter);
extern void HandleBlockMessage(CNode *pfrom, const std::string &strCommand, CBlock &block, const CInv &inv);

extern bool CheckAndRequestExpeditedBlocks(CNode* pfrom);  // Checks to see if the node is configured in bitcoin.conf to be an expedited block source and if so, request them.
extern void SendExpeditedBlock(CXThinBlock& thinBlock,unsigned char hops, const CNode* skip=NULL);
extern void SendExpeditedBlock(const CBlock& block,const CNode* skip=NULL);
extern void HandleExpeditedRequest(CDataStream& vRecv,CNode* pfrom);
extern bool IsRecentlyExpeditedAndStore(const uint256& hash);


extern CSemaphore*  semOutboundAddNode;
extern std::vector<CNode*> xpeditedBlk; // Who requested expedited blocks from us
extern std::vector<CNode*> xpeditedBlkUp; // Who we requested expedited blocks from
extern std::vector<CNode*> xpeditedTxn;
extern CStatHistory<uint64_t > recvAmt; 
extern CStatHistory<uint64_t > sendAmt; 

// Connection Slot mitigation - used to track connection attempts and evictions
struct ConnectionHistory
{
    double nConnections;      // number of connection attempts made within 1 minute
    int64_t nLastConnectionTime;  // the time the last connection attempt was made

    double nEvictions;        // number of times a connection was de-prioritized and disconnected in last 30 minutes
    int64_t nLastEvictionTime;    // the time the last eviction occurred.
};
extern std::map<CNetAddr, ConnectionHistory > mapInboundConnectionTracker;
extern CCriticalSection cs_mapInboundConnectionTracker;

// statistics
void UpdateSendStats(CNode* pfrom, const char* strCommand, int msgSize, int64_t nTime);

void UpdateRecvStats(CNode* pfrom, const std::string& strCommand, int msgSize, int64_t nTimeReceived);
// txn mempool statistics
extern CStatHistory<unsigned int, MinValMax<unsigned int> > txAdded;
extern CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize;

// Configuration variable validators
bool MiningAndExcessiveBlockValidatorRule(const unsigned int newExcessiveBlockSize, const unsigned int newMiningBlockSize);
std::string ExcessiveBlockValidator(const unsigned int& value,unsigned int* item,bool validate);
std::string OutboundConnectionValidator(const int& value,int* item,bool validate);
std::string SubverValidator(const std::string& value,std::string* item,bool validate);
std::string MiningBlockSizeValidator(const uint64_t& value,uint64_t* item,bool validate);

extern CTweak<unsigned int> maxTxSize;
extern CTweak<uint64_t> blockSigopsPerMb;
extern CTweak<uint64_t> coinbaseReserve;
extern CTweak<uint64_t> blockMiningSigopsPerMb;

// Protocol changes:

enum {
  EXPEDITED_STOP   = 1,
  EXPEDITED_BLOCKS = 2,
  EXPEDITED_TXNS   = 4,
};

enum {
  EXPEDITED_MSG_HEADER   = 1,
  EXPEDITED_MSG_XTHIN    = 2,
};


#endif
