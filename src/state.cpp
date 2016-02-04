// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "state.h"

int nNodeMode = NT_FULL;
int nNodeState = NS_STARTUP;

int nMaxThinPeers = 8;
int nBloomFilterElements = 1536;
int nMinStakeInterval = 0;         // in seconds, min time between successful stakes
int nThinIndexWindow = 4096;        // no. of block headers to keep in memory

// -- services provided by local node, initialise to all on
uint64_t nLocalServices     = 0 | NODE_NETWORK | THIN_SUPPORT | THIN_STEALTH | SMSG_RELAY;
uint32_t nLocalRequirements = 0 | NODE_NETWORK;


bool fTestNet = false;
bool fDebug = false;
bool fDebugNet = false;
bool fDebugSmsg = false;
bool fDebugChain = false;
bool fDebugRingSig = false;
bool fDebugPoS = false;
bool fNoSmsg = false;
bool fPrintToConsole = false;
bool fPrintToDebugLog = true;
//bool fShutdown = false;
bool fDaemon = false;
bool fServer = false;
bool fCommandLine = false;
std::string strMiscWarning;
bool fNoListen = false;
bool fLogTimestamps = false;
bool fReopenDebugLog = false;
bool fThinFullIndex = false; // when in thin mode don't keep all headers in memory
bool fReindexing = false;
bool fHaveGUI = false;
volatile bool fIsStaking = false; // looks at stake weight also

bool fConfChange;
bool fEnforceCanonical;
unsigned int nNodeLifespan;
unsigned int nDerivationMethodIndex;
unsigned int nMinerSleep;
unsigned int nBlockMaxSize;
unsigned int nBlockPrioritySize;
unsigned int nBlockMinSize;
int64_t nMinTxFee = MIN_TX_FEE;


unsigned int nStakeSplitAge = 1 * 24 * 60 * 60;
int64_t nStakeCombineThreshold = 1000 * COIN;



