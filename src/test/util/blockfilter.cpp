// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/blockfilter.h>

#include <chainparams.h>
#include <node/blockstorage.h>
#include <validation.h>

using node::ReadBlockFromDisk;
using node::UndoReadFromDisk;

bool ComputeFilter(BlockFilterType filter_type, const CBlockIndex* block_index, BlockFilter& filter)
{
    LOCK(::cs_main);

    CBlock block;
    if (!ReadBlockFromDisk(block, block_index->GetBlockPos(), Params().GetConsensus())) {
        return false;
    }

    CBlockUndo block_undo;
    if (block_index->nHeight > 0 && !UndoReadFromDisk(block_undo, block_index)) {
        return false;
    }

    filter = BlockFilter(filter_type, block, block_undo);
    return true;
}
