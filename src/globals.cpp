// Copyright (c) 2016 Bitcoin Unlimited Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// All global variables that have construction/destruction dependencies
// must be placed in this file so that the ctor/dtor order is correct.

// Independent global variables may be placed here for organizational
// purposes.

#include "addrman.h"
#include "chain.h"
#include "chainparams.h"
#include "clientversion.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "dosman.h"
#include "leakybucket.h"
#include "main.h"
#include "miner.h"
#include "netbase.h"
#include "nodestate.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "requestManager.h"
#include "rpc/server.h"
#include "stat.h"
#include "thinblock.h"
#include "timedata.h"
#include "tinyformat.h"
#include "tweak.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"
#include "validationinterface.h"
#include "version.h"

#include <atomic>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <inttypes.h>
#include <iomanip>
#include <list>
#include <queue>

using namespace std;

#ifdef DEBUG_LOCKORDER
boost::mutex dd_mutex;
std::map<std::pair<void *, void *>, LockStack> lockorders;
boost::thread_specific_ptr<LockStack> lockstack;
#endif


std::atomic<bool> fIsInitialBlockDownload{false};

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

// moved from main.cpp (now part of nodestate.h)
std::map<uint256, pair<NodeId, std::list<QueuedBlock>::iterator> > mapBlocksInFlight;
std::map<NodeId, CNodeState> mapNodeState;

set<uint256> setPreVerifiedTxHash;
set<uint256> setUnVerifiedOrphanTxHash;
CCriticalSection cs_xval;
CCriticalSection cs_vNodes;
CCriticalSection cs_mapLocalHost;
map<CNetAddr, LocalServiceInfo> mapLocalHost;
uint64_t CNode::nTotalBytesRecv = 0;
uint64_t CNode::nTotalBytesSent = 0;
CCriticalSection CNode::cs_totalBytesRecv;
CCriticalSection CNode::cs_totalBytesSent;

// critical sections from net.cpp
CCriticalSection cs_setservAddNodeAddresses;
CCriticalSection cs_vAddedNodes;
CCriticalSection cs_vUseDNSSeeds;
CCriticalSection cs_mapInboundConnectionTracker;
CCriticalSection cs_vOneShots;

CCriticalSection cs_statMap;

deque<string> vOneShots;
std::map<CNetAddr, ConnectionHistory> mapInboundConnectionTracker;
vector<std::string> vUseDNSSeeds;
vector<std::string> vAddedNodes;
set<CNetAddr> setservAddNodeAddresses;

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;
uint64_t excessiveBlockSize = DEFAULT_EXCESSIVE_BLOCK_SIZE;
unsigned int excessiveAcceptDepth = DEFAULT_EXCESSIVE_ACCEPT_DEPTH;
unsigned int maxMessageSizeMultiplier = DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER;
int nMaxOutConnections = DEFAULT_MAX_OUTBOUND_CONNECTIONS;

uint32_t blockVersion = 0; // Overrides the mined block version if non-zero

std::vector<std::string> BUComments = std::vector<std::string>();
std::string minerComment;

CLeakyBucket receiveShaper(DEFAULT_MAX_RECV_BURST, DEFAULT_AVE_RECV);
CLeakyBucket sendShaper(DEFAULT_MAX_SEND_BURST, DEFAULT_AVE_SEND);
boost::chrono::steady_clock CLeakyBucket::clock;

// Variables for statistics tracking, must be before the "requester" singleton instantiation
const char *sampleNames[] = {"sec10", "min5", "hourly", "daily", "monthly"};
int operateSampleCount[] = {30, 12, 24, 30};
int interruptIntervals[] = {30, 30 * 12, 30 * 12 * 24, 30 * 12 * 24 * 30};

CTxMemPool mempool(::minRelayTxFee);

boost::posix_time::milliseconds statMinInterval(10000);
boost::asio::io_service stat_io_service;

std::list<CStatBase *> mallocedStats;
CStatMap statistics;
CTweakMap tweaks;

map<CInv, CDataStream> mapRelay;
deque<pair<int64_t, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
limitedmap<uint256, int64_t> mapAlreadyAskedFor(MAX_INV_SZ);

vector<CNode *> vNodes;
list<CNode *> vNodesDisconnected;
CSemaphore *semOutbound = NULL;
CSemaphore *semOutboundAddNode = NULL; // BU: separate semaphore for -addnodes
CNodeSignals g_signals;
CAddrMan addrman;
CDoSManager dosMan;

// BU: change locking of orphan map from using cs_main to cs_orphancache.  There is too much dependance on cs_main locks
// which are generally too broad in scope.
CCriticalSection cs_orphancache;
map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(cs_orphancache);
map<uint256, set<uint256> > mapOrphanTransactionsByPrev GUARDED_BY(cs_orphancache);

CTweakRef<uint64_t> ebTweak("net.excessiveBlock",
    "Excessive block size in bytes",
    &excessiveBlockSize,
    &ExcessiveBlockValidator);
CTweak<uint64_t> blockSigopsPerMb("net.excessiveSigopsPerMb",
    "Excessive effort per block, denoted in cost (# inputs * txsize) per MB",
    BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS);
CTweak<uint64_t> blockMiningSigopsPerMb("mining.excessiveSigopsPerMb",
    "Excessive effort per block, denoted in cost (# inputs * txsize) per MB",
    BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS);
CTweak<uint64_t> coinbaseReserve("mining.coinbaseReserve",
    "How much space to reserve for the coinbase transaction, in bytes",
    DEFAULT_COINBASE_RESERVE_SIZE);
