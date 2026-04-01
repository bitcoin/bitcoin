// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <random.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/hasher.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <optional>

class MempoolUpdatesListener : public CValidationInterface
{
public:
    std::vector<MemPoolChunksUpdate> updates;
    void MempoolUpdated(const MemPoolChunksUpdate& update) override { updates.push_back(update); }
    void Clear() { updates.clear(); }
};

struct MemPoolUpdateSetup : TestingSetup {
    MempoolUpdatesListener listener;
    CTxMemPool& mpool;
    MemPoolUpdateSetup() : TestingSetup(ChainType::REGTEST), mpool{*Assert(m_node.mempool)}
    {
        m_node.validation_signals->RegisterValidationInterface(&listener);
    }
    ~MemPoolUpdateSetup()
    {
        m_node.validation_signals->UnregisterValidationInterface(&listener);
    }
};

BOOST_FIXTURE_TEST_SUITE(mempool_update_tests, MemPoolUpdateSetup)

static FastRandomContext det_rand{true};

static CMutableTransaction ConstructTx(CAmount value = 1 * COIN, std::optional<COutPoint> prev_out = std::nullopt)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    if (prev_out) {
        tx.vin[0].prevout = prev_out.value();
    } else {
        tx.vin[0].prevout = COutPoint{Txid::FromUint256(det_rand.rand256()), 0};
    }
    tx.vout.resize(1);
    tx.vout[0].nValue = value;
    tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    return tx;
}

