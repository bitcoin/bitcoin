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
#include <primitives/transaction.h>
#include <random.h>
#include <txmempool.h>
#include <util/check.h>
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
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    bilingual_str error;
    CTxMemPool pool{CTxMemPool::Options{}, error};
    Assert(error.empty());
    std::vector<COutPoint> outpoints;
    std::deque<COutPoint> available_coins = g_available_coins;
    LOCK2(::cs_main, pool.cs);
    // Cluster size cannot exceed 500
    LIMITED_WHILE(!available_coins.empty(), 500)
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
        pool.addUnchecked(entry.Fee(fee).FromTx(tx));

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

} // namespace
