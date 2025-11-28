// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <kernel/types.h>
#include <sync.h>
#include <uint256.h>

class CBlock;

using kernel::ChainstateRole;

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

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role) {
    if (!role.validated) {
        os << "assumedvalid";
    } else if (role.historical) {
        os << "background";
    } else {
        os << "normal";
    }
    return os;
}
} // namespace kernel
