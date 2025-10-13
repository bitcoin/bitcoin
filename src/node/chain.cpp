// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chain.h>

#include <interfaces/types.h>

#include <chain.h>
#include <kernel/cs_main.h>
#include <sync.h>
#include <uint256.h>
#include <validation.h>

class CBlock;

using interfaces::BlockInfo;

namespace node {
BlockInfo MakeBlockInfo(const CBlockIndex* index, const CBlock* data, const Chainstate* chainstate)
{
    BlockInfo info{index ? *index->phashBlock : uint256::ZERO};
    if (index) {
        info.prev_hash = index->pprev ? index->pprev->phashBlock : nullptr;
        info.height = index->nHeight;
        info.chain_time_max = index->GetBlockTimeMax();
        LOCK(::cs_main);
        info.file_number = index->nFile;
        info.data_pos = index->nDataPos;
        if (chainstate) {
            info.background_sync = true;
            const CBlockIndex* last_flushed{chainstate->LastFlushedBlock()};
            if (index == last_flushed) {
                info.status = BlockInfo::FLUSHED_TIP;
            } else if (!last_flushed || last_flushed->GetAncestor(index->nHeight) == index) {
                info.status = BlockInfo::FLUSHED;
            }
        }
    }
    info.data = data;
    return info;
}
} // namespace node
