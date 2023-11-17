// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/creditpool.h>

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/specialtx.h>

#include <chain.h>
#include <consensus/validation.h>
#include <llmq/utils.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <validation.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stack>

// Forward declaration to prevent a new circular dependencies through masternode/payments.h
namespace MasternodePayments {
CAmount PlatformShare(const CAmount masternodeReward);
} // namespace MasternodePayments

static const std::string DB_CREDITPOOL_SNAPSHOT = "cpm_S";

std::unique_ptr<CCreditPoolManager> creditPoolManager;

static bool GetDataFromUnlockTx(const CTransaction& tx, CAmount& toUnlock, uint64_t& index, TxValidationState& state)
{
    CAssetUnlockPayload assetUnlockTx;
    if (!GetTxPayload(tx, assetUnlockTx)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-payload");
    }

    index = assetUnlockTx.getIndex();
    toUnlock = assetUnlockTx.getFee();
    for (const CTxOut& txout : tx.vout) {
        if (!MoneyRange(txout.nValue)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-txout-outofrange");
        }
        toUnlock += txout.nValue;
    }
    return true;
}

namespace {
    struct UnlockDataPerBlock  {
        CAmount unlocked{0};
        std::unordered_set<uint64_t> indexes;
    };
} // anonymous namespace

// it throws exception if anything went wrong
static UnlockDataPerBlock GetDataFromUnlockTxes(const std::vector<CTransactionRef>& vtx)
{
    UnlockDataPerBlock blockData;

    for (CTransactionRef tx : vtx) {
        if (tx->nVersion != 3 || tx->nType != TRANSACTION_ASSET_UNLOCK) continue;

        CAmount unlocked{0};
        TxValidationState tx_state;
        uint64_t index{0};
        if (!GetDataFromUnlockTx(*tx, unlocked, index, tx_state)) {
            throw std::runtime_error(strprintf("%s: CCreditPoolManager::GetDataFromUnlockTxes failed: %s", __func__, tx_state.ToString()));
        }
        blockData.unlocked += unlocked;
        blockData.indexes.insert(index);
    }
    return blockData;
}

std::string CCreditPool::ToString() const
{
    return strprintf("CCreditPool(locked=%lld, currentLimit=%lld)",
            locked, currentLimit);
}

std::optional<CCreditPool> CCreditPoolManager::GetFromCache(const CBlockIndex* const block_index)
{
    if (!llmq::utils::IsV20Active(block_index)) return CCreditPool{};

    const uint256 block_hash = block_index->GetBlockHash();
    CCreditPool pool;
    {
        LOCK(cache_mutex);
        if (creditPoolCache.get(block_hash, pool)) {
            return pool;
        }
    }
    if (block_index->nHeight % DISK_SNAPSHOT_PERIOD == 0) {
        if (evoDb.Read(std::make_pair(DB_CREDITPOOL_SNAPSHOT, block_hash), pool)) {
            LOCK(cache_mutex);
            creditPoolCache.insert(block_hash, pool);
            return pool;
        }
    }
    return std::nullopt;
}

void CCreditPoolManager::AddToCache(const uint256& block_hash, int height, const CCreditPool &pool)
{
    {
        LOCK(cache_mutex);
        creditPoolCache.insert(block_hash, pool);
    }
    if (height % DISK_SNAPSHOT_PERIOD == 0) {
        evoDb.Write(std::make_pair(DB_CREDITPOOL_SNAPSHOT, block_hash), pool);
    }
}

static std::optional<CBlock> GetBlockForCreditPool(const CBlockIndex* const block_index, const Consensus::Params& consensusParams)
{
    CBlock block;
    if (!ReadBlockFromDisk(block, block_index, consensusParams)) {
        throw std::runtime_error("failed-getcbforblock-read");
    }
    // Should not fail if V20 (DIP0027) is active but it happens for RegChain (unit tests)
    if (block.vtx[0]->nVersion != 3) return std::nullopt;

    assert(!block.vtx.empty());
    assert(block.vtx[0]->nVersion == 3);
    assert(!block.vtx[0]->vExtraPayload.empty());

    return block;
}

