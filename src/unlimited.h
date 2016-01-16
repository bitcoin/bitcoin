// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "leakybucket.h"
#include "net.h"
#include <univalue.h>

enum {
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000,
    DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 4,
    DEFAULT_EXCESSIVE_BLOCK_SIZE = 16000000,
    DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER = 10
};

class CBlock;
class CBlockIndex;
class CValidationState;
class CDiskBlockPos;
class CNode;

extern uint64_t maxGeneratedBlock;
extern unsigned int excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;
extern unsigned int maxMessageSizeMultiplier;

extern void UnlimitedSetup(void);
extern std::string UnlimitedCmdLineHelp();

// Called whenever a new block is accepted
extern void UnlimitedAcceptBlock(const CBlock& block, CValidationState& state, CBlockIndex* ppindex, CDiskBlockPos* dbp);

extern void UnlimitedLogBlock(const CBlock& block, const std::string& hash, uint64_t receiptTime);

// Check whether this block is bigger in some metric than we really want to accept
extern bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx);

// Check whether any block N back in this chain is an excessive block
extern int isChainExcessive(const CBlockIndex* blk, unsigned int checkDepth = excessiveAcceptDepth);

// RPC calls
extern UniValue settrafficshaping(const UniValue& params, bool fHelp);
extern UniValue gettrafficshaping(const UniValue& params, bool fHelp);
extern UniValue pushtx(const UniValue& params, bool fHelp);

extern UniValue getminingmaxblock(const UniValue& params, bool fHelp);
extern UniValue setminingmaxblock(const UniValue& params, bool fHelp);

extern UniValue getexcessiveblock(const UniValue& params, bool fHelp);
extern UniValue setexcessiveblock(const UniValue& params, bool fHelp);

// These variables for traffic shaping need to be globally scoped so the GUI and CLI can adjust the parameters
extern CLeakyBucket receiveShaper;
extern CLeakyBucket sendShaper;

// BUIP010 Xtreme Thinblocks:
extern bool HaveThinblockNodeConnections();
extern bool IsThinBlocksEnabled();
extern bool IsChainNearlySyncd();
extern void SendSeededBloomFilter(CNode *pto);
extern void HandleBlockMessage(CNode *pfrom, const std::string &strCommand, CBlock &block, const CInv &inv);
extern void ConnectToThinBlockNodes();
extern void CheckNodeSupportForThinBlocks();
extern void SendXThinBlock(CBlock &block, CNode* pfrom, const CInv &inv);

#endif
