// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <privbcast/assign.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <random.h>
#include <tinyformat.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace privbcast;

namespace {

CService Endpoint(const std::string& addr)
{
    const std::optional<CService> endpoint{Lookup(addr, /*portDefault=*/8333, /*fAllowLookup=*/false)};
    BOOST_REQUIRE(endpoint);
    return *endpoint;
}

/** A public IPv4 endpoint with the default port, different for every (seed, n). */
CService Answer(int seed, int n)
{
    return Endpoint(strprintf("1.%d.%d.1", seed + 1, n + 1));
}

std::string Name(int seed) { return strprintf("seed%d.example", seed); }

/** A seed whose queries got these usable endpoints, through query sets 0, 1 and so on, or none. */
SeedAnswers Seed(const std::string& name, const std::vector<std::optional<CService>>& usable)
{
    SeedAnswers seed;
    seed.name = name;
    seed.queries = QUERIES_PER_SEED;
    for (size_t set{0}; set < usable.size(); ++set) {
        seed.usable.at(set) = usable[set];
        if (usable[set]) ++seed.answers;
    }
    return seed;
}

/** Seeds 0 to count - 1, each with endpoints of its own through query sets 0 to answers - 1. */
DiscoveryResult Seeds(int count, int answers)
{
    DiscoveryResult result;
    for (int seed{0}; seed < count; ++seed) {
        std::vector<std::optional<CService>> usable;
        for (int n{0}; n < answers; ++n) usable.emplace_back(Answer(seed, n));
        result.seeds.push_back(Seed(Name(seed), usable));
    }
    return result;
}

/** A plan with a real schedule and these seeds in this order. */
Plan MakePlan(std::vector<std::string> seed_order)
{
    FastRandomContext rng{uint256{1}};
    Plan plan{DrawPlan(rng, Timing{}, {})};
    plan.seed_order = std::move(seed_order);
    return plan;
}

std::vector<std::string> Names(int count)
{
    std::vector<std::string> names;
    for (int seed{0}; seed < count; ++seed) names.push_back(Name(seed));
    return names;
}

/** The opportunities, a slot at a time: D for a candidate, . for none. */
std::string Layout(const Assignment& assignment)
{
    std::string layout;
    for (size_t slot{0}; slot < SLOTS; ++slot) {
        if (slot > 0) layout += ' ';
        for (const std::optional<Candidate>& candidate : assignment.opportunities[slot]) {
            layout += candidate ? 'D' : '.';
        }
    }
    return layout;
}

/** The seed of each of a slot's candidates. */
std::vector<std::string> Provenances(const Assignment& assignment, size_t slot)
{
    std::vector<std::string> provenances;
    for (const std::optional<Candidate>& candidate : assignment.opportunities[slot]) {
        if (candidate) provenances.push_back(candidate->provenance);
    }
    return provenances;
}

/** The first attempts, in slot order. */
std::vector<Candidate> FirstAttempts(const Assignment& assignment)
{
    std::vector<Candidate> firsts;
    for (size_t slot{0}; slot < SLOTS; ++slot) {
        BOOST_REQUIRE(assignment.opportunities[slot][0]);
        firsts.push_back(*assignment.opportunities[slot][0]);
    }
    return firsts;
}

/** The seeds the first attempts come from. */
std::set<std::string> FirstSeeds(const Assignment& assignment)
{
    std::set<std::string> seeds;
    for (const Candidate& first : FirstAttempts(assignment)) seeds.insert(first.provenance);
    return seeds;
}

} // namespace

BOOST_AUTO_TEST_SUITE(privbcast_assign_tests)

BOOST_AUTO_TEST_CASE(order_at_random)
{
    // R5b: every accepted endpoint is a candidate, and a seed hands its candidates out in an order
    // drawn at random: each comes first under some answer_seed.
    const DiscoveryResult four{Seeds(1, QUERIES_PER_SEED)};
    Plan plan{MakePlan(Names(1))};
    std::set<CService> first;
    for (uint64_t answer_seed{0}; answer_seed < 100; ++answer_seed) {
        plan.answer_seed = answer_seed;
        const Assignment assignment{Assign(plan, four)};
        BOOST_CHECK_EQUAL(assignment.seeds[0].accepted, QUERIES_PER_SEED);
        BOOST_REQUIRE_EQUAL(assignment.seeds[0].candidates.size(), size_t{QUERIES_PER_SEED});
        first.insert(assignment.seeds[0].candidates.front());
    }
    BOOST_CHECK_EQUAL(first.size(), size_t{QUERIES_PER_SEED});
}