static uint256 ExpectedChunkHash(const std::vector<CMutableTransaction>& txs)
{
    std::vector<Wtxid> wtxids;
    wtxids.reserve(txs.size());
    for (const auto& tx : txs) {
        wtxids.emplace_back(CTransaction(tx).GetWitnessHash());
    }
    return GetHashFromWitnesses(std::move(wtxids));
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_NormalAddition)
{
    TestMemPoolEntryHelper entry;
    auto tx = ConstructTx();
    CAmount fee{100};
    auto tx_entry = entry.Fee(fee).FromTx(tx);
    int32_t size{tx_entry.GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, tx_entry);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::REPLACED);
    BOOST_CHECK(update.old_chunks.empty());
    BOOST_CHECK_EQUAL(update.new_chunks.size(), 1U);
    BOOST_CHECK_EQUAL(update.new_chunks[0].m_chunk_hash, ExpectedChunkHash({tx}));
    BOOST_CHECK(update.new_chunks[0].m_fee_rate == expected_fee_rate);
    listener.Clear();
    // Child pays for low-fee parent
    auto parent_tx = ConstructTx();
    CAmount parent_fee{100};
    auto parent_entry = entry.Fee(parent_fee).FromTx(parent_tx);
    int32_t parent_size{parent_entry.GetAdjustedWeight()};
    FeeFrac parent_fee_rate{parent_fee, parent_size};
    // Child spends parent output and pays a high fee, boosting the package.
    auto child_tx = ConstructTx(1 * COIN, COutPoint{parent_tx.GetHash(), 0});
    CAmount child_fee{10000};
    auto child_entry = entry.Fee(child_fee).FromTx(child_tx);
    int32_t child_size{child_entry.GetAdjustedWeight()};
    // Once child is added the two txs form a single chunk — fee rate is combined.
    FeeFrac expected_cpfp_rate{parent_fee + child_fee, parent_size + child_size};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, parent_entry);
        TryAddToMempool(mpool, child_entry);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    // Two signals: one for parent, one for child.
    BOOST_CHECK_EQUAL(listener.updates.size(), 2U);
    // After the child is added the chunk merges parent+child
    const auto& cpfp_update = listener.updates[1];
    BOOST_CHECK(cpfp_update.reason == MemPoolRemovalReason::REPLACED);
    BOOST_CHECK_EQUAL(cpfp_update.old_chunks.size(), 1U);
    const auto& old_chunk = cpfp_update.old_chunks[0];
    BOOST_CHECK_EQUAL(old_chunk.m_chunk_hash, ExpectedChunkHash({parent_tx}));
    BOOST_CHECK(old_chunk.m_fee_rate == parent_fee_rate);
    BOOST_CHECK_EQUAL(cpfp_update.new_chunks.size(), 1U);
    const auto& cpfp_chunk = cpfp_update.new_chunks[0];
    BOOST_CHECK_EQUAL(cpfp_chunk.m_chunk_hash, ExpectedChunkHash({parent_tx, child_tx}));
    BOOST_CHECK(cpfp_chunk.m_fee_rate == expected_cpfp_rate);
    listener.Clear();
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_RBFReplacement)
{
    TestMemPoolEntryHelper entry;
    COutPoint shared_input{Txid::FromUint256(det_rand.rand256()), 0};
    auto original_tx = ConstructTx();
    original_tx.vin[0].prevout = shared_input;
    CAmount original_fee{1000};
    auto original_entry = entry.Fee(original_fee).FromTx(original_tx);
    FeeFrac expected_original_rate{original_fee, original_entry.GetAdjustedWeight()};
    // Replacement pays higher fee to satisfy RBF rules.
    auto replacement_tx = ConstructTx();
    replacement_tx.vin[0].prevout = shared_input;
    CAmount replacement_fee{5000};
    auto replacement_entry = entry.Fee(replacement_fee).FromTx(replacement_tx);
    FeeFrac expected_replacement_rate{replacement_fee, replacement_entry.GetAdjustedWeight()};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, original_entry);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    {
        LOCK2(cs_main, mpool.cs);
        auto changeset = mpool.GetChangeSet();
        changeset->StageAddition(replacement_entry.GetSharedTx(), replacement_entry.GetFee(), replacement_entry.GetTime().count(),
                                 replacement_entry.GetHeight(), replacement_entry.GetSequence(), replacement_entry.GetSpendsCoinbase(),
                                 replacement_entry.GetSigOpCost(), replacement_entry.GetLockPoints());
        auto conflict_iter = mpool.GetIter(original_entry.GetSharedTx()->GetHash());
        Assert(conflict_iter.has_value());
        changeset->StageRemoval(conflict_iter.value());
        changeset->Apply();
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::REPLACED);
    // Old diagram: original tx's chunk is gone.
    BOOST_CHECK_EQUAL(update.old_chunks.size(), 1U);
    BOOST_CHECK_EQUAL(update.old_chunks[0].m_chunk_hash, ExpectedChunkHash({original_tx}));
    BOOST_CHECK(update.old_chunks[0].m_fee_rate == expected_original_rate);
    // New diagram: replacement tx's chunk appears.
    BOOST_CHECK_EQUAL(update.new_chunks.size(), 1U);
    BOOST_CHECK_EQUAL(update.new_chunks[0].m_chunk_hash, ExpectedChunkHash({replacement_tx}));
    BOOST_CHECK(update.new_chunks[0].m_fee_rate == expected_replacement_rate);
    listener.Clear();
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_ChunkHashDeterminism)
{
    TestMemPoolEntryHelper entry;
    auto tx = ConstructTx();
    const uint256 expected_hash = ExpectedChunkHash({tx});
    CAmount fee{1000};
    int32_t size{entry.Fee(fee).FromTx(tx).GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};
    for (int round = 0; round < 2; ++round) {
        {
            LOCK2(cs_main, mpool.cs);
            TryAddToMempool(mpool, entry.Fee(fee).FromTx(tx));
        }
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        listener.Clear();
        {
            LOCK(mpool.cs);
            mpool.removeForBlock({MakeTransactionRef(tx)}, /*nBlockHeight=*/1);
        }
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
        BOOST_CHECK_EQUAL(listener.updates[0].new_chunks.size(), 0U);
        BOOST_CHECK_EQUAL(listener.updates[0].old_chunks.size(), 1U);
        BOOST_CHECK(listener.updates[0].reason == MemPoolRemovalReason::BLOCK);
        BOOST_CHECK(listener.updates[0].block_height.value() == 1);
        const auto& chunk = listener.updates[0].old_chunks[0];
        // Both rounds must produce the same hash, matching the expected value.
        BOOST_CHECK_EQUAL(chunk.m_chunk_hash, expected_hash);
        BOOST_CHECK(chunk.m_fee_rate == expected_fee_rate);
    }
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_BlockConnection)
{
    TestMemPoolEntryHelper entry;
    // Build a CPFP cluster: low-fee parent + high-fee child.
    // Together they form one chunk with the combined package fee rate.
    auto parent_tx = ConstructTx();
    CAmount parent_fee{100};
    auto parent_entry = entry.Fee(parent_fee).FromTx(parent_tx);
    int32_t parent_size{parent_entry.GetAdjustedWeight()};
    auto child_tx = ConstructTx(1 * COIN, COutPoint{parent_tx.GetHash(), 0});
    CAmount child_fee{10000};
    auto child_entry = entry.Fee(child_fee).FromTx(child_tx);
    int32_t child_size{child_entry.GetAdjustedWeight()};
    FeeFrac expected_cluster_rate{parent_fee + child_fee, parent_size + child_size};
    FeeFrac expected_child_solo_rate{child_fee, child_size};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, parent_entry);
        TryAddToMempool(mpool, child_entry);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    // Mine the parent — child remains but is now its own solo chunk.
    {
        LOCK(mpool.cs);
        mpool.removeForBlock({MakeTransactionRef(parent_tx)}, /*nBlockHeight=*/2);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::BLOCK);
    BOOST_CHECK(listener.updates[0].block_height.value() == 2);
    // Old diagram: the merged parent+child chunk.
    BOOST_CHECK_EQUAL(update.old_chunks.size(), 1U);
    BOOST_CHECK_EQUAL(update.old_chunks[0].m_chunk_hash, ExpectedChunkHash({parent_tx, child_tx}));
    BOOST_CHECK(update.old_chunks[0].m_fee_rate == expected_cluster_rate);

    // New diagram: child is now a solo chunk with its own fee rate.
    BOOST_CHECK_EQUAL(update.new_chunks.size(), 1U);
    BOOST_CHECK_EQUAL(update.new_chunks[0].m_chunk_hash, ExpectedChunkHash({child_tx}));
    BOOST_CHECK(update.new_chunks[0].m_fee_rate == expected_child_solo_rate);

    BOOST_CHECK(!mpool.exists(parent_tx.GetHash()));
    BOOST_CHECK(mpool.exists(child_tx.GetHash()));
    listener.Clear();
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_BlockConflict)
{
    TestMemPoolEntryHelper entry;
    COutPoint shared_input{Txid::FromUint256(det_rand.rand256()), 0};
    auto mempool_tx = ConstructTx();
    mempool_tx.vin[0].prevout = shared_input;
    const uint256 expected_hash = ExpectedChunkHash({mempool_tx});
    CAmount fee{1000};
    int32_t size{entry.Fee(fee).FromTx(mempool_tx).GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};

    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, entry.Fee(fee).FromTx(mempool_tx));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();

    // block_tx spends the same input — evicts mempool_tx as a conflict.
    auto block_tx = ConstructTx();
    block_tx.vin[0].prevout = shared_input;
    block_tx.vout[0].nValue = 9 * COIN / 10;
    {
        LOCK(mpool.cs);
        mpool.removeForBlock({MakeTransactionRef(block_tx)}, /*nBlockHeight=*/3);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    // One BLOCK update (empty staged set) and one CONFLICT update.
    BOOST_CHECK(listener.updates.size() >= 2U);
    // Conflict notification comes after block.
    const auto& update = listener.updates[1];
    const auto& chunk = update.old_chunks[0];
    BOOST_CHECK(update.new_chunks.empty());
    BOOST_CHECK_EQUAL(chunk.m_chunk_hash, expected_hash);
    BOOST_CHECK(chunk.m_fee_rate == expected_fee_rate);
    BOOST_CHECK(update.reason == MemPoolRemovalReason::CONFLICT);
    BOOST_CHECK(!mpool.exists(mempool_tx.GetHash()));
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_Expiry)
{
    TestMemPoolEntryHelper entry;
    auto old_tx = ConstructTx();
    auto fresh_tx = ConstructTx();
    CAmount fee{1000};
    int32_t size{entry.Fee(fee).FromTx(old_tx).GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};
    const auto old_time = Now<NodeSeconds>() - std::chrono::hours(25);
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, entry.Fee(fee).Time(old_time).FromTx(old_tx));
        TryAddToMempool(mpool, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(fresh_tx));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    {
        LOCK2(cs_main, mpool.cs);
        mpool.Expire((Now<NodeSeconds>() - std::chrono::seconds(1)).time_since_epoch());
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK_EQUAL(update.old_chunks.size(), 1U);
    const auto& chunk = update.old_chunks[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::EXPIRY);
    BOOST_CHECK_EQUAL(chunk.m_chunk_hash, ExpectedChunkHash({old_tx}));
    BOOST_CHECK(chunk.m_fee_rate == expected_fee_rate);
    BOOST_CHECK(update.new_chunks.empty());
    BOOST_CHECK(!mpool.exists(old_tx.GetHash()));
    BOOST_CHECK(mpool.exists(fresh_tx.GetHash()));
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_SizeLimit)
{
    TestMemPoolEntryHelper entry;
    auto low_fee_tx = ConstructTx();
    auto high_fee_tx = ConstructTx();
    CAmount fee_low{1000};
    CAmount fee_high{9000};
    FeeFrac expected_fee_rate_low{fee_low, entry.Fee(fee_low).FromTx(low_fee_tx).GetAdjustedWeight()};
    FeeFrac expected_fee_rate_high{fee_high, entry.Fee(fee_high).FromTx(high_fee_tx).GetAdjustedWeight()};
    auto low_fee_entry = entry.Fee(fee_low).FromTx(low_fee_tx);
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, entry.Fee(fee_high).FromTx(high_fee_tx));
        TryAddToMempool(mpool, entry.Fee(fee_low).FromTx(low_fee_tx));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    // Trim to one tx worth of memory — evicts the low fee tx.
    {
        LOCK2(cs_main, mpool.cs);
        mpool.TrimToSize(mpool.DynamicMemoryUsage() - low_fee_entry.DynamicMemoryUsage());
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK_EQUAL(update.old_chunks.size(), 1U);
    const auto& chunk_low = update.old_chunks[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::SIZELIMIT);
    BOOST_CHECK_EQUAL(chunk_low.m_chunk_hash, ExpectedChunkHash({low_fee_tx}));
    BOOST_CHECK(chunk_low.m_fee_rate == expected_fee_rate_low);
    BOOST_CHECK(update.new_chunks.empty());
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_RemoveRecursive)
{
    TestMemPoolEntryHelper entry;
    auto tx = ConstructTx();
    CAmount fee{1000};
    int32_t size{entry.Fee(fee).FromTx(tx).GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, entry.Fee(fee).FromTx(tx));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    {
        LOCK(mpool.cs);
        mpool.removeRecursive(CTransaction(tx), MemPoolRemovalReason::REORG);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    BOOST_CHECK_EQUAL(listener.updates[0].old_chunks.size(), 1U);
    const auto& chunk = listener.updates[0].old_chunks[0];
    BOOST_CHECK_EQUAL(chunk.m_chunk_hash, ExpectedChunkHash({tx}));
    BOOST_CHECK(chunk.m_fee_rate == expected_fee_rate);
    BOOST_CHECK(listener.updates[0].new_chunks.empty());
}

BOOST_AUTO_TEST_CASE(MempoolUpdated_Reorg)
{
    TestMemPoolEntryHelper entry;
    auto tx_to_invalidate = ConstructTx();
    auto valid_tx = ConstructTx();
    CAmount fee{1000};
    int32_t size{entry.Fee(fee).FromTx(tx_to_invalidate).GetAdjustedWeight()};
    FeeFrac expected_fee_rate{fee, size};
    {
        LOCK2(cs_main, mpool.cs);
        TryAddToMempool(mpool, entry.Fee(fee).FromTx(tx_to_invalidate));
        TryAddToMempool(mpool, entry.Fee(1000).FromTx(valid_tx));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    const Txid tx_to_invalidate_txid = tx_to_invalidate.GetHash();
    {
        LOCK2(cs_main, mpool.cs);
        auto& chainstate = m_node.chainman->ActiveChainstate();
        mpool.removeForReorg(chainstate.m_chain, [&](CTxMemPool::txiter it)
                                                     EXCLUSIVE_LOCKS_REQUIRED(mpool.cs, ::cs_main) {
                                                         AssertLockHeld(mpool.cs);
                                                         AssertLockHeld(::cs_main);
                                                         return it->GetTx().GetHash() == tx_to_invalidate_txid;
                                                     });
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    BOOST_CHECK_EQUAL(listener.updates[0].old_chunks.size(), 1U);
    const auto& chunk = listener.updates[0].old_chunks[0];
    BOOST_CHECK_EQUAL(chunk.m_chunk_hash, ExpectedChunkHash({tx_to_invalidate}));
    BOOST_CHECK(chunk.m_fee_rate == expected_fee_rate);
    BOOST_CHECK(listener.updates[0].new_chunks.empty());
    BOOST_CHECK(!mpool.exists(tx_to_invalidate.GetHash()));
    BOOST_CHECK(mpool.exists(valid_tx.GetHash()));
}

// Simulate a scenario where a reorg causes a tx to be added to the mempool
// and connected to a cluster; this connection causes mempool limits to be exceeded.
// In production this happens in MaybeUpdateMempoolForReorg:
// txs are added via AcceptToMemoryPool, then UpdateTransactionsFromBlock
// connects the parent->child dependency.
//
// Here chain[1..cluster_limit] are in-mempool and connected to a tx in a block, but the
// reorg does not include the parent of chain[1..cluster_limit].
// chain[0] is then added independently as a cluster of 1. UpdateTransactionsFromBlock scans
// the mempool for txs that spends an output of chain[1], finds chain[1], and calls AddDependency
// merging both clusters into one of size cluster_limit+1, which exceeds the limit.
// Trim() evicts the lowest-fee tail tx.
BOOST_AUTO_TEST_CASE(MempoolUpdated_UpdateTransactionsFromBlock)
{
    TestMemPoolEntryHelper entry;
    const int chain_len{static_cast<int>(mpool.m_opts.limits.cluster_count + 1)};
    std::vector<CMutableTransaction> chain;
    chain.reserve(chain_len);
    std::vector<Txid> txids;
    chain.push_back(ConstructTx()); // chain[0] — the re-added parent
    for (int i = 1; i < chain_len; ++i) {
        chain.push_back(ConstructTx(1 * COIN, COutPoint{chain[i - 1].GetHash(), 0}));
    }
    CAmount normal_fee{1000};
    CAmount low_fee{100};
    int32_t last_tx_size{0};
    {
        LOCK2(cs_main, mpool.cs);
        for (int i = 1; i < chain_len; ++i) {
            CAmount fee = (i == chain_len - 1) ? low_fee : normal_fee;
            auto curr_entry = entry.Fee(fee).FromTx(chain[i]);
            if (i == chain_len - 1) last_tx_size = curr_entry.GetAdjustedWeight();
            TryAddToMempool(mpool, curr_entry);
        }
        auto root_entry = entry.Fee(normal_fee).FromTx(chain[0]);
        TryAddToMempool(mpool, root_entry);
        txids.push_back(chain[0].GetHash());
    }
    // Verify the two clusters are independent before UpdateTransactionsFromBlock.
    BOOST_CHECK_EQUAL(WITH_LOCK(mpool.cs, return mpool.GetCluster(chain[0].GetHash()).size()), 1);
    BOOST_CHECK_EQUAL(WITH_LOCK(mpool.cs, return mpool.GetCluster(chain[1].GetHash()).size()), chain_len - 1);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    listener.Clear();
    {
        LOCK2(cs_main, mpool.cs);
        // Scans mapNextTx for chain[0]'s outputs, finds chain[1], calls
        // AddDependency(chain[0], chain[1]). Merged cluster = chain_len > limit.
        // Trim() evicts the lowest-fee tail tx (chain.back()).
        mpool.UpdateTransactionsFromBlock(txids);
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    // Both clusters are now merged (minus the evicted tail tx).
    BOOST_CHECK_EQUAL(WITH_LOCK(mpool.cs, return mpool.GetCluster(chain[0].GetHash()).size()), chain_len - 1);
    BOOST_CHECK_EQUAL(WITH_LOCK(mpool.cs, return mpool.GetCluster(chain[1].GetHash()).size()), chain_len - 1);
    BOOST_CHECK_EQUAL(listener.updates.size(), 1U);
    const auto& update = listener.updates[0];
    BOOST_CHECK(update.reason == MemPoolRemovalReason::SIZELIMIT);
    // old_chunks: the merged cluster before eviction — includes the low-fee tail tx.
    BOOST_CHECK(!update.old_chunks.empty());
    const auto& evicted_chunk = update.old_chunks.back();
    BOOST_CHECK_EQUAL(evicted_chunk.m_chunk_hash, ExpectedChunkHash({chain.back()}));
    BOOST_CHECK(evicted_chunk.m_fee_rate == FeeFrac(low_fee, last_tx_size));
    // new_chunks: the merged cluster after eviction — excludes the low-fee tail tx.
    BOOST_CHECK(!update.new_chunks.empty());
    BOOST_CHECK_EQUAL(update.new_chunks.size(), update.old_chunks.size() - 1);
    BOOST_CHECK(!mpool.exists(chain.back().GetHash()));
    // All other txs must still be present.
    for (int i = 0; i < chain_len - 1; ++i) {
        BOOST_CHECK(mpool.exists(chain[i].GetHash()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
