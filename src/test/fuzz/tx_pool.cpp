// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <miner.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <util/rbf.h>
#include <validation.h>
#include <validationinterface.h>

namespace {

const TestingSetup* g_setup;
std::vector<COutPoint> g_outpoints_coinbase_init_mature;
std::vector<COutPoint> g_outpoints_coinbase_init_immature;

struct MockedTxPool : public CTxMemPool {
    void RollingFeeUpdate() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        lastRollingFeeUpdate = GetTime();
        blockSinceLastRollingFeeBump = true;
    }
};

void initialize_tx_pool()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();

    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        CTxIn in = MineBlock(g_setup->m_node, P2WSH_OP_TRUE);
        // Remember the txids to avoid expensive disk access later on
        auto& outpoints = i < COINBASE_MATURITY ?
                              g_outpoints_coinbase_init_mature :
                              g_outpoints_coinbase_init_immature;
        outpoints.push_back(in.prevout);
    }
    SyncWithValidationInterfaceQueue();
}

struct TransactionsDelta final : public CValidationInterface {
    std::set<CTransactionRef>& m_removed;
    std::set<CTransactionRef>& m_added;

    explicit TransactionsDelta(std::set<CTransactionRef>& r, std::set<CTransactionRef>& a)
        : m_removed{r}, m_added{a} {}

    void TransactionAddedToMempool(const CTransactionRef& tx, uint64_t /* mempool_sequence */) override
    {
        Assert(m_added.insert(tx).second);
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t /* mempool_sequence */) override
    {
        Assert(m_removed.insert(tx).second);
    }
};

void SetMempoolConstraints(ArgsManager& args, FuzzedDataProvider& fuzzed_data_provider)
{
    args.ForceSetArg("-limitancestorcount",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 50)));
    args.ForceSetArg("-limitancestorsize",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 202)));
    args.ForceSetArg("-limitdescendantcount",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 50)));
    args.ForceSetArg("-limitdescendantsize",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 202)));
    args.ForceSetArg("-maxmempool",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 200)));
    args.ForceSetArg("-mempoolexpiry",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 999)));
}

void Finish(FuzzedDataProvider& fuzzed_data_provider, MockedTxPool& tx_pool, CChainState& chainstate)
{
    WITH_LOCK(::cs_main, tx_pool.check(chainstate));
    {
        BlockAssembler::Options options;
        options.nBlockMaxWeight = fuzzed_data_provider.ConsumeIntegralInRange(0U, MAX_BLOCK_WEIGHT);
        options.blockMinFeeRate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /* max */ COIN)};
        auto assembler = BlockAssembler{chainstate, *static_cast<CTxMemPool*>(&tx_pool), ::Params(), options};
        auto block_template = assembler.CreateNewBlock(CScript{} << OP_TRUE);
        Assert(block_template->block.vtx.size() >= 1);
    }
    const auto info_all = tx_pool.infoAll();
    if (!info_all.empty()) {
        const auto& tx_to_remove = *PickValue(fuzzed_data_provider, info_all).tx;
        WITH_LOCK(tx_pool.cs, tx_pool.removeRecursive(tx_to_remove, /* dummy */ MemPoolRemovalReason::BLOCK));
        std::vector<uint256> all_txids;
        tx_pool.queryHashes(all_txids);
        assert(all_txids.size() < info_all.size());
        WITH_LOCK(::cs_main, tx_pool.check(chainstate));
    }
    SyncWithValidationInterfaceQueue();
}

void MockTime(FuzzedDataProvider& fuzzed_data_provider, const CChainState& chainstate)
{
    const auto time = ConsumeTime(fuzzed_data_provider,
                                  chainstate.m_chain.Tip()->GetMedianTimePast() + 1,
                                  std::numeric_limits<decltype(chainstate.m_chain.Tip()->nTime)>::max());
    SetMockTime(time);
}

