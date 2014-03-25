// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_CHECKPOINT_H
#define BITCOIN_BITCOIN_CHECKPOINT_H

#include <map>

class Bitcoin_CBlockIndex;
class uint256;

/** Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{
    // Returns true if block passes checkpoint checks
    bool Bitcoin_CheckBlock(int nHeight, const uint256& hash);

    // Return conservative estimate of total number of blocks, 0 if unknown
    int Bitcoin_GetTotalBlocksEstimate();

    // Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
    Bitcoin_CBlockIndex* Bitcoin_GetLastCheckpoint(const std::map<uint256, Bitcoin_CBlockIndex*>& mapBlockIndex);

    double Bitcoin_GuessVerificationProgress(Bitcoin_CBlockIndex *pindex, bool fSigchecks = true);

    extern bool bitcoin_fEnabled;
}

#endif
