// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>

#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>
#include <util/system.h>
#include <validation.h>

#include <memory>
#include <utility>

namespace interfaces {
namespace {

class LockImpl : public Chain::Lock
{
    Optional<int> getHeight() override
    {
        int height = ::chainActive.Height();
        if (height >= 0) {
            return height;
        }
        return nullopt;
    }
    Optional<int> getBlockHeight(const uint256& hash) override
    {
        CBlockIndex* block = LookupBlockIndex(hash);
        if (block && ::chainActive.Contains(block)) {
            return block->nHeight;
        }
        return nullopt;
    }
    int getBlockDepth(const uint256& hash) override
    {
        const Optional<int> tip_height = getHeight();
        const Optional<int> height = getBlockHeight(hash);
        return tip_height && height ? *tip_height - *height + 1 : 0;
    }
    uint256 getBlockHash(int height) override
    {
        CBlockIndex* block = ::chainActive[height];
        assert(block != nullptr);
        return block->GetBlockHash();
    }
    int64_t getBlockTime(int height) override
    {
        CBlockIndex* block = ::chainActive[height];
        assert(block != nullptr);
        return block->GetBlockTime();
    }
    int64_t getBlockMedianTimePast(int height) override
    {
        CBlockIndex* block = ::chainActive[height];
        assert(block != nullptr);
        return block->GetMedianTimePast();
    }
    Optional<int> findFork(const uint256& hash, Optional<int>* height) override
    {
        const CBlockIndex* block = LookupBlockIndex(hash);
        const CBlockIndex* fork = block ? ::chainActive.FindFork(block) : nullptr;
        if (height) {
            if (block) {
                *height = block->nHeight;
            } else {
                height->reset();
            }
        }
        if (fork) {
            return fork->nHeight;
        }
        return nullopt;
    }
};

class LockingStateImpl : public LockImpl, public UniqueLock<CCriticalSection>
{
    using UniqueLock::UniqueLock;
};

class ChainImpl : public Chain
{
public:
    std::unique_ptr<Chain::Lock> lock(bool try_lock) override
    {
        auto result = MakeUnique<LockingStateImpl>(::cs_main, "cs_main", __FILE__, __LINE__, try_lock);
        if (try_lock && result && !*result) return {};
        // std::move necessary on some compilers due to conversion from
        // LockingStateImpl to Lock pointer
        return std::move(result);
    }
    std::unique_ptr<Chain::Lock> assumeLocked() override { return MakeUnique<LockImpl>(); }
    bool findBlock(const uint256& hash, CBlock* block, int64_t* time, int64_t* time_max) override
    {
        CBlockIndex* index;
        {
            LOCK(cs_main);
            index = LookupBlockIndex(hash);
            if (!index) {
                return false;
            }
            if (time) {
                *time = index->GetBlockTime();
            }
            if (time_max) {
                *time_max = index->GetBlockTimeMax();
            }
        }
        if (block && !ReadBlockFromDisk(*block, index, Params().GetConsensus())) {
            block->SetNull();
        }
        return true;
    }
};

} // namespace

std::unique_ptr<Chain> MakeChain() { return MakeUnique<ChainImpl>(); }

} // namespace interfaces
