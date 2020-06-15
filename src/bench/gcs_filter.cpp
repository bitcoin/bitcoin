// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <blockfilter.h>

static const GCSFilter::ElementSet GenerateGCSTestElements()
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
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
    bench.batch(elements.size()).unit("elem").run([&] {
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
BENCHMARK(GCSBlockFilterGetHash);
BENCHMARK(GCSFilterConstruct);
BENCHMARK(GCSFilterDecode);
BENCHMARK(GCSFilterDecodeSkipCheck);
BENCHMARK(GCSFilterMatch);
