// Copyright (c) 2016 Bitcoin Unlimited Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// All global variables that have construction/destruction dependencies
// must be placed in this file so that the ctor/dtor order is correct.

// Independent global variables may be placed here for organizational
// purposes.

#include "chain.h"
#include "clientversion.h"
#include "chainparams.h"
#include "miner.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "leakybucket.h"
#include "main.h"
#include "net.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "rpcserver.h"
#include "thinblock.h"
#include "timedata.h"
#include "tinyformat.h"
#include "txmempool.h"
#include "unlimited.h"
#include "utilstrencodings.h"
#include "ui_interface.h"
#include "util.h"
#include "validationinterface.h"
#include "alert.h"
#include "version.h"
#include "stat.h"
#include "tweak.h"
#include "addrman.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <boost/thread.hpp>
#include <inttypes.h>
#include <queue>


using namespace std;

#ifdef DEBUG_LOCKORDER
boost::mutex dd_mutex;
std::map<std::pair<void*, void*>, LockStack> lockorders;
boost::thread_specific_ptr<LockStack> lockstack;
#endif

// main.cpp CriticalSections:
CCriticalSection cs_LastBlockFile;
CCriticalSection cs_nBlockSequenceId;

CCriticalSection cs_nTimeOffset;
int64_t nTimeOffset = 0;

CCriticalSection cs_rpcWarmup;

CCriticalSection cs_main;
BlockMap mapBlockIndex;
CChain chainActive;
CWaitableCriticalSection csBestBlock;
CConditionVariable cvBlockChange;

proxyType proxyInfo[NET_MAX];
proxyType nameProxy;
CCriticalSection cs_proxyInfos;

map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

set<uint256> setPreVerifiedTxHash;
set<uint256> setUnVerifiedOrphanTxHash;
CCriticalSection cs_xval;
CCriticalSection cs_vNodes;
CCriticalSection cs_mapLocalHost;
map<CNetAddr, LocalServiceInfo> mapLocalHost;
std::vector<CSubNet> CNode::vWhitelistedRange;
CCriticalSection CNode::cs_vWhitelistedRange;
CCriticalSection CNode::cs_setBanned;
uint64_t CNode::nTotalBytesRecv = 0;
uint64_t CNode::nTotalBytesSent = 0;
CCriticalSection CNode::cs_totalBytesRecv;
CCriticalSection CNode::cs_totalBytesSent;

bool fIsChainNearlySyncd;
CCriticalSection cs_ischainnearlysyncd;

// critical sections from net.cpp
CCriticalSection cs_setservAddNodeAddresses;
CCriticalSection cs_vAddedNodes;
CCriticalSection cs_vUseDNSSeeds;
CCriticalSection cs_nLastNodeId;
CCriticalSection cs_mapInboundConnectionTracker;
CCriticalSection cs_vOneShots;

deque<string> vOneShots;
std::map<CNetAddr, ConnectionHistory> mapInboundConnectionTracker;
vector<std::string> vUseDNSSeeds;
vector<std::string> vAddedNodes;
set<CNetAddr> setservAddNodeAddresses;

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;
unsigned int excessiveBlockSize = DEFAULT_EXCESSIVE_BLOCK_SIZE;
unsigned int excessiveAcceptDepth = DEFAULT_EXCESSIVE_ACCEPT_DEPTH;
unsigned int maxMessageSizeMultiplier = DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER;
int nMaxOutConnections = DEFAULT_MAX_OUTBOUND_CONNECTIONS;

uint32_t blockVersion = 0;  // Overrides the mined block version if non-zero

std::vector<std::string> BUComments = std::vector<std::string>();
std::string minerComment;

// Variables for traffic shaping
CLeakyBucket receiveShaper(DEFAULT_MAX_RECV_BURST, DEFAULT_AVE_RECV);
CLeakyBucket sendShaper(DEFAULT_MAX_SEND_BURST, DEFAULT_AVE_SEND);
boost::chrono::steady_clock CLeakyBucket::clock;

// Variables for statistics tracking, must be before the "requester" singleton instantiation
const char* sampleNames[] = { "sec10", "min5", "hourly", "daily","monthly"};
int operateSampleCount[] = { 30,       12,   24,  30 };
int interruptIntervals[] = { 30,       30*12,   30*12*24,   30*12*24*30 };

CTxMemPool mempool(::minRelayTxFee);

