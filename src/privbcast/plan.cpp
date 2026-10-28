// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/plan.h>

#include <privbcast/params.h>
#include <random.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace privbcast {
namespace {

/** A uniform draw from range under timing, at millisecond resolution. */
std::chrono::milliseconds Draw(FastRandomContext& rng, const Timing& timing, const DurationRange& range)
{
    const std::chrono::milliseconds min{timing.Scale(range.min)};
    const std::chrono::milliseconds max{timing.Scale(range.max)};
    return min + rng.randrange<std::chrono::milliseconds>(max - min + 1ms);
}

} // namespace

Plan DrawPlan(FastRandomContext& rng, const Timing& timing, std::span<const std::string> dns_seeds)
{
    Plan plan;
    plan.timing = timing;

    static_assert(std::ranges::count(SLOT_LAYOUT, Stratum::Late) == 2);
    std::array<std::chrono::milliseconds, 2> late{};
    // Redrawn until LATE_SLOT_SEPARATION apart.
    do {
        for (auto& offset : late) offset = Draw(rng, timing, LATE_SLOT_RANGE);
    } while (std::chrono::abs(late[0] - late[1]) < timing.Scale(LATE_SLOT_SEPARATION));

    const std::chrono::milliseconds delivery_start{timing.Scale(DISCOVERY_WINDOW)};
    size_t next_late{0};
    for (size_t i{0}; i < SLOTS; ++i) {
        SlotSchedule& slot{plan.slots[i]};
        slot.index = static_cast<int>(i);
        slot.stratum = SLOT_LAYOUT[i];
        slot.scheduled[0] = delivery_start;
        switch (slot.stratum) {
        case Stratum::Prompt:
            break;
        case Stratum::Mid:
            slot.scheduled[0] += Draw(rng, timing, MID_SLOT_RANGE);
            break;
        case Stratum::Late:
            slot.scheduled[0] += late[next_late++];
            break;
        } // no default case, so the compiler can warn about missing cases
        for (size_t k{1}; k < OPPORTUNITIES_PER_SLOT; ++k) {
            slot.scheduled[k] = slot.scheduled[k - 1] + Draw(rng, timing, BACKUP_RANGE);
        }
        slot.scheduled_end = slot.scheduled.back() + timing.Scale(ATTEMPT_MAX);
    }

    for (const std::string& name : dns_seeds) {
        if (std::ranges::find(plan.seed_order, name) == plan.seed_order.end()) plan.seed_order.push_back(name);
    }
    std::shuffle(plan.seed_order.begin(), plan.seed_order.end(), rng);

    plan.answer_seed = rng.rand64();
    return plan;
}

} // namespace privbcast
