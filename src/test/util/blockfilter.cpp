// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/blockfilter.h>

#include <chainparams.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <undo.h>

using node::BlockManager;

bool ComputeFilter(BlockFilterType filter_type, const CBlockIndex& block_index, BlockFilter& filter, const BlockManager& blockman)
{
    CBlock block;

    {
        LOCK_SHARED(g_cs_blockindex_data); // keep lock until we finish reading the block from disk
        if (!blockman.ReadBlockFromDisk(block, block_index.GetFilePos(/*is_undo=*/false))) {
            return false;
        }
    }

    CBlockUndo block_undo;
    if (block_index.nHeight > 0 && !blockman.UndoReadFromDisk(block_undo, block_index)) {
        return false;
    }

    filter = BlockFilter(filter_type, block, block_undo);
    return true;
}