FUZZ_TARGET_INIT(tx_pool_standard, initialize_tx_pool)
{
    // Pick an arbitrary upper bound to limit the runtime and avoid timeouts on
    // inputs.
    int limit_max_ops{300};

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate = node.chainman->ActiveChainstate();

    MockTime(fuzzed_data_provider, chainstate);
    SetMempoolConstraints(*node.args, fuzzed_data_provider);

    // All RBF-spendable outpoints
    std::set<COutPoint> outpoints_rbf;
    // All outpoints counting toward the total supply (subset of outpoints_rbf)
    std::set<COutPoint> outpoints_supply;
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        Assert(outpoints_supply.insert(outpoint).second);
    }
    outpoints_rbf = outpoints_supply;

    // The sum of the values of all spendable outpoints
    constexpr CAmount SUPPLY_TOTAL{COINBASE_MATURITY * 50 * COIN};

    CTxMemPool tx_pool_{/* estimator */ nullptr, /* check_ratio */ 1};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(&tx_pool_);

    // Helper to query an amount
    const CCoinsViewMemPool amount_view{WITH_LOCK(::cs_main, return &chainstate.CoinsTip()), tx_pool};
    const auto GetAmount = [&](const COutPoint& outpoint) {
        Coin c;
        Assert(amount_view.GetCoin(outpoint, c));
        return c.out.nValue;
    };

    while (--limit_max_ops >= 0 && fuzzed_data_provider.ConsumeBool()) {
        {
            // Total supply is the mempool fee + all outpoints
            CAmount supply_now{WITH_LOCK(tx_pool.cs, return tx_pool.GetTotalFee())};
            for (const auto& op : outpoints_supply) {
                supply_now += GetAmount(op);
            }
            Assert(supply_now == SUPPLY_TOTAL);
        }
        Assert(!outpoints_supply.empty());

        // Create transaction to add to the mempool
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            tx_mut.nVersion = CTransaction::CURRENT_VERSION;
            tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, outpoints_rbf.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, outpoints_rbf.size() * 2);

            CAmount amount_in{0};
            for (int i = 0; i < num_in; ++i) {
                // Pop random outpoint
                auto pop = outpoints_rbf.begin();
                std::advance(pop, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, outpoints_rbf.size() - 1));
                const auto outpoint = *pop;
                outpoints_rbf.erase(pop);
                amount_in += GetAmount(outpoint);

                // Create input
                const auto sequence = ConsumeSequence(fuzzed_data_provider);
                const auto script_sig = CScript{};
                const auto script_wit_stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};
                CTxIn in;
                in.prevout = outpoint;
                in.nSequence = sequence;
                in.scriptSig = script_sig;
                in.scriptWitness.stack = script_wit_stack;

                tx_mut.vin.push_back(in);
            }
            const auto amount_fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-1000, amount_in);
            const auto amount_out = (amount_in - amount_fee) / num_out;
            for (int i = 0; i < num_out; ++i) {
                tx_mut.vout.emplace_back(amount_out, P2WSH_OP_TRUE);
            }
            const auto tx = MakeTransactionRef(tx_mut);
            // Restore previously removed outpoints
            for (const auto& in : tx->vin) {
                Assert(outpoints_rbf.insert(in.prevout).second);
            }
            return tx;
        }();

        if (fuzzed_data_provider.ConsumeBool()) {
            MockTime(fuzzed_data_provider, chainstate);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            SetMempoolConstraints(*node.args, fuzzed_data_provider);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            tx_pool.RollingFeeUpdate();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const auto& txid = fuzzed_data_provider.ConsumeBool() ?
                                   tx->GetHash() :
                                   PickValue(fuzzed_data_provider, outpoints_rbf).hash;
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            tx_pool.PrioritiseTransaction(txid, delta);
        }

        // Remember all removed and added transactions
        std::set<CTransactionRef> removed;
        std::set<CTransactionRef> added;
        auto txr = std::make_shared<TransactionsDelta>(removed, added);
        RegisterSharedValidationInterface(txr);
        const bool bypass_limits = fuzzed_data_provider.ConsumeBool();
        ::fRequireStandard = fuzzed_data_provider.ConsumeBool();

        // Make sure ProcessNewPackage on one transaction works and always fully validates the transaction.
        // The result is not guaranteed to be the same as what is returned by ATMP.
        const auto result_package = WITH_LOCK(::cs_main,
                                    return ProcessNewPackage(node.chainman->ActiveChainstate(), tx_pool, {tx}, true));
        auto it = result_package.m_tx_results.find(tx->GetWitnessHash());
        Assert(it != result_package.m_tx_results.end());
        Assert(it->second.m_result_type == MempoolAcceptResult::ResultType::VALID ||
               it->second.m_result_type == MempoolAcceptResult::ResultType::INVALID);

        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, tx_pool, tx, bypass_limits));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        SyncWithValidationInterfaceQueue();
        UnregisterSharedValidationInterface(txr);

        Assert(accepted != added.empty());
        Assert(accepted == res.m_state.IsValid());
        Assert(accepted != res.m_state.IsInvalid());
        if (accepted) {
            Assert(added.size() == 1); // For now, no package acceptance
            Assert(tx == *added.begin());
        } else {
            // Do not consider rejected transaction removed
            removed.erase(tx);
        }

        // Helper to insert spent and created outpoints of a tx into collections
        using Sets = std::vector<std::reference_wrapper<std::set<COutPoint>>>;
        const auto insert_tx = [](Sets created_by_tx, Sets consumed_by_tx, const auto& tx) {
            for (size_t i{0}; i < tx.vout.size(); ++i) {
                for (auto& set : created_by_tx) {
                    Assert(set.get().emplace(tx.GetHash(), i).second);
                }
            }
            for (const auto& in : tx.vin) {
                for (auto& set : consumed_by_tx) {
                    Assert(set.get().insert(in.prevout).second);
                }
            }
        };
        // Add created outpoints, remove spent outpoints
        {
            // Outpoints that no longer exist at all
            std::set<COutPoint> consumed_erased;
            // Outpoints that no longer count toward the total supply
            std::set<COutPoint> consumed_supply;
            for (const auto& removed_tx : removed) {
                insert_tx(/* created_by_tx */ {consumed_erased}, /* consumed_by_tx */ {outpoints_supply}, /* tx */ *removed_tx);
            }
            for (const auto& added_tx : added) {
                insert_tx(/* created_by_tx */ {outpoints_supply, outpoints_rbf}, /* consumed_by_tx */ {consumed_supply}, /* tx */ *added_tx);
            }
            for (const auto& p : consumed_erased) {
                Assert(outpoints_supply.erase(p) == 1);
                Assert(outpoints_rbf.erase(p) == 1);
            }
            for (const auto& p : consumed_supply) {
                Assert(outpoints_supply.erase(p) == 1);
            }
        }
    }
    Finish(fuzzed_data_provider, tx_pool, chainstate);
}

