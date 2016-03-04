// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "leakybucket.h"
#include <univalue.h>

enum {
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000,
    DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 4,
    DEFAULT_EXCESSIVE_BLOCK_SIZE = 16000000,
    DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER = 10,
    MAX_COINBASE_SCRIPTSIG_SIZE = 100
};

class CBlock;
class CBlockIndex;
class CValidationState;
class CDiskBlockPos;

extern uint64_t maxGeneratedBlock;
extern unsigned int excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;
extern unsigned int maxMessageSizeMultiplier;

extern std::vector<std::string> BUComments;  // BU005: Strings specific to the config of this client that should be communicated to other clients
extern std::string minerComment;  // An arbitrary field that miners can change to annotate their blocks
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

extern UniValue getminercomment(const UniValue& params, bool fHelp);
extern UniValue setminercomment(const UniValue& params, bool fHelp);

// These variables for traffic shaping need to be globally scoped so the GUI and CLI can adjust the parameters
extern CLeakyBucket receiveShaper;
extern CLeakyBucket sendShaper;


#endif
