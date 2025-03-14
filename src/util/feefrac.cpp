// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/feefrac.h>
#include <algorithm>
#include <array>
#include <vector>

std::partial_ordering CompareChunks(std::span<const FeeFrac> chunks0, std::span<const FeeFrac> chunks1)
{
    /** Array to allow indexed access to input diagrams. */
    const std::array<std::span<const FeeFrac>, 2> chunk = {chunks0, chunks1};
    /** How many elements we have processed in each input. */
    size_t next_index[2] = {0, 0};
    /** Accumulated fee/sizes in diagrams, up to next_index[i] - 1. */
    FeeFrac accum[2];
    /** Whether the corresponding input is strictly better than the other at least in one place. */
    bool better_somewhere[2] = {false, false};
    /** Get the first unprocessed point in diagram number dia. */
    const auto next_point = [&](int dia) { return chunk[dia][next_index[dia]] + accum[dia]; };
    /** Get the last processed point in diagram number dia. */
    const auto prev_point = [&](int dia) { return accum[dia]; };
    /** Move to the next point in diagram number dia. */
    const auto advance = [&](int dia) { accum[dia] += chunk[dia][next_index[dia]++]; };

    do {
        bool done_0 = next_index[0] == chunk[0].size();
        bool done_1 = next_index[1] == chunk[1].size();
        if (done_0 && done_1) break;

        // Determine which diagram has the first unprocessed point. If a single side is finished, use the
        // other one. Only up to one can be done due to check above.
        const int unproc_side = (done_0 || done_1) ? done_0 : next_point(0).size > next_point(1).size;

        // Let `P` be the next point on diagram unproc_side, and `A` and `B` the previous and next points
        // on the other diagram. We want to know if P lies above or below the line AB. To determine this, we
        // compute the slopes of line AB and of line AP, and compare them. These slopes are fee per size,
        // and can thus be expressed as FeeFracs.
        const FeeFrac& point_p = next_point(unproc_side);
        const FeeFrac& point_a = prev_point(!unproc_side);

        const auto slope_ap = point_p - point_a;
        Assume(slope_ap.size > 0);
        std::weak_ordering cmp = std::weak_ordering::equivalent;
        if (done_0 || done_1) {
            // If a single side has no points left, act as if AB has slope tail_feerate(of 0).
            Assume(!(done_0 && done_1));
            cmp = FeeRateCompare(slope_ap, FeeFrac(0, 1));
        } else {
            // If both sides have points left, compute B, and the slope of AB explicitly.
            const FeeFrac& point_b = next_point(!unproc_side);
            const auto slope_ab = point_b - point_a;
            Assume(slope_ab.size >= slope_ap.size);
            cmp = FeeRateCompare(slope_ap, slope_ab);

            // If B and P have the same size, B can be marked as processed (in addition to P, see
            // below), as we've already performed a comparison at this size.
            if (point_b.size == point_p.size) advance(!unproc_side);
        }
        // If P lies above AB, unproc_side is better in P. If P lies below AB, then !unproc_side is
        // better in P.
        if (std::is_gt(cmp)) better_somewhere[unproc_side] = true;
        if (std::is_lt(cmp)) better_somewhere[!unproc_side] = true;
        advance(unproc_side);

        // If both diagrams are better somewhere, they are incomparable.
        if (better_somewhere[0] && better_somewhere[1]) return std::partial_ordering::unordered;
    } while(true);

    // Otherwise compare the better_somewhere values.
    return better_somewhere[0] <=> better_somewhere[1];
}
