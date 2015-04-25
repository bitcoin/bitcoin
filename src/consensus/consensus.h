// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class CValidationState;
class uint256;

namespace Consensus {

class Params;

} // namespace Consensus

/** Block header validation functions */

/**
 * Context-independent CBlockHeader validity checks
 */
bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, bool fCheckPOW = true);
/**
 * Context-dependent CBlockHeader validity checks
 */
bool ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, CBlockIndex *pindexPrev);

/** Block header validation utility functions */

uint32_t GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
uint32_t CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);
/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last Consensus::Params::nMajorityWindow blocks, starting at pstart and going backwards.
 */
bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned nRequired);

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
