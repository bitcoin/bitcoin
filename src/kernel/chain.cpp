// Copyright (c) 2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <sync.h>
#include <uint256.h>

class CBlock;

namespace kernel {
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* index, const CBlock* data)
{
    interfaces::BlockInfo info{index ? *index->phashBlock : uint256::ZERO};
    if (index) {
        info.prev_hash = index->pprev ? index->pprev->phashBlock : nullptr;
        info.height = index->nHeight;
        info.chain_time_max = index->GetBlockTimeMax();
        LOCK(::cs_main);
        info.file_number = index->nFile;
        info.data_pos = index->nDataPos;
    }
    info.data = data;
    return info;
}
} // namespace kernel

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role) {
    switch(role) {
        case ChainstateRole::NORMAL: os << "normal"; break;
        case ChainstateRole::ASSUMEDVALID: os << "assumedvalid"; break;
        case ChainstateRole::BACKGROUND: os << "background"; break;
        default: os.setstate(std::ios_base::failbit);
    }
    return os;
}
