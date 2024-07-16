// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/spend.h>

namespace wallet {
namespace {

const TestingSetup* g_setup;
void initialize_coinsresult()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(coinsresult, .init = initialize_coinsresult)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CoinsResult coins_result;
    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    bool good_data{true};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool() && good_data, 100) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::optional<COutPoint> optional_outpoint{ConsumeDeserializable<COutPoint>(fuzzed_data_provider)};
                if (!optional_outpoint) {
                    good_data = false;
                    return;
                }
                COutput output = COutput{/*outpoint=*/*optional_outpoint,
                                          /*txout=*/CTxOut{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)},
                                          /*depth=*/fuzzed_data_provider.ConsumeIntegral<int>(),
                                          /*input_bytes=*/fuzzed_data_provider.ConsumeIntegralInRange<int>(-1, 1'000'000),
                                          /*spendable=*/fuzzed_data_provider.ConsumeBool(),
                                          /*solvable=*/fuzzed_data_provider.ConsumeBool(),
                                          /*safe=*/fuzzed_data_provider.ConsumeBool(),
                                          /*time=*/fuzzed_data_provider.ConsumeIntegral<int64_t>(),
                                          /*from_me=*/fuzzed_data_provider.ConsumeBool()};
                OutputType output_type{fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)};
                coins_result.Add(output_type, output);
            },
            [&] {
                std::unordered_set<COutPoint, SaltedOutpointHasher> outs_to_remove;
                std::vector<COutput> coins{coins_result.All()};
                for (const COutput& coin : coins) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        outs_to_remove.emplace(coin.outpoint);
                    }
                }
                coins_result.Erase(outs_to_remove);
            },
            [&] {
                (void)coins_result.All();
            },
            [&] {
                coins_result.Clear();
                const auto coins{coins_result.All()};
                const auto size{coins_result.Size()};
                assert(coins.empty() && size == 0);
            },
            [&] {
                (void)coins_result.GetTotalAmount();
            },
            [&] {
                (void)coins_result.GetEffectiveTotalAmount();
            },
            [&] {
                coins_result.Shuffle(fast_random_context);
            },
            [&] {
                (void)coins_result.Size();
            },
            [&] {
                (void)coins_result.TypesCount();
            }
        );
    }
}
} // namespace
} // namespace wallet