CCreditPool CCreditPoolManager::ConstructCreditPool(const CBlockIndex* const block_index, CCreditPool prev, const Consensus::Params& consensusParams)
{
    std::optional<CBlock> block = GetBlockForCreditPool(block_index, consensusParams);
    if (!block) {
        // If reading of previous block is not successfully, but
        // prev contains credit pool related data, something strange happened
        assert(prev.locked == 0);
        assert(prev.indexes.IsEmpty());

        CCreditPool emptyPool;
        AddToCache(block_index->GetBlockHash(), block_index->nHeight, emptyPool);
        return emptyPool;
    }
    CAmount locked{0};
    {
        CCbTx cbTx;
        if (!GetTxPayload(block->vtx[0]->vExtraPayload, cbTx)) {
            throw std::runtime_error(strprintf("%s: failed-getcreditpool-cbtx-payload", __func__));
        }
        locked = cbTx.creditPoolBalance;
    }

    // We use here sliding window with LimitBlocksToTrace to determine
    // current limits for asset unlock transactions.
    // Indexes should not be duplicated since genesis block, but the Unlock Amount
    // of withdrawal transaction is limited only by this window
    UnlockDataPerBlock blockData = GetDataFromUnlockTxes(block->vtx);
    CRangesSet indexes{std::move(prev.indexes)};
    if (std::any_of(blockData.indexes.begin(), blockData.indexes.end(), [&](const uint64_t index) { return !indexes.Add(index); })) {
        throw std::runtime_error(strprintf("%s: failed-getcreditpool-index-duplicated", __func__));
    }

    const CBlockIndex* distant_block_index = block_index;
    for (size_t i = 0; i < CCreditPoolManager::LimitBlocksToTrace; ++i) {
        distant_block_index = distant_block_index->pprev;
        if (distant_block_index == nullptr) break;
    }
    CAmount distantUnlocked{0};
    if (distant_block_index) {
        if (std::optional<CBlock> distant_block = GetBlockForCreditPool(distant_block_index, consensusParams); distant_block) {
            distantUnlocked = GetDataFromUnlockTxes(distant_block->vtx).unlocked;
        }
    }

    // Unlock limits are # max(100, min(.10 * assetlockpool, 1000)) inside window
    CAmount currentLimit = locked;
    const CAmount latelyUnlocked = prev.latelyUnlocked + blockData.unlocked - distantUnlocked;
    if (currentLimit + latelyUnlocked > LimitAmountLow) {
        currentLimit = std::max(LimitAmountLow, locked / 10) - latelyUnlocked;
        if (currentLimit < 0) currentLimit = 0;
    }
    currentLimit = std::min(currentLimit, LimitAmountHigh - latelyUnlocked);

    assert(currentLimit >= 0);

    if (currentLimit > 0 || latelyUnlocked > 0 || locked > 0) {
        LogPrint(BCLog::CREDITPOOL, "CCreditPoolManager: asset unlock limits on height: %d locked: %d.%08d limit: %d.%08d previous: %d.%08d\n",
               block_index->nHeight, locked / COIN, locked % COIN,
               currentLimit / COIN, currentLimit % COIN,
               latelyUnlocked / COIN, latelyUnlocked % COIN);
    }

    CCreditPool pool{locked, currentLimit, latelyUnlocked, indexes};
    AddToCache(block_index->GetBlockHash(), block_index->nHeight, pool);
    return pool;

}

