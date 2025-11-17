// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator_man.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <util/fees.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {

struct FeeEstimatorManTestingSetup : public TestingSetup {
    FeeEstimatorManTestingSetup(const ChainType chain_type, TestOpts opts) : TestingSetup{chain_type, opts}
    {
    }

    ~FeeEstimatorManTestingSetup()
    {
        m_node.fee_estimator_man.reset();
    }

    void SetFeeEstimatorMan(std::unique_ptr<FeeRateEstimatorManager> fee_estimator_man)
    {
        m_node.fee_estimator_man = std::move(fee_estimator_man);
    }
};

FeeEstimatorManTestingSetup* g_setup;

class FuzzedFeeEstimatorMan : public FeeRateEstimatorManager
{
    FuzzedDataProvider& fuzzed_data_provider;

public:
    FuzzedFeeEstimatorMan(FuzzedDataProvider& provider)
        : fuzzed_data_provider(provider) {}

    FeeRateEstimatorResult GetFeeRateEstimate(int confTarget, bool conservative) override
    {
        FeeRateEstimatorResult res;
        res.feerate_estimator = FeeRateEstimatorType::BLOCK_POLICY;
        res.current_block_height = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(2, 1000);
        res.returned_target = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(2, 1004);
        res.feerate = FeePerVSize(ConsumeMoney(fuzzed_data_provider, /*max=*/1'000'000), fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1000, 1000000));
        return res;
    }
    unsigned int MaximumTarget() const override
    {
        return fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1004);
    }
};

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<FeeEstimatorManTestingSetup>();
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
    std::unique_ptr<FeeRateEstimatorManager> fee_estimator_man = std::make_unique<FuzzedFeeEstimatorMan>(fuzzed_data_provider);
    g_setup->SetFeeEstimatorMan(std::move(fee_estimator_man));
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

    FeeSource fee_source;
    int returned_target;
    int* maybe_returned_target{fuzzed_data_provider.ConsumeBool() ? nullptr : &returned_target};
    FeeSource* maybe_fee_source{fuzzed_data_provider.ConsumeBool() ? nullptr : &fee_source};
    (void)GetMinimumFeeRate(wallet, coin_control, maybe_fee_source, maybe_returned_target);
    (void)GetMinimumFee(wallet, tx_bytes, coin_control, maybe_fee_source, maybe_returned_target);
}
} // namespace
} // namespace wallet