FUZZ_TARGET_INIT(tx_pool, initialize_tx_pool)
{
    // Pick an arbitrary upper bound to limit the runtime and avoid timeouts on
    // inputs.
    int limit_max_ops{300};

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate = node.chainman->ActiveChainstate();

    MockTime(fuzzed_data_provider, chainstate);
    SetMempoolConstraints(*node.args, fuzzed_data_provider);

    std::vector<uint256> txids;
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        txids.push_back(outpoint.hash);
    }
    for (int i{0}; i <= 3; ++i) {
        // Add some immature and non-existent outpoints
        txids.push_back(g_outpoints_coinbase_init_immature.at(i).hash);
        txids.push_back(ConsumeUInt256(fuzzed_data_provider));
    }

    CTxMemPool tx_pool_{/* estimator */ nullptr, /* check_ratio */ 1};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(&tx_pool_);

    while (--limit_max_ops >= 0 && fuzzed_data_provider.ConsumeBool()) {
        const auto mut_tx = ConsumeTransaction(fuzzed_data_provider, txids);

        if (fuzzed_data_provider.ConsumeBool()) {
            MockTime(fuzzed_data_provider, chainstate);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            SetMempoolConstraints(*node.args, fuzzed_data_provider);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            tx_pool.RollingFeeUpdate();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const auto& txid = fuzzed_data_provider.ConsumeBool() ?
                                   mut_tx.GetHash() :
                                   PickValue(fuzzed_data_provider, txids);
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            tx_pool.PrioritiseTransaction(txid, delta);
        }

        const auto tx = MakeTransactionRef(mut_tx);
        const bool bypass_limits = fuzzed_data_provider.ConsumeBool();
        ::fRequireStandard = fuzzed_data_provider.ConsumeBool();
        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(node.chainman->ActiveChainstate(), tx_pool, tx, bypass_limits));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        if (accepted) {
            txids.push_back(tx->GetHash());
        }
    }
    Finish(fuzzed_data_provider, tx_pool, chainstate);
}
} // namespace
