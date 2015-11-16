// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "leakybucket.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

enum
{
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000
};

class CBlock;
class CBlockIndex;
class CValidationState;
class CDiskBlockPos;

extern uint64_t maxGeneratedBlock;
extern unsigned int excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;

extern void UnlimitedSetup(void);

// Called whenever a new block is accepted
extern void UnlimitedAcceptBlock(const CBlock& block, CValidationState& state, CBlockIndex* ppindex, CDiskBlockPos* dbp);

// Check whether this block is bigger in some metric than we really want to accept
extern bool CheckExcessive(const CBlock& block,uint64_t blockSize, uint64_t nSigOps,uint64_t nTx);

// Check whether any block N back in this chain is an excessive block
extern int isChainExcessive(const CBlockIndex* blk,unsigned int checkDepth = excessiveAcceptDepth);

// RPC calls
extern json_spirit::Value settrafficshaping(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettrafficshaping(const json_spirit::Array& params, bool fHelp);


// These variables for traffic shaping need to be globally scoped so the GUI and CLI can adjust the parameters
extern CLeakyBucket receiveShaper;
extern CLeakyBucket sendShaper;


#endif
