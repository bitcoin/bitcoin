// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EXPEDITED_H
#define BITCOIN_EXPEDITED_H

#include "net.h"
#include "thinblock.h"

#include <univalue.h>
#include <vector>


enum
{
    EXPEDITED_STOP = 1,
    EXPEDITED_BLOCKS = 2,
    EXPEDITED_TXNS = 4,
};

enum
{
    EXPEDITED_MSG_HDR = 1,
    EXPEDITED_MSG_XTHIN = 2,
};


extern CCriticalSection cs_xpedited; // protects xpeditedBlk, xpeditedBlkUp and xpeditedTxn
extern std::vector<CNode *> xpeditedBlk; // Who requested expedited blocks from us
extern std::vector<CNode *> xpeditedBlkUp; // Who we requested expedited blocks from
extern std::vector<CNode *> xpeditedTxn;

// Checks to see if the node is configured in bitcoin.conf to
extern bool CheckAndRequestExpeditedBlocks(CNode *pfrom);

// be an expedited block source and if so, request them.
extern void SendExpeditedBlock(CXThinBlock &thinBlock, unsigned char hops, const CNode *skip = NULL);
extern void SendExpeditedBlock(const CBlock &block, const CNode *skip = NULL);
extern bool HandleExpeditedRequest(CDataStream &vRecv, CNode *pfrom);
extern bool IsRecentlyExpeditedAndStore(const uint256 &hash);

// process incoming unsolicited block
extern bool HandleExpeditedBlock(CDataStream &vRecv, CNode *pfrom);

// is this node an expedited node
extern bool IsExpeditedNode(const CNode *pfrom);

#endif
