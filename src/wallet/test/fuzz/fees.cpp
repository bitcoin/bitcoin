// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {

struct FeeEstimatorTestingSetup : public TestingSetup {
    FeeEstimatorTestingSetup(const ChainType chain_type, TestOpts opts) : TestingSetup{chain_type, opts}
    {
    }

    ~FeeEstimatorTestingSetup() {
        m_node.fee_estimator.reset();
    }

    void SetFeeEstimator(std::unique_ptr<CBlockPolicyEstimator> fee_estimator)
    {
        m_node.fee_estimator = std::move(fee_estimator);
    }
};

FeeEstimatorTestingSetup* g_setup;

class FuzzedBlockPolicyEstimator : public CBlockPolicyEstimator
{
    FuzzedDataProvider& fuzzed_data_provider;

public:
    FuzzedBlockPolicyEstimator(FuzzedDataProvider& provider)
        : CBlockPolicyEstimator(fs::path{}, false), fuzzed_data_provider(provider) {}

    CFeeRate estimateSmartFee(int confTarget, FeeCalculation* feeCalc, bool conservative) const override
    {
        return CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/1'000'000)};
    }

    unsigned int HighestTargetTracked(FeeEstimateHorizon horizon) const override
    {
        return fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1000);
    }
};

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<FeeEstimatorTestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_fees, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    auto& node{g_setup->m_node};
    Chainstate* chainstate = &node.chainman->ActiveChainstate();

    bilingual_str error;
    CTxMemPool::Options mempool_opts{
        .incremental_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)},
        .min_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)},
        .dust_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)}
    };
    node.mempool = std::make_unique<CTxMemPool>(mempool_opts, error);
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator = std::make_unique<FuzzedBlockPolicyEstimator>(fuzzed_data_provider);
    g_setup->SetFeeEstimator(std::move(fee_estimator));
    auto target_feerate{CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/1'000'000)}};
    if (target_feerate > node.mempool->m_opts.incremental_relay_feerate &&
        target_feerate > node.mempool->m_opts.min_relay_feerate) {
        MockMempoolMinFee(target_feerate, *node.mempool);
    }
    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetLastBlockProcessed(chainstate->m_chain.Height(), chainstate->m_chain.Tip()->GetBlockHash());
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_fallback_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_discard_rate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    (void)GetDiscardRate(wallet);

    const auto tx_bytes{fuzzed_data_provider.ConsumeIntegralInRange(0, std::numeric_limits<int32_t>::max())};
    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_pay_tx_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
        wallet.m_min_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }

    (void)GetRequiredFee(wallet, tx_bytes);
    (void)GetRequiredFeeRate(wallet);

    CCoinControl coin_control;
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_confirm_target = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 999'000);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_fee_mode = fuzzed_data_provider.ConsumeBool() ? FeeEstimateMode::CONSERVATIVE : FeeEstimateMode::ECONOMICAL;
    }

    FeeCalculation fee_calculation;
    FeeCalculation* maybe_fee_calculation{fuzzed_data_provider.ConsumeBool() ? nullptr : &fee_calculation};
    (void)GetMinimumFeeRate(wallet, coin_control, maybe_fee_calculation);
    (void)GetMinimumFee(wallet, tx_bytes, coin_control, maybe_fee_calculation);
}
} // namespace
} // namespace wallet
