// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSTANTS_H
#define BITCOIN_CONSTANTS_H

#include <cstdint>

/* One gigabyte (GB) in bytes */
static constexpr uint64_t GB_BYTES{1000000000};

/* One mebibyte (MiB) in bytes */
static constexpr int32_t MIB_BYTES{1024 * 1024};

// Require that user allocate at least 550 MiB for block & undo files (blk???.dat and rev???.dat)
// At 1MB per block, 288 blocks = 288MB.
// Add 15% for Undo data = 331MB
// Add 20% for Orphan block rate = 397MB
// We want the low water mark after pruning to be at least 397 MB and since we prune in
// full block file chunks, we need the high water mark which triggers the prune to be
// one 128MB block file + added 15% undo data = 147MB greater for a total of 545MB
// Setting the target to >= 550 MiB will make it likely we can respect the target.
static constexpr int32_t MIN_MIB_FOR_BLOCK_FILES{550};
static constexpr int32_t MIN_BYTES_FOR_BLOCK_FILES{MIN_MIB_FOR_BLOCK_FILES * MIB_BYTES};

#endif // BITCOIN_CONSTANTS_H
