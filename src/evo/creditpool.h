// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CREDITPOOL_H
#define BITCOIN_EVO_CREDITPOOL_H

#include <coins.h>

#include <evo/assetlocktx.h>
#include <evo/evodb.h>

#include <saltedhasher.h>
#include <serialize.h>
#include <sync.h>
#include <threadsafety.h>
#include <unordered_lru_cache.h>
#include <util/ranges_set.h>

#include <optional>
#include <unordered_set>

class CBlockIndex;
class BlockValidationState;
class TxValidationState;

namespace Consensus
{
    struct Params;
}

struct CCreditPool {
    CAmount locked{0};

    // needs for logic of limits of unlocks
    CAmount currentLimit{0};
    CAmount latelyUnlocked{0};
    CRangesSet indexes{};

    std::string ToString() const;

    SERIALIZE_METHODS(CCreditPool, obj)
    {
        READWRITE(
            obj.locked,
            obj.currentLimit,
            obj.latelyUnlocked,
            obj.indexes
        );
    }
};

/**
 * The class CCreditPoolDiff has 2 purposes:
 *  - it helps to determine which transaction can be included in new mined block
 *  within current limits for Asset Unlock transactions and filter duplicated indexes
 *  - to validate Asset Unlock transaction in mined block. The standalone checks of tx
 *  such as CheckSpecialTx is not able to do so because at that moment there is no full
 *  information about Credit Pool limits.
 *
 * CCreditPoolDiff temporary stores new values `lockedAmount` and `indexes` while
 * limits should stay same and depends only on the previous block.
 */
class CCreditPoolDiff {
private:
    const CCreditPool pool;
    std::unordered_set<uint64_t> newIndexes;

    CAmount sessionLocked{0};
    CAmount sessionUnlocked{0};
    CAmount platformReward{0};

    const CBlockIndex *pindexPrev{nullptr};
    const Consensus::Params& params;
public:
    explicit CCreditPoolDiff(CCreditPool starter, const CBlockIndex *pindexPrev,
                             const Consensus::Params& consensusParams,
                             const CAmount blockSubsidy);

    /**
     * This function should be called for each Asset Lock/Unlock tx
     * to change amount of credit pool
     * @return true if transaction can be included in this block
     */
    bool ProcessLockUnlockTransaction(const CTransaction& tx, TxValidationState& state);

    /**
     * this function returns total amount of credits for the next block
     */
    CAmount GetTotalLocked() const {
        return pool.locked + sessionLocked - sessionUnlocked + platformReward;
    }

    std::string ToString() const {
        return strprintf("CCreditPoolDiff(sessionLocked=%lld, sessionUnlocked=%lld, platforomReward=%lld, newIndexes=%lld, pool=%s)", sessionLocked, sessionUnlocked, platformReward, newIndexes.size(), pool.ToString());
    }

private:
    bool Lock(const CTransaction& tx, TxValidationState& state);
    bool Unlock(const CTransaction& tx, TxValidationState& state);
};

class CCreditPoolManager
{
private:
    static constexpr size_t CreditPoolCacheSize = 1000;
    RecursiveMutex cache_mutex;
    unordered_lru_cache<uint256, CCreditPool, StaticSaltedHasher> creditPoolCache GUARDED_BY(cache_mutex) {CreditPoolCacheSize};

    CEvoDB& evoDb;

    static constexpr int DISK_SNAPSHOT_PERIOD = 576; // once per day

public:
    static constexpr int LimitBlocksToTrace = 576;
    static constexpr CAmount LimitAmountLow = 100 * COIN;
    static constexpr CAmount LimitAmountHigh = 1000 * COIN;

    explicit CCreditPoolManager(CEvoDB& _evoDb);

    ~CCreditPoolManager() = default;

    /**
      * @return CCreditPool with data or with empty depends on activation V19 at that block
      * In case if block is invalid the function GetCreditPool throws an exception
      * it can happen if there limits of withdrawal (unlock) exceed
      */
    CCreditPool GetCreditPool(const CBlockIndex* block, const Consensus::Params& consensusParams);

private:
    std::optional<CCreditPool> GetFromCache(const CBlockIndex& block_index);
    void AddToCache(const uint256& block_hash, int height, const CCreditPool& pool);

    CCreditPool ConstructCreditPool(const CBlockIndex* block_index, CCreditPool prev, const Consensus::Params& consensusParams);
};

std::optional<CCreditPoolDiff> GetCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams,
                                                         const CAmount blockSubsidy, BlockValidationState& state);

extern std::unique_ptr<CCreditPoolManager> creditPoolManager;

#endif
