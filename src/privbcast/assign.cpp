// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/assign.h>

#include <hash.h>
#include <netaddress.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <random.h>
#include <util/check.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace privbcast {
namespace {

/** Assign()'s hand-out state, by position in the plan's order of the seeds. */
struct HandOut {
    /** Each seed's candidates not handed out yet, in the order it hands them out. */
    std::vector<std::vector<Candidate>> left;
    std::array<std::vector<bool>, SLOTS> drawn_from;
    std::vector<bool> supplied_first;
    size_t next_seed{0};
};

/** The candidate for one opportunity of a slot, or none. */
std::optional<Candidate> Draw(HandOut& state, size_t slot, size_t layer)
{
    const size_t num_seeds{state.left.size()};
    // First a seed the slot may still draw from, then any with a candidate left (R5b).
    const std::vector<bool>& used{layer == 0 ? state.supplied_first : state.drawn_from[slot]};
    // A first attempt prefers its query set (R5c).
    const size_t wanted{slot % QUERIES_PER_SEED};
    for (const bool again : {false, true}) {
        for (const bool any_set : {false, true}) {
            if (!any_set && layer > 0) continue;
            for (size_t step{0}; step < num_seeds; ++step) {
                const size_t pos{(state.next_seed + step) % num_seeds};
                if (used[pos] && !again) continue;
                std::vector<Candidate>& candidates{state.left[pos]};
                const auto it{any_set ? candidates.begin() : std::ranges::find(candidates, wanted, &Candidate::query_set)};
                if (it == candidates.end()) continue;
                state.drawn_from[slot][pos] = true;
                if (layer == 0) state.supplied_first[pos] = true;
                state.next_seed = (pos + 1) % num_seeds;
                Candidate candidate{std::move(*it)};
                candidates.erase(it);
                return candidate;
            }
        }
    }
    return std::nullopt;
}

} // namespace

Assignment Assign(const Plan& plan, const DiscoveryResult& result)
{
    // Discovery lists the seeds in the plan's order.
    Assume(std::ranges::equal(result.seeds, plan.seed_order, {}, &SeedAnswers::name));
    const size_t num_seeds{result.seeds.size()};
    Assignment assignment;
    assignment.seeds.resize(num_seeds);

    // Each seed's candidates, in the order it hands them out.
    HandOut state;
    state.left.resize(num_seeds);
    std::vector<std::vector<Candidate>>& left{state.left};
    std::set<CService> accepted;
    for (size_t pos{0}; pos < num_seeds; ++pos) {
        const SeedAnswers& answers{result.seeds[pos]};
        SeedSummary& seed{assignment.seeds[pos]};
        seed.name = answers.name;
        seed.queries = answers.queries;
        seed.skipped = answers.skipped;
        seed.answers = answers.answers;
        assignment.rejected += answers.rejected;

        // By query set, so that the order the answers arrived in plays no part (R5b), and an
        // endpoint that came through several sets is credited to the lowest of them (R5c).
        std::vector<Candidate>& credited{left[pos]};
        for (size_t set{0}; set < answers.usable.size(); ++set) {
            const std::optional<CService>& endpoint{answers.usable[set]};
            if (!endpoint) continue;
            if (accepted.insert(*endpoint).second) {
                credited.push_back({*endpoint, seed.name, set});
            } else {
                ++assignment.duplicates;
            }
        }
        // A generator per seed, so that how many answers the other seeds had plays no part in this
        // seed's order.
        FastRandomContext rng{(HashWriter{} << plan.answer_seed << uint64_t{pos}).GetSHA256()};
        std::shuffle(credited.begin(), credited.end(), rng);
        seed.accepted = static_cast<int>(credited.size());
        for (const Candidate& candidate : credited) seed.candidates.push_back(candidate.endpoint);
        assignment.exit_path_candidates += seed.accepted;
    }

    state.drawn_from.fill(std::vector<bool>(num_seeds, false));
    state.supplied_first.assign(num_seeds, false);

    for (size_t layer{0}; layer < OPPORTUNITIES_PER_SLOT; ++layer) {
        for (size_t slot{0}; slot < SLOTS; ++slot) {
            assignment.opportunities[slot][layer] = Draw(state, slot, layer);
        }
    }
    return assignment;
}

} // namespace privbcast