CTweakRef<std::string> miningCommentTweak("mining.comment", "Include text in a block's coinbase.", &minerComment);
CTweakRef<uint64_t> miningBlockSize("mining.blockSize",
    "Maximum block size in bytes.  The maximum block size returned from 'getblocktemplate' will be this value minus "
    "mining.coinbaseReserve.",
    &maxGeneratedBlock,
    &MiningBlockSizeValidator);

#ifdef BITCOIN_CASH
CTweak<uint64_t> miningForkTime("mining.forkTime",
    "Time in seconds since the epoch to initiate a hard fork as per BUIP055.",
    1501590000); // Tue 1 Aug 2017 12:20:00 UTC, uahf-technical-spec.md REQ-2
#else
CTweak<uint64_t> miningForkTime("mining.forkTime",
    "Time in seconds since the epoch to initiate a hard fork as per BUIP055.",
    0);
#endif

CTweak<bool> unsafeGetBlockTemplate("mining.unsafeGetBlockTemplate",
    "Allow getblocktemplate to succeed even if the chain tip is old or this node is not connected to other nodes",
    false);

CTweak<uint64_t> miningForkEB("mining.forkExcessiveBlock",
    "Set the excessive block to this value at the time of the fork.",
    8000000); // 8MB, uahf-technical-spec.md REQ-4-1
CTweak<uint64_t> miningForkMG("mining.forkBlockSize",
    "Set the maximum block generation size to this value at the time of the fork.",
    2000000); // 2MB, uahf-technical-spec.md REQ-4-2

CTweak<bool> walletSignWithForkSig("wallet.useNewSig",
    "Once the fork occurs, sign transactions using the new signature scheme so that they will only be valid on the "
    "fork.",
    true);

CTweak<unsigned int> maxTxSize("net.excessiveTx", "Largest transaction size in bytes", DEFAULT_LARGEST_TRANSACTION);
CTweakRef<unsigned int> eadTweak("net.excessiveAcceptDepth",
    "Excessive block chain acceptance depth in blocks",
    &excessiveAcceptDepth);
CTweakRef<int> maxOutConnectionsTweak("net.maxOutboundConnections",
    "Maximum number of outbound connections",
    &nMaxOutConnections,
    &OutboundConnectionValidator);
CTweakRef<int> maxConnectionsTweak("net.maxConnections", "Maximum number of connections", &nMaxConnections);
CTweakRef<int> minXthinNodesTweak("net.minXthinNodes",
    "Minimum number of outbound xthin capable nodes to connect to",
    &nMinXthinNodes);
CTweakRef<int> minBitcoinCashNodesTweak("net.minBitcoinCashNodes",
    "Minimum number of outbound BitcoinCash capable nodes to connect to",
    &nMinBitcoinCashNodes);
// When should I request a tx from someone else (in microseconds). cmdline/bitcoin.conf: -txretryinterval
CTweakRef<unsigned int> triTweak("net.txRetryInterval",
    "How long to wait in microseconds before requesting a transaction from another source",
    &MIN_TX_REQUEST_RETRY_INTERVAL);
// When should I request a block from someone else (in microseconds). cmdline/bitcoin.conf: -blkretryinterval
CTweakRef<unsigned int> briTweak("net.blockRetryInterval",
    "How long to wait in microseconds before requesting a block from another source",
    &MIN_BLK_REQUEST_RETRY_INTERVAL);

CTweakRef<std::string> subverOverrideTweak("net.subversionOverride",
    "If set, this field will override the normal subversion field.  This is useful if you need to hide your node.",
    &subverOverride,
    &SubverValidator);

CTweak<CAmount> maxTxFee("wallet.maxTxFee",
    "Maximum total fees to use in a single wallet transaction or raw transaction; setting this too low may abort large "
    "transactions.",
    DEFAULT_TRANSACTION_MAXFEE);

/** Number of blocks that can be requested at any given time from a single peer. */
CTweak<unsigned int> maxBlocksInTransitPerPeer("net.maxBlocksInTransitPerPeer",
    "Number of blocks that can be requested at any given time from a single peer. 0 means use algorithm.",
    0);
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
CTweak<unsigned int> blockDownloadWindow("net.blockDownloadWindow",
    "How far ahead of our current height do we fetch? 0 means use algorithm.",
    0);

/** This is the initial size of CFileBuffer's RAM buffer during reindex.  A
larger size will result in a tiny bit better performance if blocks are that
size.
The real purpose of this parameter is to exhaustively test dynamic buffer resizes
during reindexing by allowing the size to be set to low and random values.
*/
CTweak<uint64_t> reindexTypicalBlockSize("reindex.typicalBlockSize",
    "Set larger than the typical block size.  The block data file's RAM buffer will initally be 2x this size.",
    TYPICAL_BLOCK_SIZE);

/** This is the initial size of CFileBuffer's RAM buffer during reindex.  A
larger size will result in a tiny bit better performance if blocks are that
size.
The real purpose of this parameter is to exhaustively test dynamic buffer resizes
during reindexing by allowing the size to be set to low and random values.
*/
CTweak<uint64_t> checkScriptDays("blockchain.checkScriptDays",
    "The number of days in the past we check scripts during initial block download.",
    DEFAULT_CHECKPOINT_DAYS);


CRequestManager requester; // after the maps nodes and tweaks

CStatHistory<unsigned int> txAdded; //"memPool/txAdded");
CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize; // "memPool/size",STAT_OP_AVE);
CStatHistory<uint64_t> recvAmt;
CStatHistory<uint64_t> sendAmt;
CStatHistory<uint64_t> nTxValidationTime("txValidationTime", STAT_OP_MAX | STAT_INDIVIDUAL);
CStatHistory<uint64_t> nBlockValidationTime("blockValidationTime", STAT_OP_MAX | STAT_INDIVIDUAL);
CCriticalSection cs_blockvalidationtime;

CThinBlockData thindata; // Singleton class
