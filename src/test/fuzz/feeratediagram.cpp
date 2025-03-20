// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

#include <vector>

#include <util/feefrac.h>
#include <policy/rbf.h>

#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <assert.h>

namespace {

/** Takes the pre-computed and topologically-valid chunks and generates a fee diagram which starts at FeeFrac of (0, 0) */
std::vector<FeeFrac> BuildDiagramFromChunks(const std::span<const FeeFrac> chunks)
{
    std::vector<FeeFrac> diagram;
    diagram.reserve(chunks.size() + 1);

    diagram.emplace_back(0, 0);
    for (auto& chunk : chunks) {
        diagram.emplace_back(diagram.back() + chunk);
    }
    return diagram;
}


/** Evaluate a diagram at a specific size, returning the fee as a fraction.
 *
 * Fees in diagram cannot exceed 2^32, as the returned evaluation could overflow
 * the FeeFrac::fee field in the result. */
FeeFrac EvaluateDiagram(int32_t size, std::span<const FeeFrac> diagram)
{
    assert(diagram.size() > 0);
    unsigned not_above = 0;
    unsigned not_below = diagram.size() - 1;
    // If outside the range of diagram, extend begin/end.
    if (size < diagram[not_above].size) return {diagram[not_above].fee, 1};
    if (size > diagram[not_below].size) return {diagram[not_below].fee, 1};
    // Perform bisection search to locate the diagram segment that size is in.
    while (not_below > not_above + 1) {
        unsigned mid = (not_below + not_above) / 2;
        if (diagram[mid].size <= size) not_above = mid;
        if (diagram[mid].size >= size) not_below = mid;
    }
    // If the size matches a transition point between segments, return its fee.
    if (not_below == not_above) return {diagram[not_below].fee, 1};
    // Otherwise, interpolate.
    auto dir_coef = diagram[not_below] - diagram[not_above];
    assert(dir_coef.size > 0);
    // Let A = diagram[not_above] and B = diagram[not_below]
    const auto& point_a = diagram[not_above];
    // We want to return:
    //     A.fee + (B.fee - A.fee) / (B.size - A.size) * (size - A.size)
    //   = A.fee + dir_coef.fee / dir_coef.size * (size - A.size)
    //   = (A.fee * dir_coef.size + dir_coef.fee * (size - A.size)) / dir_coef.size
    assert(size >= point_a.size);
    return {point_a.fee * dir_coef.size + dir_coef.fee * (size - point_a.size), dir_coef.size};
}

std::weak_ordering CompareFeeFracWithDiagram(const FeeFrac& ff, std::span<const FeeFrac> diagram)
{
    return FeeRateCompare(FeeFrac{ff.fee, 1}, EvaluateDiagram(ff.size, diagram));
}

std::partial_ordering CompareDiagrams(std::span<const FeeFrac> dia1, std::span<const FeeFrac> dia2)
{
    bool all_ge = true;
    bool all_le = true;
    for (const auto p1 : dia1) {
        auto cmp = CompareFeeFracWithDiagram(p1, dia2);
        if (std::is_lt(cmp)) all_ge = false;
        if (std::is_gt(cmp)) all_le = false;
    }
    for (const auto p2 : dia2) {
        auto cmp = CompareFeeFracWithDiagram(p2, dia1);
        if (std::is_lt(cmp)) all_le = false;
        if (std::is_gt(cmp)) all_ge = false;
    }
    if (all_ge && all_le) return std::partial_ordering::equivalent;
    if (all_ge && !all_le) return std::partial_ordering::greater;
    if (!all_ge && all_le) return std::partial_ordering::less;
    return std::partial_ordering::unordered;
}

void PopulateChunks(FuzzedDataProvider& fuzzed_data_provider, std::vector<FeeFrac>& chunks)
{
    chunks.clear();

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 50)
    {
        chunks.emplace_back(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(INT32_MIN>>1, INT32_MAX>>1), fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 1000000));
    }
    return;
}

} // namespace

FUZZ_TARGET(build_and_compare_feerate_diagram)
{
    // Generate a random set of chunks
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::vector<FeeFrac> chunks1, chunks2;
    FeeFrac empty{0, 0};

    PopulateChunks(fuzzed_data_provider, chunks1);
    PopulateChunks(fuzzed_data_provider, chunks2);

    std::vector<FeeFrac> diagram1{BuildDiagramFromChunks(chunks1)};
    std::vector<FeeFrac> diagram2{BuildDiagramFromChunks(chunks2)};

    assert(diagram1.front() == empty);
    assert(diagram2.front() == empty);

    auto real = CompareChunks(chunks1, chunks2);
    auto sim = CompareDiagrams(diagram1, diagram2);
    assert(real == sim);

    // Do explicit evaluation at up to 1000 points, and verify consistency with the result.
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 1000) {
        int32_t size = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, diagram2.back().size);
        auto eval1 = EvaluateDiagram(size, diagram1);
        auto eval2 = EvaluateDiagram(size, diagram2);
        auto cmp = FeeRateCompare(eval1, eval2);
        if (std::is_lt(cmp)) assert(!std::is_gt(real));
        if (std::is_gt(cmp)) assert(!std::is_lt(real));
    }
}
