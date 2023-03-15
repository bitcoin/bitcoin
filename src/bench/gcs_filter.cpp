// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <blockfilter.h>

static GCSFilter::ElementSet GenerateGCSTestElements()
{
    GCSFilter::ElementSet elements;

    // Testing the benchmarks with different number of elements show that a filter
    // with at least 100,000 elements results in benchmarks that have the same
    // ns/op. This makes it easy to reason about how long (in nanoseconds) a single
    // filter element takes to process.
    for (int i = 0; i < 100000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }

    return elements;
}

static void GCSBlockFilterGetHash(benchmark::Bench& bench)
{
    auto elements = GenerateGCSTestElements();

    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    BlockFilter block_filter(BlockFilterType::BASIC, {}, filter.GetEncoded(), /*skip_decode_check=*/false);

    bench.run([&] {
        block_filter.GetHash();
    });
}

static void GCSFilterConstruct(benchmark::Bench& bench)
{
    auto elements = GenerateGCSTestElements();

    uint64_t siphash_k0 = 0;
    bench.run([&]{
        GCSFilter filter({siphash_k0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);

        siphash_k0++;
    });
}

static void GCSFilterDecode(benchmark::Bench& bench)
{
    auto elements = GenerateGCSTestElements();

    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    auto encoded = filter.GetEncoded();

    bench.run([&] {
        GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, encoded, /*skip_decode_check=*/false);
    });
}

static void GCSFilterDecodeSkipCheck(benchmark::Bench& bench)
{
    auto elements = GenerateGCSTestElements();

    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    auto encoded = filter.GetEncoded();

    bench.run([&] {
        GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, encoded, /*skip_decode_check=*/true);
    });
}

static void GCSFilterMatch(benchmark::Bench& bench)
{
    auto elements = GenerateGCSTestElements();

    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);

    bench.run([&] {
        filter.Match(GCSFilter::Element());
    });
}
BENCHMARK(GCSBlockFilterGetHash, benchmark::PriorityLevel::HIGH);
BENCHMARK(GCSFilterConstruct, benchmark::PriorityLevel::HIGH);
BENCHMARK(GCSFilterDecode, benchmark::PriorityLevel::HIGH);
BENCHMARK(GCSFilterDecodeSkipCheck, benchmark::PriorityLevel::HIGH);
BENCHMARK(GCSFilterMatch, benchmark::PriorityLevel::HIGH);