CCreditPool CCreditPoolManager::GetCreditPool(const CBlockIndex* block_index, const Consensus::Params& consensusParams)
{
    std::stack<const CBlockIndex *> to_calculate;

    std::optional<CCreditPool> poolTmp;
    while (!(poolTmp = GetFromCache(block_index)).has_value()) {
        to_calculate.push(block_index);
        block_index = block_index->pprev;
    }
    while (!to_calculate.empty()) {
        poolTmp = ConstructCreditPool(to_calculate.top(), *poolTmp, consensusParams);
        to_calculate.pop();
    }
    return *poolTmp;
}

CCreditPoolManager::CCreditPoolManager(CEvoDB& _evoDb)
: evoDb(_evoDb)
{
}

CCreditPoolDiff::CCreditPoolDiff(CCreditPool starter, const CBlockIndex *pindex, const Consensus::Params& consensusParams, const CAmount blockSubsidy) :
    pool(std::move(starter)),
    pindex(pindex),
    params(consensusParams)
{
    assert(pindex);

    if (llmq::utils::IsMNRewardReallocationActive(pindex)) {
        // We consider V20 active if mn_rr is active
        platformReward = MasternodePayments::PlatformShare(GetMasternodePayment(pindex->nHeight, blockSubsidy, /*fV20Active=*/ true));
    }
}

bool CCreditPoolDiff::Lock(const CTransaction& tx, TxValidationState& state)
{
    if (CAssetLockPayload assetLockTx; !GetTxPayload(tx, assetLockTx)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-lock-payload");
    }

    for (const CTxOut& txout : tx.vout) {
        if (const CScript& script = txout.scriptPubKey; script.empty() || script[0] != OP_RETURN) continue;

        sessionLocked += txout.nValue;
        return true;
    }

    return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-lock-invalid");
}

bool CCreditPoolDiff::Unlock(const CTransaction& tx, TxValidationState& state)
{
    uint64_t index{0};
    CAmount toUnlock{0};
    if (!GetDataFromUnlockTx(tx, toUnlock, index, state)) {
        // state is set up inside GetDataFromUnlockTx
        return false;
    }

    if (sessionUnlocked + toUnlock > pool.currentLimit) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-too-much");
    }

    if (pool.indexes.Contains(index) || newIndexes.count(index)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-duplicated-index");
    }

    newIndexes.insert(index);
    sessionUnlocked += toUnlock;
    return true;
}

bool CCreditPoolDiff::ProcessLockUnlockTransaction(const CTransaction& tx, TxValidationState& state)
{
    if (tx.nVersion != 3) return true;
    if (tx.nType != TRANSACTION_ASSET_LOCK && tx.nType != TRANSACTION_ASSET_UNLOCK) return true;

    if (!CheckAssetLockUnlockTx(tx, pindex, pool.indexes, state)) {
        // pass the state returned by the function above
        return false;
    }

    try {
        switch (tx.nType) {
        case TRANSACTION_ASSET_LOCK:
            return Lock(tx, state);
        case TRANSACTION_ASSET_UNLOCK:
            return Unlock(tx, state);
        default:
            return true;
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-procassetlocksinblock");
    }
}

std::optional<CCreditPoolDiff> GetCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams,
                                                         const CAmount blockSubsidy, BlockValidationState& state)
{
    try {
        const CCreditPool creditPool = creditPoolManager->GetCreditPool(pindexPrev, consensusParams);
        LogPrint(BCLog::CREDITPOOL, "%s: CCreditPool is %s\n", __func__, creditPool.ToString());
        CCreditPoolDiff creditPoolDiff(creditPool, pindexPrev, consensusParams, blockSubsidy);
        for (size_t i = 1; i < block.vtx.size(); ++i) {
            const auto& tx = *block.vtx[i];
            TxValidationState tx_state;
            if (!creditPoolDiff.ProcessLockUnlockTransaction(tx, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS);
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Process Lock/Unlock Transaction failed at Credit Pool (tx hash %s) %s", tx.GetHash().ToString(), tx_state.GetDebugMessage()));
                return std::nullopt;
            }
        }
        return creditPoolDiff;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-getcreditpooldiff");
        return std::nullopt;
    }
}
