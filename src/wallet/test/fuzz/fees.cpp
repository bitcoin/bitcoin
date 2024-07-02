// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>
#include <validation.h>

namespace wallet {
namespace {
const TestingSetup* g_setup;
static std::unique_ptr<CWallet> g_wallet_ptr;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    const auto& node{g_setup->m_node};
    g_wallet_ptr = std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase());
}

FUZZ_TARGET(wallet_fees, .init = initialize_setup)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto& node{g_setup->m_node};
    Chainstate* chainstate = &node.chainman->ActiveChainstate();
    CWallet& wallet = *g_wallet_ptr;
    {
        LOCK(wallet.cs_wallet);
        wallet.SetBestBlock(chainstate->m_chain.Height(), chainstate->m_chain.Tip()->GetBlockHash());
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_fallback_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_discard_rate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    (void)GetDiscardRate(wallet);

    const auto tx_bytes{fuzzed_data_provider.ConsumeIntegral<unsigned int>()};

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
