// Copyright (c) 2015 G. Andrew Stone
// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "net.h"
#include "stat.h"
#include <univalue.h>
#include <vector>

class CBlock;
class CBlockIndex;
class CValidationState;
class CDiskBlockPos;
class CNode;

// Return a list of all available statistics
extern UniValue getstatlist(const UniValue& params, bool fHelp);
// Get a particular statistic
extern UniValue getstat(const UniValue& params, bool fHelp);

// BUIP010 Xtreme Thinblocks:
extern bool HaveConnectThinblockNodes();
extern bool HaveThinblockNodes();
extern bool CheckThinblockTimer(uint256 hash);
extern void ClearThinblockTimer(uint256 hash);
extern bool IsThinBlocksEnabled();
extern bool IsChainNearlySyncd();
extern void BuildSeededBloomFilter(CBloomFilter& memPoolFilter, std::vector<uint256>& vOrphanHashes);
extern void LoadFilter(CNode *pfrom, CBloomFilter *filter);
extern void HandleBlockMessage(CNode *pfrom, const std::string &strCommand, CBlock &block, const CInv &inv);
extern void ConnectToThinBlockNodes();
extern void CheckNodeSupportForThinBlocks();
extern void SendXThinBlock(CBlock &block, CNode* pfrom, const CInv &inv);

// Handle receiving and sending messages from thin block capable nodes only (so that thin block nodes capable nodes are preferred)
extern bool ThinBlockMessageHandler(std::vector<CNode*>& vNodesCopy);
extern std::map<uint256, uint64_t> mapThinBlockTimer;

// txn mempool statistics
extern CStatHistory<unsigned int, MinValMax<unsigned int> > txAdded;
extern CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize;

#endif
