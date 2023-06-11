// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <sync.h>
#include <uint256.h>

class CBlock;

namespace kernel {
    interfaces::BlockInfo MakeBlockInfoRecursive(const CBlockIndex* index, const CBlock* data)
    {
        interfaces::BlockInfo info;

        if (index) {
            const auto& indexBlock = *index;
            const auto* pprevBlock = indexBlock.pprev;

            info.hash = indexBlock.phashBlock ? *indexBlock.phashBlock : uint256::ZERO;
            info.prev_hash = GetPreviousHash(pprevBlock);
            info.height = indexBlock.nHeight;
            info.chain_time_max = indexBlock.GetBlockTimeMax();
            
            {
                LOCK(::cs_main);
                info.file_number = indexBlock.nFile;
                info.data_pos = indexBlock.nDataPos;
            }
        }
        
        info.data = data;
        return info;
    }

    const uint256* GetPreviousHash(const CBlockIndex* index)
    {
        if (index) {
            const auto* pprevBlock = index->pprev;
            return pprevBlock ? (pprevBlock->phashBlock ? pprevBlock->phashBlock : GetPreviousHash(pprevBlock)) : nullptr;
        }
        return nullptr;
    }
}




