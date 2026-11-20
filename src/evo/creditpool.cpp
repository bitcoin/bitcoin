// Copyright (c) 2023-2025 The Dash Core developers
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
#include <masternode/payments.h>
#include <node/blockstorage.h>
#include <shutdown.h>
#include <util/overflow.h>
#include <validation.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stack>
#include <stdexcept>

using node::ReadBlockFromDisk;

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
        const auto total = CheckedAdd(toUnlock, txout.nValue);
        if (!total) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-overflow");
        }
        toUnlock = *total;
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
    if (!DeploymentActiveAt(*block_index, consensusParams, Consensus::DEPLOYMENT_DIP0003)) {
        return std::nullopt;
    }

    CreditPoolDataPerBlock blockData;

    static Mutex cache_mutex;
    static Uint256LruHashMap<CreditPoolDataPerBlock> block_data_cache GUARDED_BY(cache_mutex){
        static_cast<size_t>(Params().CreditPoolPeriodBlocks()) * 2};
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
    for (const CTransactionRef& tx : block.vtx) {
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
    if (!DeploymentActiveAt(block_index, m_chainman.GetConsensus(), Consensus::DEPLOYMENT_V20)) return CCreditPool{};

    const uint256 block_hash = block_index.GetBlockHash();
    CCreditPool pool;
    if (WITH_LOCK(cache_mutex, return creditPoolCache.get(block_hash, pool))) {
        // The pool may have been constructed, and cached, outside a block-scoped transaction
        // (mempool acceptance, template creation, RPC), where its snapshot cannot be written.
        // Persist it from the first transaction-scoped lookup instead, or the snapshot would be
        // lost for good: block connection only sees the cache hit and never constructs it again.
        MaybeWriteSnapshot(block_hash, block_index.nHeight, pool);
        return pool;
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

void CCreditPoolManager::MaybeWriteSnapshot(const uint256& block_hash, int height, const CCreditPool& pool)
{
    // The disk snapshot is an optimization; skip it outside a block-scoped EvoDB transaction
    // (e.g. a pool constructed on a cold cache during mempool acceptance or template creation),
    // where the write would never be committed and would trip the clean-transaction assertion
    // at the next root commit. GetFromCache() writes it once a transaction-scoped lookup hits
    // the cached pool.
    if (height % DISK_SNAPSHOT_PERIOD != 0 || !evoDb.HasActiveTransaction()) return;
    if (!evoDb.WriteDerived(std::make_pair(DB_CREDITPOOL_SNAPSHOT, block_hash), pool)) {
        // A mismatch is local EvoDB corruption, not a statement about the
        // block. Abort here: some callers (miner, RPC) never pass through a
        // validation-state catch, and the block-connect catches must not
        // translate this into a consensus rejection.
        const std::string msg = strprintf("CCreditPoolManager::%s -- EvoDB credit pool mismatch for block %s", __func__,
                                          block_hash.ToString());
        AbortNode(msg);
        throw EvoDbInconsistencyError(msg);
    }
}

void CCreditPoolManager::AddToCache(const uint256& block_hash, int height, const CCreditPool& pool)
{
    MaybeWriteSnapshot(block_hash, height, pool);
    LOCK(cache_mutex);
    creditPoolCache.insert(block_hash, pool);
}

CCreditPool CCreditPoolManager::ConstructCreditPool(const gsl::not_null<const CBlockIndex*> block_index, CCreditPool prev)
{
    std::optional<CreditPoolDataPerBlock> opt_block_data = GetCreditDataFromBlock(block_index, m_chainman.GetConsensus());
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
        block_index->GetAncestor(block_index->nHeight - m_chainman.GetParams().CreditPoolPeriodBlocks())};
    CAmount distantUnlocked{0};
    CAmount distantBalance{0};
    if (distant_block_index) {
        if (std::optional<CreditPoolDataPerBlock> distant_block{
                GetCreditDataFromBlock(distant_block_index, m_chainman.GetConsensus())};
            distant_block) {
            distantUnlocked = distant_block->unlocked;
            distantBalance = distant_block->credit_pool;
        }
    }

    CAmount currentLimit = blockData.credit_pool;
    const CAmount latelyUnlocked = prev.latelyUnlocked + blockData.unlocked - distantUnlocked;
    if (DeploymentActiveAt(*block_index, m_chainman, Consensus::DEPLOYMENT_V24)) {
        currentLimit = UnlockLimitV24(blockData.credit_pool, distantBalance);
    } else if (DeploymentActiveAt(*block_index, m_chainman.GetConsensus(), Consensus::DEPLOYMENT_WITHDRAWALS)) {
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
                 "unlocked-in-window: %d.%08d locked-at-window-start: %d.%08d\n",
                 block_index->nHeight, blockData.credit_pool / COIN, blockData.credit_pool % COIN, currentLimit / COIN,
                 currentLimit % COIN, latelyUnlocked / COIN, latelyUnlocked % COIN, distantBalance / COIN,
                 distantBalance % COIN);
    }

    if (currentLimit < 0) {
        throw std::runtime_error(
            strprintf("Negative limit for CreditPool: %d.%08d\n", currentLimit / COIN, currentLimit % COIN));
    }

    CCreditPool pool{blockData.credit_pool, currentLimit, latelyUnlocked, indexes};
    AddToCache(block_index->GetBlockHash(), block_index->nHeight, pool);
    return pool;

}

CAmount CCreditPoolManager::GetBalanceAt(const gsl::not_null<const CBlockIndex*> block_index,
                                         const Consensus::Params& consensusParams)
{
    const std::optional<CreditPoolDataPerBlock> block_data{GetCreditDataFromBlock(block_index, consensusParams)};
    return block_data ? block_data->credit_pool : CAmount{0};
}

