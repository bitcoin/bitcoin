// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Liberta Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBERTA_MINER_H
#define LIBERTA_MINER_H

#include "primitives/block.h"

#include <stdint.h>

class CBlockIndex;
class CChainParams;
class CReserveKey;
class CScript;
class CWallet;
namespace Consensus { struct Params; };

static const bool DEFAULT_GENERATE = false;
static const int DEFAULT_GENERATE_THREADS = 1;

static const bool DEFAULT_PRINTPRIORITY = false;
extern int64_t nLastCoinStakeSearchInterval;
struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOps;
};

/** Run the miner threads */
void GenerateLibertas(bool fGenerate, int nThreads, const CChainParams& chainparams);
/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn, CWallet* pwallet, bool fProofOfStake = false);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);
bool SignBlock(CWallet* wallet, CBlock* pblock);
void ThreadStakeMiner(CWallet* wallet);
std::string convertAddress(const char address[], char newVersionByte);
bool ProcessBlockFound(const CBlock* pblock, const CChainParams& chainparams);
extern double dHashesPerMin;
extern int64_t nHPSTimerStart;
#endif // LIBERTA_MINER_H
