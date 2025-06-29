// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/creditpool.h>

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/evodb.h>
#include <evo/specialtx.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <validation.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stack>

using node::BlockManager;
using node::ReadBlockFromDisk;

// Forward declaration to prevent a new circular dependencies through masternode/payments.h
CAmount PlatformShare(const CAmount masternodeReward);

static const std::string DB_CREDITPOOL_SNAPSHOT = "cpm_S";

static bool GetDataFromUnlockTx(const CTransaction& tx, CAmount& toUnlock, uint64_t& index, TxValidationState& state)
{
    const auto opt_assetUnlockTx = GetTxPayload<CAssetUnlockPayload>(tx);
    if (!opt_assetUnlockTx.has_value()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-payload");
    }

    index = opt_assetUnlockTx->getIndex();
    toUnlock = opt_assetUnlockTx->getFee();
    for (const CTxOut& txout : tx.vout) {
        if (!MoneyRange(txout.nValue)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-txout-outofrange");
        }
        toUnlock += txout.nValue;
    }
    return true;
}

namespace {
struct CreditPoolDataPerBlock {
    CAmount credit_pool{0};
    CAmount unlocked{0};
    std::unordered_set<uint64_t> indexes;
};
} // anonymous namespace

// it throws exception if anything went wrong
static std::optional<CreditPoolDataPerBlock> GetCreditDataFromBlock(const gsl::not_null<const CBlockIndex*> block_index,
                                                                    const Consensus::Params& consensusParams)
{
    // There's no CbTx before DIP0003 activation
    if (!DeploymentActiveAt(*block_index, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0003)) {
        return std::nullopt;
    }

    CreditPoolDataPerBlock blockData;

    static Mutex cache_mutex;
    static unordered_lru_cache<uint256, CreditPoolDataPerBlock, StaticSaltedHasher> block_data_cache GUARDED_BY(
        cache_mutex){static_cast<size_t>(Params().CreditPoolPeriodBlocks()) * 2};
    if (LOCK(cache_mutex); block_data_cache.get(block_index->GetBlockHash(), blockData)) {
        return blockData;
    }

    CBlock block;
    if (!ReadBlockFromDisk(block, block_index, consensusParams)) {
        throw std::runtime_error("failed-getcbforblock-read");
    }

    if (block.vtx.empty() || block.vtx[0]->vExtraPayload.empty() || !block.vtx[0]->IsSpecialTxVersion()) {
        LogPrintf("%s: ERROR: empty CbTx for CreditPool at height=%d\n", __func__, block_index->nHeight);
        return std::nullopt;
    }


    if (const auto opt_cbTx = GetTxPayload<CCbTx>(block.vtx[0]->vExtraPayload); opt_cbTx) {
        blockData.credit_pool = opt_cbTx->creditPoolBalance;
    } else {
        LogPrintf("%s: WARNING: No valid CbTx at height=%d\n", __func__, block_index->nHeight);
        return std::nullopt;
    }
    for (CTransactionRef tx : block.vtx) {
        if (!tx->IsSpecialTxVersion() || tx->nType != TRANSACTION_ASSET_UNLOCK) continue;

        CAmount unlocked{0};
        TxValidationState tx_state;
        uint64_t index{0};
        if (!GetDataFromUnlockTx(*tx, unlocked, index, tx_state)) {
            throw std::runtime_error(strprintf("%s: GetDataFromUnlockTx failed: %s", __func__, tx_state.ToString()));
        }
        blockData.unlocked += unlocked;
        blockData.indexes.insert(index);
    }

    LOCK(cache_mutex);
    block_data_cache.insert(block_index->GetBlockHash(), blockData);
    return blockData;
}

std::string CCreditPool::ToString() const
{
    return strprintf("CCreditPool(locked=%lld, currentLimit=%lld)",
            locked, currentLimit);
}

