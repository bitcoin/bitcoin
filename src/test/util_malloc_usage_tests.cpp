// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memusage.h>
#include <tinyformat.h>

#include <unordered_map>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(util_malloc_usage_tests)

// Allocate memory of various sizes and keep track of the most common address
// difference between consecutive allocations of the same size. This reliably
// indicates the amount of actual memory consumed by that allocation size.
BOOST_AUTO_TEST_CASE(malloc_usage)
{
    std::vector<void*> allocations;
    for (size_t s{1}; s < 120; ++s) {
        // histogram to track address difference frequencies
        std::vector<int> hist;
        hist.resize(200);

        uintptr_t prev{0};
        for (int i{1}; i < 1000; ++i) {
            void *a{malloc(s)};
            allocations.push_back(a);
            uintptr_t next{uintptr_t(a)};
            // Some memory allocators go from higher to lower addresses.
            uintptr_t d{(next > prev) ? next - prev : prev - next};
            prev = next;
            if (d < hist.size()) hist[d]++;
        }
        // Find the most common (most frequently occurring) address difference.
        int max_frequency{0};
        size_t most_common_diff{0};
        for (size_t c{1}; c < hist.size(); ++c) {
            if (max_frequency < hist[c]) {
                max_frequency = hist[c];
                most_common_diff = c;
            }
        }
        // We assume most_common_diff is the actual allocation amount.

        // this stuff is temporary (just to get info from CI)
        if (most_common_diff != memusage::MallocUsage(s)) {
            BOOST_TEST_MESSAGE(strprintf("%i %i %i", s, most_common_diff, memusage::MallocUsage(s)));
        }

        BOOST_CHECK_EQUAL(most_common_diff, memusage::MallocUsage(s));
    }
    for (void* a : allocations) free(a);
}

// Add nodes to an unordered map, and keep track of the address differences
// between consecutive nodes. The difference that is most common reliably
// indicates the amount of actual memory allocated to each node. Note that
// each node needs to include at least a pointer to the next element in the
// per-bucket list.
BOOST_AUTO_TEST_CASE(unordered_map)
{
    std::string display{"map: "};
    // Make the data size strange so some alignment rounding is likely needed.
    using map_data = std::unordered_map<size_t, std::array<uint8_t, 50>>;
    std::unordered_map<size_t, map_data> m;
    // Try to prevent bucket reallocations, which interfere with node allocations.
    m.reserve(20'000);

    // histogram to track address difference frequencies
    std::vector<int> hist;
    hist.resize(200);

    uintptr_t prev{0};
    for (size_t i{0}; i < 10'000; ++i) {
        m[i] = map_data{};
        auto x{&m[i]};
        uintptr_t next{uintptr_t(x)};
        // Some memory allocators go from higher to lower addresses.
        uintptr_t d{(next > prev) ? next - prev : prev - next};
        prev = next;
        if (d < hist.size()) hist[d]++;
    }
    // Find the most common (most frequently occurring) address difference.
    int max_frequency{0};
    size_t most_common_diff{0};
    for (size_t c{1}; c < hist.size(); ++c) {
        if (max_frequency < hist[c]) {
            max_frequency = hist[c];
            most_common_diff = c;
        }
    }
    // We assume most_common_diff is the actual allocation amount.
    using node_type = std::pair<size_t, map_data>;
    size_t node_malloc_usage{memusage::MallocUsage(sizeof(node_type) + sizeof(void*))};
    BOOST_REQUIRE_EQUAL(most_common_diff, node_malloc_usage);

    // this stuff is temporary (just to get info from CI)
    BOOST_TEST_MESSAGE(node_malloc_usage);
    BOOST_TEST_MESSAGE(m.bucket_count());
    BOOST_TEST_MESSAGE(memusage::DynamicUsage(m));
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_SUITE_END()
