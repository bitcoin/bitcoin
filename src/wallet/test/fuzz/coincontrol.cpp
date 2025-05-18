// Copyright (c) 2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coincontrol.h>
#include <wallet/test/util.h>

namespace wallet {
namespace {

const TestingSetup* g_setup;

void initialize_coincontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(coincontrol, .init = initialize_coincontrol)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    ArgsManager& args = *node.args;

    // for GetBoolArg to return true sometimes
    args.ForceSetArg("-avoidpartialspends", fuzzed_data_provider.ConsumeBool()?"1":"0");

    CCoinControl coin_control;
    COutPoint out_point;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::optional<COutPoint> optional_out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!optional_out_point) {
                    return;
                }
                out_point = *optional_out_point;
            },
            [&] {
                (void)coin_control.HasSelected();
            },
            [&] {
                (void)coin_control.IsSelected(out_point);
            },
            [&] {
                (void)coin_control.IsExternalSelected(out_point);
            },
            [&] {
                (void)coin_control.GetExternalOutput(out_point);
            },
            [&] {
                (void)coin_control.Select(out_point);
            },
            [&] {
                const CTxOut tx_out{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)};
                (void)coin_control.Select(out_point).SetTxOut(tx_out);
            },
            [&] {
                (void)coin_control.UnSelect(out_point);
            },
            [&] {
                (void)coin_control.UnSelectAll();
            },
            [&] {
                (void)coin_control.ListSelected();
            },
            [&] {
                int64_t weight{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                (void)coin_control.SetInputWeight(out_point, weight);
            },
            [&] {
                (void)coin_control.GetInputWeight(out_point);
            });
    }
}
} // namespace
} // namespace wallet