std::optional<CCreditPool> CCreditPoolManager::GetFromCache(const CBlockIndex& block_index)
{
    if (!DeploymentActiveAt(block_index, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)) return CCreditPool{};

    const uint256 block_hash = block_index.GetBlockHash();
    CCreditPool pool;
    {
        LOCK(cache_mutex);
        if (creditPoolCache.get(block_hash, pool)) {
            return pool;
        }
    }
    if (block_index.nHeight % DISK_SNAPSHOT_PERIOD == 0) {
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

CCreditPool CCreditPoolManager::ConstructCreditPool(const gsl::not_null<const CBlockIndex*> block_index,
                                                    CCreditPool prev, const Consensus::Params& consensusParams)
{
    std::optional<CreditPoolDataPerBlock> opt_block_data = GetCreditDataFromBlock(block_index, consensusParams);
    if (!opt_block_data) {
        // If reading of previous block is not successfully, but
        // prev contains credit pool related data, something strange happened
        if (prev.locked != 0) {
            throw std::runtime_error(strprintf("Failed to create CreditPool but previous block has value"));
        }
        if (!prev.indexes.IsEmpty()) {
            throw std::runtime_error(
                strprintf("Failed to create CreditPool but asset unlock transactions already mined"));
        }
        CCreditPool emptyPool;
        AddToCache(block_index->GetBlockHash(), block_index->nHeight, emptyPool);
        return emptyPool;
    }
    const CreditPoolDataPerBlock& blockData{*opt_block_data};

    // We use here sliding window with Params().CreditPoolPeriodBlocks to determine
    // current limits for asset unlock transactions.
    // Indexes should not be duplicated since genesis block, but the Unlock Amount
    // of withdrawal transaction is limited only by this window
    CRangesSet indexes{std::move(prev.indexes)};
    if (std::any_of(blockData.indexes.begin(), blockData.indexes.end(), [&](const uint64_t index) { return !indexes.Add(index); })) {
        throw std::runtime_error(strprintf("%s: failed-getcreditpool-index-duplicated", __func__));
    }

    const CBlockIndex* distant_block_index{
        block_index->GetAncestor(block_index->nHeight - Params().CreditPoolPeriodBlocks())};
    CAmount distantUnlocked{0};
    if (distant_block_index) {
        if (std::optional<CreditPoolDataPerBlock> distant_block{GetCreditDataFromBlock(distant_block_index, consensusParams)};
            distant_block) {
            distantUnlocked = distant_block->unlocked;
        }
    }

    CAmount currentLimit = blockData.credit_pool;
    const CAmount latelyUnlocked = prev.latelyUnlocked + blockData.unlocked - distantUnlocked;
    if (DeploymentActiveAt(*block_index, Params().GetConsensus(), Consensus::DEPLOYMENT_WITHDRAWALS)) {
        currentLimit = std::min(currentLimit, LimitAmountV22);
    } else {
        // Unlock limits in pre-v22 are max(100, min(.10 * assetlockpool, 1000)) inside window
        if (currentLimit + latelyUnlocked > LimitAmountLow) {
            currentLimit = std::max(LimitAmountLow, blockData.credit_pool / 10) - latelyUnlocked;
            if (currentLimit < 0) currentLimit = 0;
        }
        currentLimit = std::min(currentLimit, LimitAmountHigh - latelyUnlocked);
    }

    if (currentLimit != 0 || latelyUnlocked > 0 || blockData.credit_pool > 0) {
        LogPrint(BCLog::CREDITPOOL, /* Continued */
                 "CCreditPoolManager: asset unlock limits on height: %d locked: %d.%08d limit: %d.%08d "
                 "unlocked-in-window: %d.%08d\n",
                 block_index->nHeight, blockData.credit_pool / COIN, blockData.credit_pool % COIN, currentLimit / COIN,
                 currentLimit % COIN, latelyUnlocked / COIN, latelyUnlocked % COIN);
    }

    if (currentLimit < 0) {
        throw std::runtime_error(
            strprintf("Negative limit for CreditPool: %d.%08d\n", currentLimit / COIN, currentLimit % COIN));
    }

    CCreditPool pool{blockData.credit_pool, currentLimit, latelyUnlocked, indexes};
    AddToCache(block_index->GetBlockHash(), block_index->nHeight, pool);
    return pool;

}

CCreditPool CCreditPoolManager::GetCreditPool(const CBlockIndex* block_index, const Consensus::Params& consensusParams)
{
    std::stack<gsl::not_null<const CBlockIndex*>> to_calculate;

    std::optional<CCreditPool> poolTmp;
    while (block_index != nullptr && !(poolTmp = GetFromCache(*block_index)).has_value()) {
        to_calculate.push(block_index);
        block_index = block_index->pprev;
    }
    if (block_index == nullptr) poolTmp = CCreditPool{};
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

CCreditPoolDiff::CCreditPoolDiff(CCreditPool starter, const CBlockIndex* pindexPrev,
                                 const Consensus::Params& consensusParams, const CAmount blockSubsidy) :
    pool(std::move(starter)),
    pindexPrev(pindexPrev)
{
    assert(pindexPrev);

    if (DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_MN_RR)) {
        // If credit pool exists, it means v20 is activated
        platformReward = PlatformShare(GetMasternodePayment(pindexPrev->nHeight + 1, blockSubsidy, /*fV20Active=*/ true));
    }
}

bool CCreditPoolDiff::Lock(const CTransaction& tx, TxValidationState& state)
{
    if (const auto opt_assetLockTx = GetTxPayload<CAssetLockPayload>(tx); !opt_assetLockTx) {
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

bool CCreditPoolDiff::ProcessLockUnlockTransaction(const BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, TxValidationState& state)
{
    if (!tx.IsSpecialTxVersion()) return true;
    if (tx.nType != TRANSACTION_ASSET_LOCK && tx.nType != TRANSACTION_ASSET_UNLOCK) return true;

    if (!CheckAssetLockUnlockTx(blockman, qman, tx, pindexPrev, pool.indexes, state)) {
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

std::optional<CCreditPoolDiff> GetCreditPoolDiffForBlock(CCreditPoolManager& cpoolman, const BlockManager& blockman, const llmq::CQuorumManager& qman,
                                                         const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams,
                                                         const CAmount blockSubsidy, BlockValidationState& state)
{
    try {
        const CCreditPool creditPool = cpoolman.GetCreditPool(pindexPrev, consensusParams);
        LogPrint(BCLog::CREDITPOOL, "%s: CCreditPool is %s\n", __func__, creditPool.ToString());
        CCreditPoolDiff creditPoolDiff(creditPool, pindexPrev, consensusParams, blockSubsidy);
        for (size_t i = 1; i < block.vtx.size(); ++i) {
            const auto& tx = *block.vtx[i];
            TxValidationState tx_state;
            if (!creditPoolDiff.ProcessLockUnlockTransaction(blockman, qman, tx, tx_state)) {
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
