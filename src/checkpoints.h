// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKPOINTS_H
#define BITCOIN_CHECKPOINTS_H

#include "uint256.h"

#include <map>

class CBlockIndex;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{
//! Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate(const CCheckpointData &data);

//! Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex *GetLastCheckpoint(const CCheckpointData &data);

double GuessVerificationProgress(const CCheckpointData &data, CBlockIndex *pindex, bool fSigchecks = true);

} // namespace Checkpoints

#endif // BITCOIN_CHECKPOINTS_H
