// Copyright (c) 2022 The Bitcoin Core developers
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
                assert(coin_control.IsSelected(out_point));
            },
            [&] {
                const CTxOut tx_out{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)};
                auto& input = coin_control.Select(out_point);
                input.SetTxOut(tx_out);
                assert(input.HasTxOut());
                assert(input.GetTxOut() == tx_out);
                assert(coin_control.IsExternalSelected(out_point));
            },
            [&] {
                (void)coin_control.UnSelect(out_point);
                assert(!coin_control.IsSelected(out_point));
            },
            [&] {
                (void)coin_control.UnSelectAll();
                assert(!coin_control.HasSelected());
            },
            [&] {
                const std::vector<COutPoint> selected = coin_control.ListSelected();
                for (const auto& out : selected) {
                    assert(coin_control.IsSelected(out));
                }
            },
            [&] {
                int64_t weight{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                (void)coin_control.SetInputWeight(out_point, weight);
                assert(coin_control.GetInputWeight(out_point) == weight);
            },
            [&] {
                (void)coin_control.GetSequence(out_point);
            },
            [&] {
                (void)coin_control.GetScripts(out_point);
            },
            [&] {
                (void)coin_control.HasSelectedOrder();
            },
            [&] {
                (void)coin_control.GetSelectionPos(out_point);
            },
            [&] {
                auto& input = coin_control.Select(out_point);
                uint32_t sequence{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                input.SetSequence(sequence);
                assert(input.GetSequence() == sequence);
            },
            [&] {
                auto& input = coin_control.Select(out_point);
                const CScript script{ConsumeScript(fuzzed_data_provider)};
                input.SetScriptSig(script);
                assert(input.HasScripts());
                assert(input.GetScripts().first == script);
            },
            [&] {
                auto& input = coin_control.Select(out_point);
                const CScriptWitness script_wit{ConsumeScriptWitness(fuzzed_data_provider)};
                input.SetScriptWitness(script_wit);
                assert(input.HasScripts());
                assert(input.GetScripts().second->stack == script_wit.stack);
            },
            [&] {
                auto& input = coin_control.Select(out_point);
                unsigned int pos{fuzzed_data_provider.ConsumeIntegral<unsigned int>()};
                input.SetPosition(pos);
                assert(input.GetPosition() == pos);
            });
    }
}
} // namespace
} // namespace wallet
