// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <test/util/mining.h>

#include <node/miner.h>
#include <node/mini_miner.h>
#include <node/types.h>
#include <primitives/transaction.h>
#include <random.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>

#include <deque>
#include <vector>

namespace {

const TestingSetup* g_setup;
std::deque<COutPoint> g_available_coins;
void initialize_miner()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    for (uint32_t i = 0; i < uint32_t{100}; ++i) {
        g_available_coins.emplace_back(Txid::FromUint256(uint256::ZERO), i);
    }
}

// Test that the MiniMiner can run with various outpoints and feerates.
FUZZ_TARGET(mini_miner, .init = initialize_miner)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    bilingual_str error;
    CTxMemPool pool{CTxMemPool::Options{}, error};
    Assert(error.empty());
    std::vector<COutPoint> outpoints;
    std::deque<COutPoint> available_coins = g_available_coins;
    LOCK2(::cs_main, pool.cs);
    // Cluster size cannot exceed 500
    LIMITED_WHILE(!available_coins.empty(), 100)
    {
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, available_coins.size());
        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.front();
            mtx.vin.emplace_back(prevout, CScript());
            available_coins.pop_front();
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
        }
        CTransactionRef tx = MakeTransactionRef(mtx);
        TestMemPoolEntryHelper entry;
        const CAmount fee{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/100000)};
        assert(MoneyRange(fee));
        AddToMempool(pool, entry.Fee(fee).FromTx(tx));

        // All outputs are available to spend
        for (uint32_t n{0}; n < num_outputs; ++n) {
            if (fuzzed_data_provider.ConsumeBool()) {
                available_coins.emplace_back(tx->GetHash(), n);
            }
        }

        if (fuzzed_data_provider.ConsumeBool() && !tx->vout.empty()) {
            // Add outpoint from this tx (may or not be spent by a later tx)
            outpoints.emplace_back(tx->GetHash(),
                                          (uint32_t)fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, tx->vout.size()));
        } else {
            // Add some random outpoint (will be interpreted as confirmed or not yet submitted
            // to mempool).
            auto outpoint = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
            if (outpoint.has_value() && std::find(outpoints.begin(), outpoints.end(), *outpoint) == outpoints.end()) {
                outpoints.push_back(*outpoint);
            }
        }

    }

    const CFeeRate target_feerate{CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/1000)}};
    std::optional<CAmount> total_bumpfee;
    CAmount sum_fees = 0;
    {
        node::MiniMiner mini_miner{pool, outpoints};
        assert(mini_miner.IsReadyToCalculate());
        const auto bump_fees = mini_miner.CalculateBumpFees(target_feerate);
        for (const auto& outpoint : outpoints) {
            auto it = bump_fees.find(outpoint);
            assert(it != bump_fees.end());
            assert(it->second >= 0);
            sum_fees += it->second;
        }
        assert(!mini_miner.IsReadyToCalculate());
    }
    {
        node::MiniMiner mini_miner{pool, outpoints};
        assert(mini_miner.IsReadyToCalculate());
        total_bumpfee = mini_miner.CalculateTotalBumpFees(target_feerate);
        assert(total_bumpfee.has_value());
        assert(!mini_miner.IsReadyToCalculate());
    }
    // Overlapping ancestry across multiple outpoints can only reduce the total bump fee.
    assert (sum_fees >= *total_bumpfee);
}

// Test that MiniMiner and BlockAssembler build the same block given the same transactions and constraints.
FUZZ_TARGET(mini_miner_selection, .init = initialize_miner)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    bilingual_str error;
    CTxMemPool pool{CTxMemPool::Options{}, error};
    Assert(error.empty());
    // Make a copy to preserve determinism.
    std::deque<COutPoint> available_coins = g_available_coins;
    std::vector<CTransactionRef> transactions;

    LOCK2(::cs_main, pool.cs);
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CMutableTransaction mtx = CMutableTransaction();
        assert(!available_coins.empty());
        const size_t num_inputs = std::min(size_t{2}, available_coins.size());
        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(2, 5);
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.at(0);
            mtx.vin.emplace_back(prevout, CScript());
            available_coins.pop_front();
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
        }
        CTransactionRef tx = MakeTransactionRef(mtx);

        // First 2 outputs are available to spend. The rest are added to outpoints to calculate bumpfees.
        // There is no overlap between spendable coins and outpoints passed to MiniMiner because the
        // MiniMiner interprets spent coins as to-be-replaced and excludes them.
        for (uint32_t n{0}; n < num_outputs - 1; ++n) {
            if (fuzzed_data_provider.ConsumeBool()) {
                available_coins.emplace_front(tx->GetHash(), n);
            } else {
                available_coins.emplace_back(tx->GetHash(), n);
            }
        }

        const auto block_adjusted_max_weight = MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT;
        // Stop if pool reaches block_adjusted_max_weight because BlockAssembler will stop when the
        // block template reaches that, but the MiniMiner will keep going.
        if (pool.GetTotalTxSize() + GetVirtualTransactionSize(*tx) >= block_adjusted_max_weight) break;
        TestMemPoolEntryHelper entry;
        const CAmount fee{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/100000)};
        assert(MoneyRange(fee));
        AddToMempool(pool, entry.Fee(fee).FromTx(tx));
        transactions.push_back(tx);
    }
    std::vector<COutPoint> outpoints;
    for (const auto& coin : g_available_coins) {
        if (!pool.GetConflictTx(coin)) outpoints.push_back(coin);
    }
    for (const auto& tx : transactions) {
        assert(pool.exists(tx->GetHash()));
        for (uint32_t n{0}; n < tx->vout.size(); ++n) {
            COutPoint coin{tx->GetHash(), n};
            if (!pool.GetConflictTx(coin)) outpoints.push_back(coin);
        }
    }
    const CFeeRate target_feerate{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/100000)};

    node::BlockAssembler::Options miner_options;
    miner_options.blockMinFeeRate = target_feerate;
    miner_options.nBlockMaxWeight = MAX_BLOCK_WEIGHT;
    miner_options.test_block_validity = false;
    miner_options.coinbase_output_script = CScript() << OP_0;

    node::BlockAssembler miner{g_setup->m_node.chainman->ActiveChainstate(), &pool, miner_options};
    node::MiniMiner mini_miner{pool, outpoints};
    assert(mini_miner.IsReadyToCalculate());

    // Use BlockAssembler as oracle. BlockAssembler and MiniMiner should select the same
    // transactions, stopping once packages do not meet target_feerate.
    const auto blocktemplate{miner.CreateNewBlock()};
    mini_miner.BuildMockTemplate(target_feerate);
    assert(!mini_miner.IsReadyToCalculate());
    auto mock_template_txids = mini_miner.GetMockTemplateTxids();
    // MiniMiner doesn't add a coinbase tx.
    assert(mock_template_txids.count(blocktemplate->block.vtx[0]->GetHash()) == 0);
    auto [iter, new_entry] = mock_template_txids.emplace(blocktemplate->block.vtx[0]->GetHash());
    assert(new_entry);

    assert(mock_template_txids.size() == blocktemplate->block.vtx.size());
    for (const auto& tx : blocktemplate->block.vtx) {
        assert(mock_template_txids.count(tx->GetHash()));
    }
}
} // namespace