boost::posix_time::milliseconds statMinInterval(10000);
boost::asio::io_service stat_io_service;

CStatMap statistics;
CTweakMap tweaks;

map<CInv, CDataStream> mapRelay;
deque<pair<int64_t, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
limitedmap<CInv, int64_t> mapAlreadyAskedFor(MAX_INV_SZ);

vector<CNode*> vNodes;
list<CNode*> vNodesDisconnected;
CSemaphore*  semOutbound = NULL;
CSemaphore*  semOutboundAddNode = NULL; // BU: separate semaphore for -addnodes
CNodeSignals g_signals;
CAddrMan addrman;

// BU: change locking of orphan map from using cs_main to cs_orphancache.  There is too much dependance on cs_main locks which
//     are generally too broad in scope.
CCriticalSection cs_orphancache;
map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(cs_orphancache);
map<uint256, set<uint256> > mapOrphanTransactionsByPrev GUARDED_BY(cs_orphancache);

CTweakRef<unsigned int> ebTweak("net.excessiveBlock","Excessive block size in bytes", &excessiveBlockSize,&ExcessiveBlockValidator);
CTweakRef<unsigned int> eadTweak("net.excessiveAcceptDepth","Excessive block chain acceptance depth in blocks", &excessiveAcceptDepth);
CTweakRef<int> maxOutConnectionsTweak("net.maxOutboundConnections","Maximum number of outbound connections", &nMaxOutConnections,&OutboundConnectionValidator);
CTweakRef<int> maxConnectionsTweak("net.maxConnections","Maximum number of connections connections",&nMaxConnections);
CTweakRef<unsigned int> triTweak("net.txRetryInterval","How long to wait in microseconds before requesting a transaction from another source", &MIN_TX_REQUEST_RETRY_INTERVAL);  // When should I request a tx from someone else (in microseconds). cmdline/bitcoin.conf: -txretryinterval
CTweakRef<unsigned int> briTweak("net.blockRetryInterval","How long to wait in microseconds before requesting a block from another source", &MIN_BLK_REQUEST_RETRY_INTERVAL); // When should I request a block from someone else (in microseconds). cmdline/bitcoin.conf: -blkretryinterval

CTweakRef<std::string> subverOverrideTweak("net.subversionOveride","If set, this field will override the normal subversion field.  This is useful if you need to hide your node.",&subverOverride,&SubverValidator);

CRequestManager requester;  // after the maps nodes and tweaks

CStatHistory<unsigned int, MinValMax<unsigned int> > txAdded; //"memPool/txAdded");
CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize; // "memPool/size",STAT_OP_AVE);
CStatHistory<uint64_t > recvAmt; 
CStatHistory<uint64_t > sendAmt; 

// Thin block statistics need to be located here to ensure that the "statistics" global variable is constructed first
CStatHistory<uint64_t> CThinBlockStats::nOriginalSize("thin/blockSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nThinSize("thin/thinSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nBlocks("thin/numBlocks", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nMempoolLimiterBytesSaved("nSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nTotalBloomFilterBytes("nSizeBloom", STAT_OP_SUM | STAT_KEEP);
CCriticalSection cs_thinblockstats;
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksInBound;
std::map<int64_t, int> CThinBlockStats::mapThinBlocksInBoundReRequestedTx;
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksOutBound;
std::map<int64_t, uint64_t> CThinBlockStats::mapBloomFiltersInBound;
std::map<int64_t, uint64_t> CThinBlockStats::mapBloomFiltersOutBound;
std::map<int64_t, double> CThinBlockStats::mapThinBlockResponseTime;
std::map<int64_t, double> CThinBlockStats::mapThinBlockValidationTime;



// Expedited blocks
std::vector<CNode*> xpeditedBlk; // (256,(CNode*)NULL);    // Who requested expedited blocks from us
std::vector<CNode*> xpeditedBlkUp; //(256,(CNode*)NULL);  // Who we requested expedited blocks from
std::vector<CNode*> xpeditedTxn; // (256,(CNode*)NULL);  

// BUIP010 Xtreme Thinblocks Variables
std::map<uint256, uint64_t> mapThinBlockTimer;

//! The largest block size that we have seen since startup
uint64_t nLargestBlockSeen=BLOCKSTREAM_CORE_MAX_BLOCK_SIZE; // BU - Xtreme Thinblocks
