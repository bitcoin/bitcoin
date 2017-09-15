// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"

#include "chain.h"
#include "chainparams.h"
#include "reverse_iterator.h"
#include "validation.h"

namespace Checkpoints {

CBlockIndex* GetLastCheckpoint(const CCheckpointData& data)
{
    for (const auto& checkpoint : reverse_iterate(data.mapCheckpoints)) {
        const auto& hash = checkpoint.second;
        const auto match = mapBlockIndex.find(hash);
        if (match != mapBlockIndex.end()) {
            return match->second;
        }
    }
    return nullptr;
}

} // namespace Checkpoints
