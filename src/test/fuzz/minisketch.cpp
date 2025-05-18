// Copyright (c) 2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <minisketch.h>
#include <node/minisketchwrapper.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>

#include <map>
#include <numeric>

namespace {

Minisketch MakeFuzzMinisketch32(size_t capacity, uint32_t impl)
{
    return Assert(Minisketch(32, impl, capacity));
}

} // namespace

FUZZ_TARGET(minisketch)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const auto capacity{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 200)};
    const uint32_t impl{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, Minisketch::MaxImplementation())};
    if (!Minisketch::ImplementationSupported(32, impl)) return;

    Minisketch sketch_a{MakeFuzzMinisketch32(capacity, impl)};
    Minisketch sketch_b{MakeFuzzMinisketch32(capacity, impl)};
    sketch_a.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    sketch_b.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());

    // Fill two sets and keep the difference in a map
    std::map<uint32_t, bool> diff;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const auto entry{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, std::numeric_limits<uint32_t>::max() - 1)};
        const auto KeepDiff{[&] {
            bool& mut{diff[entry]};
            mut = !mut;
        }};
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                sketch_a.Add(entry);
                KeepDiff();
            },
            [&] {
                sketch_b.Add(entry);
                KeepDiff();
            },
            [&] {
                sketch_a.Add(entry);
                sketch_b.Add(entry);
            });
    }
    const auto num_diff{std::accumulate(diff.begin(), diff.end(), size_t{0}, [](auto n, const auto& e) { return n + e.second; })};

    Minisketch sketch_ar{MakeFuzzMinisketch32(capacity, impl)};
    Minisketch sketch_br{MakeFuzzMinisketch32(capacity, impl)};
    sketch_ar.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    sketch_br.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());

    sketch_ar.Deserialize(sketch_a.Serialize());
    sketch_br.Deserialize(sketch_b.Serialize());

    Minisketch sketch_diff{std::move(fuzzed_data_provider.ConsumeBool() ? sketch_a : sketch_ar)};
    sketch_diff.Merge(fuzzed_data_provider.ConsumeBool() ? sketch_b : sketch_br);

    if (capacity >= num_diff) {
        const auto max_elements{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(num_diff, capacity)};
        const auto dec{*Assert(sketch_diff.Decode(max_elements))};
        Assert(dec.size() == num_diff);
        for (auto d : dec) {
            Assert(diff.at(d));
        }
    }
}
