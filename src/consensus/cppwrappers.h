// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CPPWRAPPERS_H
#define BITCOIN_CONSENSUS_CPPWRAPPERS_H

#include "header_verify.h"
#include "interfaces.h"
#include "pow.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class CValidationState;
namespace Consensus { struct Params; };

inline unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& consensusParams)
{
    return PowGetNextWorkRequired(pindexLast, CreateCoreIndexInterface(), pblock, consensusParams);
}

inline unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& consensusParams)
{
    return PowCalculateNextWorkRequired(pindexLast, CreateCoreIndexInterface(), nFirstBlockTime, consensusParams);
}

inline bool ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev, int64_t nAdjustedTime)
{
    return ContextualCheckHeader(block, state, consensusParams, pindexPrev, CreateCoreIndexInterface(), nAdjustedTime);
}

#endif // BITCOIN_CONSENSUS_CPPWRAPPERS_H
