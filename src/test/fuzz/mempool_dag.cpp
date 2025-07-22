// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <policy/truc_policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/rbf.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <chrono> // Added for std::chrono::seconds

using node::BlockAssembler;
using node::NodeContext;
using util::ToString;

namespace {

const TestingSetup* g_setup;
std::vector<COutPoint> g_outpoints_coinbase_init_mature;

struct MockedTxPool : public CTxMemPool {
    MockedTxPool(CTxMemPool::Options&& opts, bilingual_str& error) : CTxMemPool(std::move(opts), error) {}

};

void initialize_mempool_dag()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(), return g_setup->m_node.chainman->ActiveTip()->Time()));

    BlockAssembler::Options options;
    options.coinbase_output_script = P2WSH_OP_TRUE;

    // Create mature coinbase outputs for spending
    for (int i = 0; i < COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(g_setup->m_node, options)};
        g_outpoints_coinbase_init_mature.push_back(prevout);
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

void SetMempoolConstraints(ArgsManager& args, FuzzedDataProvider& fuzzed_data_provider)
{
    // setting mempool limits to ranges which are reasonable, for DAG testing
    args.ForceSetArg("-limitancestorcount",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(10, 50)));
    args.ForceSetArg("-limitancestorsize",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(50, 202)));
    args.ForceSetArg("-limitdescendantcount",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(10, 50)));
    args.ForceSetArg("-limitdescendantsize",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(50, 202)));
    args.ForceSetArg("-maxmempool",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(50, 200)));
}

std::unique_ptr<CTxMemPool> MakeMempool(FuzzedDataProvider& fuzzed_data_provider, const NodeContext& node)
{
    // Take the default options for tests
    CTxMemPool::Options mempool_opts{MemPoolOptionsForTest(node)};

    // Override specific options for DAG testing
    mempool_opts.check_ratio = 1;
    mempool_opts.require_standard = fuzzed_data_provider.ConsumeBool();

    // Construct CTxMemPool - (matches tx_pool.cpp)
    bilingual_str error;
    auto mempool{std::make_unique<MockedTxPool>(std::move(mempool_opts), error)};
    Assert(error.empty() || error.original.starts_with("-maxmempool must be at least "));
    return mempool;
}

void CheckMempoolDAGInvariants(const CTxMemPool& pool, const CTransactionRef& tx)
{
    LOCK(pool.cs);

    auto it = pool.GetIter(tx->GetHash());
    if (!it.has_value()) return;

    // Test descendant limits and invariants
    CTxMemPool::setEntries descendants;
    pool.CalculateDescendants(*it, descendants);

    // Descendant count limit
    Assert(descendants.size() <= DEFAULT_DESCENDANT_LIMIT);

    // Descendant size limit (convert KVB to bytes)
    size_t descendant_size = 0;
    CAmount descendant_fees = 0;
    for (const auto& desc_it : descendants) {
        descendant_size += desc_it->GetTxSize();
        descendant_fees += desc_it->GetModifiedFee();
    }
    Assert(descendant_size <= DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * 1000);

    // Consistency checks
    Assert(descendants.count(*it) == 1); // Transaction is its own descendant

    // Fee rate consistency (descendants should not have negative effective fee rates)
    if (descendant_size > 0) {
        CFeeRate descendant_feerate(descendant_fees, descendant_size);
        Assert(descendant_feerate >= CFeeRate(0));
    }

    // Basic consistency - transaction should exist in mempool if we got here
    Assert(pool.exists(tx->GetHash()));

    // Transaction size should be reasonable
    Assert(tx->GetTotalSize() > 0);
    Assert(tx->GetTotalSize() < MAX_STANDARD_TX_WEIGHT);

    // Transaction should have at least one input and one output
    Assert(!tx->vin.empty());
    Assert(!tx->vout.empty());

    // Transaction entry properties
    const auto& entry = **it;
    Assert(entry.GetTxSize() > 0);
    Assert(entry.GetCountWithDescendants() >= 1); // At least includes itself
    Assert(entry.GetCountWithAncestors() >= 1);   // At least includes itself
}

void CheckMempoolDAGTopology(const CTxMemPool& pool, const std::vector<CTransactionRef>& created_txs)
{
    LOCK(pool.cs);

    // Graph connectivity consistency
    for (const auto& tx : created_txs) {
        if (!pool.exists(tx->GetHash())) continue;

        auto it = pool.GetIter(tx->GetHash());
        if (!it.has_value()) continue;

        // Check that all inputs reference valid parents in the pool or confirmed UTXOs
        for (const auto& input : tx->vin) {
            if (input.prevout.IsNull()) continue; // Coinbase-like

            if (pool.exists(input.prevout.hash)) {
                // Input spends from mempool - parent should exist
                auto parent_it = pool.GetIter(input.prevout.hash);
                Assert(parent_it.has_value());

                // checks consistency - parent should have enough outputs
                auto parent_tx = parent_it.value()->GetSharedTx();
                Assert(input.prevout.n < parent_tx->vout.size());
            }
        }

        // basic transaction validity
        Assert(tx->GetTotalSize() > 0);
        Assert(!tx->vin.empty());
        Assert(!tx->vout.empty());
    }
}

} // namespace

