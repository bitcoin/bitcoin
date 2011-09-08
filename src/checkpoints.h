// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_CHECKPOINT_H
#define  BITCOIN_CHECKPOINT_H

class uint256;

//
// Block-chain checkpoints are compiled-in sanity checks.
// They are updated every release or three.
//
namespace Checkpoints
{
    // Returns true if block passes checkpoint checks
    bool CheckBlock(int nHeight, const uint256& hash);

    // Return conservative estimate of total number of blocks, 0 if unknown
    int GetTotalBlocksEstimate();
}

#endif
