// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <blockfilter.h>

static void ConstructGCSFilter(benchmark::State& state)
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }

    uint64_t siphash_k0 = 0;
    while (state.KeepRunning()) {
        GCSFilter filter({siphash_k0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);

        siphash_k0++;
    }
}

static void MatchGCSFilter(benchmark::State& state)
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }
    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);

    while (state.KeepRunning()) {
        filter.Match(GCSFilter::Element());
    }
}

static void DecodeGCSFilter(benchmark::State& state)
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }
    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    auto encoded = filter.GetEncoded();

    while (state.KeepRunning()) {
        GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, encoded);
    }
}

static void BlockFilterGetHash(benchmark::State& state)
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }
    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    BlockFilter block_filter(BlockFilterType::BASIC, {}, filter.GetEncoded());

    while (state.KeepRunning()) {
        block_filter.GetHash();
    }
}

static void DecodeCheckedGCSFilter(benchmark::State& state)
{
    GCSFilter::ElementSet elements;
    for (int i = 0; i < 10000; ++i) {
        GCSFilter::Element element(32);
        element[0] = static_cast<unsigned char>(i);
        element[1] = static_cast<unsigned char>(i >> 8);
        elements.insert(std::move(element));
    }
    GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    auto encoded = filter.GetEncoded();

    while (state.KeepRunning()) {
        GCSFilter filter({0, 0, BASIC_FILTER_P, BASIC_FILTER_M}, encoded, true);
    }
}
BENCHMARK(BlockFilterGetHash, 10000);
BENCHMARK(ConstructGCSFilter, 1000);
BENCHMARK(DecodeGCSFilter, 5000);
BENCHMARK(DecodeCheckedGCSFilter, 50000);
BENCHMARK(MatchGCSFilter, 50 * 1000);