FUZZ_TARGET(mempool_dag, .init = initialize_mempool_dag)
{
    // Initialize random state and mock time
    SeedRandomStateForTest(SeedRand::ZEROS);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // mock time set at 1 to avoid time-dependent behavior
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<DummyChainState&>(node.chainman->ActiveChainstate())};
    SetMockTime(WITH_LOCK(chainstate.m_chainman.GetMutex(), return chainstate.m_chain.Tip()->Time()) + std::chrono::seconds(1));

    // Set up mempool constraints for DAG testing
    SetMempoolConstraints(*node.args, fuzzed_data_provider);
    auto tx_pool_ = MakeMempool(fuzzed_data_provider, node);
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(tx_pool_.get());

    chainstate.SetMempool(&tx_pool);

    // Track all successfully created transactions for DAG building
    std::vector<CTransactionRef> created_txs;

    // Seed with some initial coinbase-spending transactions
    for (size_t i = 0; i < std::min(size_t(3), g_outpoints_coinbase_init_mature.size()); ++i) {
        CMutableTransaction seed_tx;
        seed_tx.version = 2;
        seed_tx.nLockTime = 0;
        seed_tx.vin.emplace_back(g_outpoints_coinbase_init_mature[i], CScript{});
        seed_tx.vin[0].scriptWitness.stack = {WITNESS_STACK_ELEM_OP_TRUE};
        seed_tx.vout.emplace_back(CAmount{5000000000}, P2WSH_OP_TRUE); // 50 BTC output

        CTransactionRef seed_tx_ref = MakeTransactionRef(seed_tx);

        LOCK(cs_main);
        MempoolAcceptResult result = AcceptToMemoryPool(chainstate, seed_tx_ref, GetTime(), false, false);

        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            created_txs.push_back(seed_tx_ref);
            CheckMempoolDAGInvariants(tx_pool, seed_tx_ref);
        }
    }

    // this is the main loop:- creates DAG by building transaction chains
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 8, 30) {
        CMutableTransaction mtx;
        mtx.version = fuzzed_data_provider.ConsumeBool() ? TRUC_VERSION : 2;
        mtx.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();

        // Use parent mask for systematic creating of dependencies
        const uint32_t parents_mask = fuzzed_data_provider.ConsumeIntegral<uint32_t>();

        // then add inputs based on parent mask (key for this DAG testing)
        bool has_inputs = false;
        for (size_t i = 0; i < created_txs.size() && i < 16; ++i) {
            if ((parents_mask >> i) & 1) {
                const auto& parent_tx = created_txs[i];
                if (!parent_tx->vout.empty()) {
                    uint32_t output_idx = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, parent_tx->vout.size() - 1);

                    CTxIn input;
                    input.prevout = COutPoint(parent_tx->GetHash(), output_idx);
                    input.nSequence = ConsumeSequence(fuzzed_data_provider);
                    input.scriptSig = CScript{};
                    input.scriptWitness.stack = {WITNESS_STACK_ELEM_OP_TRUE};

                    mtx.vin.push_back(input);
                    has_inputs = true;
                }
            }
        }

        // If no inputs selected, create coinbase-like transaction
        if (!has_inputs) {
            mtx.vin.emplace_back(COutPoint{}, CScript{});
            mtx.vin[0].scriptWitness.stack = {WITNESS_STACK_ELEM_OP_TRUE};
        }

        // Add outputs
        const int num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 3);
        const CAmount output_value = has_inputs ?
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1000, 100000000) :
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1000000000, 5000000000);

        for (int i = 0; i < num_outputs; ++i) {
            mtx.vout.emplace_back(output_value, P2WSH_OP_TRUE);
        }

        CTransactionRef tx = MakeTransactionRef(mtx);

        // Occasionally update mempool state (but less frequently to avoid globals issues)
        if (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10) == 0) {
            const auto time = ConsumeTime(fuzzed_data_provider,
                                        chainstate.m_chain.Tip()->GetMedianTimePast() + 1,
                                        std::numeric_limits<decltype(chainstate.m_chain.Tip()->nTime)>::max());
            SetMockTime(time);
        }

        // Try to add transaction to mempool
        const bool bypass_limits = fuzzed_data_provider.ConsumeBool();

        LOCK(cs_main);
        MempoolAcceptResult result = AcceptToMemoryPool(chainstate, tx, GetTime(), bypass_limits, false);

        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            created_txs.push_back(tx);

            // Check all DAG invariants for the new transaction
            CheckMempoolDAGInvariants(tx_pool, tx);

            // Periodically check overall DAG topology
            if (created_txs.size() % 5 == 0) {
                CheckMempoolDAGTopology(tx_pool, created_txs);
            }

            // Check TRUC policy compliance
            if (tx->version == TRUC_VERSION) {
                CheckMempoolTRUCInvariants(tx_pool);
            }
        }
    }

    CheckMempoolDAGTopology(tx_pool, created_txs);

    // cleans up
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}
