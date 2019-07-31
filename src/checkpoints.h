// Copyright (c) 2009-2018 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINTALKCOIN_CHECKPOINTS_H
#define BITCOINTALKCOIN_CHECKPOINTS_H

#include <uint256.h>

#include <map>

class CBlockIndex;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{

//! Checks that the block hash at height nHeight matches the expected hardened checkpoint
bool CheckHardened(int nHeight, const uint256& hash, const CCheckpointData& data);

//! Returns last CBlockIndex* that is a checkpoint
CBlockIndex* GetLastCheckpoint(const CCheckpointData& data);

//! Returns last CBlockIndex* from the auto selected checkpoint
const CBlockIndex* AutoSelectSyncCheckpoint();

//! Check against automatically selected checkpoint
bool CheckSync(int nHeight);
} //namespace Checkpoints

#endif // BITCOINTALKCOIN_CHECKPOINTS_H