BOOST_AUTO_TEST_CASE(spread_over_seeds)
{
    // R5b: a slot draws from a seed again only once no seed it may draw from has a candidate,
    // rather than leave an opportunity empty. Seeds have 4, 3, 3 and 3 candidates. With four seeds,
    // slots 4 and 5 take their first attempts from seeds that supplied one already, by query set
    // (R5c). After two layers only the first seed has a candidate left, and slot 0, which has drawn
    // from it already, takes it.
    DiscoveryResult result{Seeds(4, 3)};
    result.seeds[0] = Seed(Name(0), {Answer(0, 0), Answer(0, 1), Answer(0, 2), Answer(0, 3)});
    const Plan plan{MakePlan(Names(4))};
    const Assignment assignment{Assign(plan, result)};
    BOOST_CHECK_EQUAL(Layout(assignment), "DDD. DD.. DD.. DD.. DD.. DD..");
    BOOST_CHECK(Provenances(assignment, 0) == (std::vector{Name(0), Name(3), Name(0)}));
    BOOST_CHECK(Provenances(assignment, 1) == (std::vector{Name(1), Name(0)}));
    BOOST_CHECK(Provenances(assignment, 4) == (std::vector{Name(1), Name(3)}));
    BOOST_CHECK(Provenances(assignment, 5) == (std::vector{Name(2), Name(0)}));
    BOOST_CHECK_EQUAL(assignment.opportunities[4][0]->query_set, 0U);
    BOOST_CHECK_EQUAL(assignment.opportunities[5][0]->query_set, 1U);
}

BOOST_AUTO_TEST_CASE(first_attempts_by_query_set)
{
    // R5c: with every query set answered by every seed, slot j's first attempt comes from query set
    // j mod QUERIES_PER_SEED, so no set supplies more than two of them, and still from six seeds
    // (R5b).
    const Assignment assignment{Assign(MakePlan(Names(7)), Seeds(7, QUERIES_PER_SEED))};
    const std::vector<Candidate> firsts{FirstAttempts(assignment)};
    for (size_t slot{0}; slot < SLOTS; ++slot) BOOST_CHECK_EQUAL(firsts[slot].query_set, slot % QUERIES_PER_SEED);
    BOOST_CHECK_EQUAL(FirstSeeds(assignment).size(), SLOTS);
    BOOST_CHECK_EQUAL(Layout(assignment), "DDDD DDDD DDDD DDDD DDDD DDDD");
}

BOOST_AUTO_TEST_CASE(query_set_without_answers)
{
    // R5c: a query set that got no answer, as when its circuit failed, leaves no first attempt
    // empty: the slots that would take theirs from it take one from another set.
    DiscoveryResult result;
    for (int seed{0}; seed < 7; ++seed) result.seeds.push_back(Seed(Name(seed), {Answer(seed, 0), std::nullopt, Answer(seed, 2), Answer(seed, 3)}));
    const Assignment assignment{Assign(MakePlan(Names(7)), result)};
    const std::vector<Candidate> firsts{FirstAttempts(assignment)};
    for (size_t slot{0}; slot < SLOTS; ++slot) {
        if (slot % QUERIES_PER_SEED == 1) {
            BOOST_CHECK(firsts[slot].query_set != 1);
        } else {
            BOOST_CHECK_EQUAL(firsts[slot].query_set, slot % QUERIES_PER_SEED);
        }
    }
    BOOST_CHECK_EQUAL(FirstSeeds(assignment).size(), SLOTS);
    BOOST_CHECK_EQUAL(Layout(assignment), "DDDD DDDD DDDD DDD. DDD. DDD.");
}

BOOST_AUTO_TEST_CASE(seeds_before_query_sets)
{
    // R5b before R5c: when only seeds that have supplied a first attempt have a candidate from a
    // slot's query set, the slot takes another set's from another seed. Seed 0, first in the plan's
    // order, is the only one answered through query set 1.
    DiscoveryResult result{Seeds(7, 1)};
    result.seeds[0] = Seed(Name(0), {Answer(0, 0), Answer(0, 1)});
    const std::vector<Candidate> firsts{FirstAttempts(Assign(MakePlan(Names(7)), result))};
    BOOST_CHECK_EQUAL(firsts[0].provenance, Name(0));
    BOOST_CHECK_EQUAL(firsts[0].query_set, 0U);
    BOOST_CHECK(firsts[1].provenance != Name(0));
    BOOST_CHECK_EQUAL(firsts[1].query_set, 0U);
}

BOOST_AUTO_TEST_CASE(repeat_across_query_sets)
{
    // R4, R5c: an endpoint a seed returned through two query sets is one candidate, of the lower set.
    DiscoveryResult result;
    result.seeds.push_back(Seed(Name(0), {std::nullopt, Answer(0, 0), Answer(0, 1), Answer(0, 0)}));
    const Assignment assignment{Assign(MakePlan(Names(1)), result)};
    BOOST_CHECK_EQUAL(assignment.seeds[0].accepted, 2);
    BOOST_CHECK_EQUAL(assignment.duplicates, 1);
    size_t found{0};
    for (const auto& slot : assignment.opportunities) {
        for (const std::optional<Candidate>& candidate : slot) {
            if (candidate && candidate->endpoint == Answer(0, 0)) {
                BOOST_CHECK_EQUAL(candidate->query_set, 1U);
                ++found;
            }
        }
    }
    BOOST_CHECK_EQUAL(found, 1U);
}

BOOST_AUTO_TEST_SUITE_END()
