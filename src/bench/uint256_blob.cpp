// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <bench/bench.h>
#include <random.h>
#include <uint256.h>

#include <compare>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

enum class Difference {
    NONE,
    FIRST_BYTE,
    LAST_BYTE,
};

constexpr size_t NUM_PAIRS{4'096};

std::vector<std::pair<uint256, uint256>> MakePairs(Difference difference)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    std::vector<std::pair<uint256, uint256>> pairs;
    pairs.reserve(NUM_PAIRS);

    for (size_t i{0}; i < NUM_PAIRS; ++i) {
        uint256 lhs{rng.rand256()};
        uint256 rhs{lhs};
        if (difference != Difference::NONE) {
            const size_t position{difference == Difference::FIRST_BYTE ? 0 : uint256::size() - 1};
            lhs.begin()[position] = i % 2 == 0 ? 0 : 255;
            rhs.begin()[position] = i % 2 == 0 ? 255 : 0;
        }
        pairs.emplace_back(lhs, rhs);
    }
    return pairs;
}

template <typename Comparator>
void Comparison(benchmark::Bench& bench, Difference difference, Comparator comparator)
{
    const auto pairs{MakePairs(difference)};
    bench.batch(pairs.size()).unit("comparison").run([&] {
        for (const auto& [lhs, rhs] : pairs) {
            ankerl::nanobench::doNotOptimizeAway(comparator(lhs, rhs));
        }
    });
}

void Uint256EqualIdentical(benchmark::Bench& bench)
{
    Comparison(bench, Difference::NONE, [](const uint256& lhs, const uint256& rhs) { return lhs == rhs; });
}

void Uint256EqualFirstByteDifferent(benchmark::Bench& bench)
{
    Comparison(bench, Difference::FIRST_BYTE, [](const uint256& lhs, const uint256& rhs) { return lhs == rhs; });
}

void Uint256EqualLastByteDifferent(benchmark::Bench& bench)
{
    Comparison(bench, Difference::LAST_BYTE, [](const uint256& lhs, const uint256& rhs) { return lhs == rhs; });
}

void Uint256LessIdentical(benchmark::Bench& bench)
{
    Comparison(bench, Difference::NONE, [](const uint256& lhs, const uint256& rhs) { return lhs < rhs; });
}

void Uint256LessFirstByteDifferent(benchmark::Bench& bench)
{
    Comparison(bench, Difference::FIRST_BYTE, [](const uint256& lhs, const uint256& rhs) { return lhs < rhs; });
}

void Uint256LessLastByteDifferent(benchmark::Bench& bench)
{
    Comparison(bench, Difference::LAST_BYTE, [](const uint256& lhs, const uint256& rhs) { return lhs < rhs; });
}

} // namespace

BENCHMARK(Uint256EqualIdentical);
BENCHMARK(Uint256EqualFirstByteDifferent);
BENCHMARK(Uint256EqualLastByteDifferent);
BENCHMARK(Uint256LessIdentical);
BENCHMARK(Uint256LessFirstByteDifferent);
BENCHMARK(Uint256LessLastByteDifferent);