CCreditPool CCreditPoolManager::GetCreditPool(const CBlockIndex* block_index)
{
    std::stack<gsl::not_null<const CBlockIndex*>> to_calculate;

    std::optional<CCreditPool> poolTmp;
    while (block_index != nullptr && !(poolTmp = GetFromCache(*block_index)).has_value()) {
        // Not emplace: gsl::not_null captures the caller's source_location as a
        // default argument, so constructing it inside the container would
        // report an Expects() failure against a standard library header
        // instead of this loop, and embed that header's path in the binary.
        to_calculate.push(block_index); // NOLINT(modernize-use-emplace)
        block_index = block_index->pprev;
    }
    if (block_index == nullptr) poolTmp = CCreditPool{};
    while (!to_calculate.empty()) {
        poolTmp = ConstructCreditPool(to_calculate.top(), *poolTmp);
        to_calculate.pop();
    }
    return *poolTmp;
}

CCreditPoolManager::CCreditPoolManager(CEvoDB& _evoDb, const ChainstateManager& chainman) :
    evoDb{_evoDb},
    m_chainman{chainman}
{
}

CCreditPoolManager::~CCreditPoolManager() = default;

CCreditPoolDiff::CCreditPoolDiff(CCreditPool starter, const CBlockIndex* pindexPrev,
                                 const Consensus::Params& consensusParams, const CAmount blockSubsidy) :
    pool(std::move(starter))
{
    assert(pindexPrev);

    if (DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_MN_RR)) {
        // If credit pool exists, it means v20 is activated
        platformReward = PlatformShare(GetMasternodePayment(pindexPrev->nHeight + 1, blockSubsidy, consensusParams, MnRewardEra::EvoReward));
    }
}

CAmount CCreditPoolDiff::GetTotalLocked() const
{
    // Subtract withdrawals first to preserve a representable net balance.
    auto total = CheckedAdd(pool.locked, -sessionUnlocked);
    if (total) total = CheckedAdd(*total, sessionLocked);
    if (total) total = CheckedAdd(*total, platformReward);
    if (!total) throw std::overflow_error("credit-pool balance overflow");
    return *total;
}

bool CCreditPoolDiff::Lock(const CTransaction& tx, TxValidationState& state)
{
    if (const auto opt_assetLockTx = GetTxPayload<CAssetLockPayload>(tx); !opt_assetLockTx) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-lock-payload");
    }

    for (const CTxOut& txout : tx.vout) {
        if (const CScript& script = txout.scriptPubKey; script.empty() || script[0] != OP_RETURN) continue;

        const auto total = CheckedAdd(sessionLocked, txout.nValue);
        if (!total) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-lock-overflow");
        }
        sessionLocked = *total;
        return true;
    }

    return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-lock-invalid");
}

bool CCreditPoolDiff::Unlock(const CTransaction& tx, TxValidationState& state, std::optional<uint64_t>* inserted_index)
{
    uint64_t index{0};
    CAmount toUnlock{0};
    if (!GetDataFromUnlockTx(tx, toUnlock, index, state)) {
        // state is set up inside GetDataFromUnlockTx
        return false;
    }

    const auto total = CheckedAdd(sessionUnlocked, toUnlock);
    if (!total || *total > pool.currentLimit) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-too-much");
    }

    if (pool.indexes.Contains(index) || newIndexes.count(index)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-creditpool-unlock-duplicated-index");
    }

    newIndexes.insert(index);
    if (inserted_index) *inserted_index = index;
    sessionUnlocked = *total;
    return true;
}

bool CCreditPoolDiff::ProcessLockUnlockTransaction(const CTransaction& tx, TxValidationState& state,
                                                   std::optional<uint64_t>* inserted_index)
{
    if (!tx.IsSpecialTxVersion()) return true;

    try {
        switch (tx.nType) {
        case TRANSACTION_ASSET_LOCK:
            return Lock(tx, state);
        case TRANSACTION_ASSET_UNLOCK:
            return Unlock(tx, state, inserted_index);
        default:
            return true;
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-procassetlocksinblock");
    }
}

bool CCreditPoolDiff::ProcessLockUnlockTransactions(const std::vector<CTransactionRef>& txs, TxValidationState& state)
{
    const auto initialLocked = sessionLocked;
    const auto initialUnlocked = sessionUnlocked;
    std::vector<uint64_t> insertedIndexes;

    for (const auto& tx : txs) {
        std::optional<uint64_t> inserted_index;
        if (ProcessLockUnlockTransaction(*tx, state, &inserted_index)) {
            if (inserted_index) insertedIndexes.push_back(*inserted_index);
            continue;
        }

        // Roll back exactly what this invocation changed: amounts are scalar
        // snapshots, and only the indexes inserted above are erased so that
        // state committed by earlier packages is left untouched.
        for (const uint64_t index : insertedIndexes) {
            newIndexes.erase(index);
        }
        sessionLocked = initialLocked;
        sessionUnlocked = initialUnlocked;
        return false;
    }
    return true;
}

std::optional<CCreditPoolDiff> GetCreditPoolDiffForBlock(CCreditPoolManager& cpoolman,
                                                         const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams,
                                                         const CAmount blockSubsidy, BlockValidationState& state)
{
    try {
        const CCreditPool creditPool = cpoolman.GetCreditPool(pindexPrev);
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
    } catch (const EvoDbInconsistencyError& e) {
        // Local EvoDB inconsistency (missing or mismatched state): fail with
        // M_ERROR so the block is not marked invalid.
        state.Error(e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-getcreditpooldiff");
        return std::nullopt;
    }
}
