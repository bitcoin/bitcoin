// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <numeric>
#include <memory>

extern int GetSkipHeight(int height);

BOOST_FIXTURE_TEST_SUITE(chain_tests, BasicTestingSetup)

namespace {

const CBlockIndex* NaiveGetAncestor(const CBlockIndex* a, int height)
{
    while (a->nHeight > height) {
        a = a->pprev;
    }
    BOOST_REQUIRE_EQUAL(a->nHeight, height);
    return a;
}

const CBlockIndex* NaiveLastCommonAncestor(const CBlockIndex* a, const CBlockIndex* b)
{
    while (a->nHeight > b->nHeight) {
        a = a->pprev;
    }
    while (b->nHeight > a->nHeight) {
        b = b->pprev;
    }
    while (a != b) {
        BOOST_REQUIRE_EQUAL(a->nHeight, b->nHeight);
        a = a->pprev;
        b = b->pprev;
    }
    BOOST_REQUIRE_EQUAL(a, b);
    return a;
}

} // namespace

BOOST_AUTO_TEST_CASE(get_skip_height_test)
{
    // Even values: the rightmost bit is zeroed
    // Test with various even values with at least 2 bits set
    BOOST_CHECK_EQUAL(GetSkipHeight(0b00010010),
                                    0b00010000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b00100010),
                                    0b00100000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b01000010),
                                    0b01000000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b00010100),
                                    0b00010000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b00011000),
                                    0b00010000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10101010),
                                    0b10101000);
    // Odd values: the 2nd and 3rd bits are zeroed
    // Test with various odd values with at least 4 bits set
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10010011),
                                    0b10000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10100011),
                                    0b10000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b11000011),
                                    0b10000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10010101),
                                    0b10000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10011001),
                                    0b10000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b10101011),
                                    0b10100001);
    // Some longer random values (even and odd)
    BOOST_CHECK_EQUAL(GetSkipHeight(0b0001011101101000),
                                    0b0001011101100000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b0001011101101001),
                                    0b0001011101000001);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b0110101101011000),
                                    0b0110101101010000);
    BOOST_CHECK_EQUAL(GetSkipHeight(0b0110101101011001),
                                    0b0110101101000001);
    // All values 0-10
    BOOST_CHECK_EQUAL(GetSkipHeight(0), 0);
    BOOST_CHECK_EQUAL(GetSkipHeight(1), 0);
    BOOST_CHECK_EQUAL(GetSkipHeight(2), 0);
    BOOST_CHECK_EQUAL(GetSkipHeight(3), 1);
    BOOST_CHECK_EQUAL(GetSkipHeight(4), 0);
    BOOST_CHECK_EQUAL(GetSkipHeight(5), 1);
    BOOST_CHECK_EQUAL(GetSkipHeight(6), 4);
    BOOST_CHECK_EQUAL(GetSkipHeight(7), 1);
    BOOST_CHECK_EQUAL(GetSkipHeight(8), 0);
    BOOST_CHECK_EQUAL(GetSkipHeight(9), 1);
    BOOST_CHECK_EQUAL(GetSkipHeight(10), 8);
}

BOOST_AUTO_TEST_CASE(skip_height_properties_test)
{
    // The test collects and analyses the distribution of the
    // skip difference numbers, and checks some properties:
    // non-uniform distribution with most values small but
    // with some large values as well.

    const auto n{10'000};
    std::vector<std::unique_ptr<CBlockIndex>> block_index;
    block_index.reserve(n);
    // Create genesis block
    block_index.push_back(std::make_unique<CBlockIndex>());
    // Build a chain
    for(int i{1}; i < n; ++i) {
        auto new_index = std::make_unique<CBlockIndex>();
        new_index->pprev = block_index.back().get();
        BOOST_REQUIRE(new_index->pprev);
        new_index->nHeight = new_index->pprev->nHeight + 1;
        new_index->BuildSkip();
        block_index.push_back(std::move(new_index));
    }

    // Collect the diff (in height) of each element and its pskip
    std::vector<int> skip_diffs;
    skip_diffs.reserve(n);
    for(int i{1}; i < n; ++i) {
        BOOST_REQUIRE_EQUAL(block_index[i]->nHeight, i);
        auto skip_height = block_index[i]->pskip->nHeight;
        BOOST_CHECK(skip_height <= i && skip_height >= 0);
        skip_diffs.push_back(i - skip_height);
    }

    // Analyze the diffs
    // Compute the average, median, maximum
    const auto avg{double(std::accumulate(skip_diffs.begin(), skip_diffs.end(), 0)) / double(skip_diffs.size())};
    std::ranges::sort(skip_diffs);
    const auto median{skip_diffs[n / 2]};
    const auto max{skip_diffs[skip_diffs.size() - 1]};

    // The median skip diff is lower than the average (non-uniform distribution)
    BOOST_CHECK_LT(median, avg);
    // There are some large skip diffs (more than half of n):
    BOOST_CHECK_GT(max, n / 2);
    // The median skip diff is very low (an arbitrary low value is used)
    BOOST_CHECK_LT(median, 20);
}

BOOST_AUTO_TEST_CASE(chain_test)
{
    FastRandomContext ctx;
    std::vector<std::unique_ptr<CBlockIndex>> block_index;
    // Run 10 iterations of the whole test.
    for (int i = 0; i < 10; ++i) {
        block_index.clear();
        // Create genesis block.
        auto genesis = std::make_unique<CBlockIndex>();
        genesis->nHeight = 0;
        block_index.push_back(std::move(genesis));
        // Create 10000 more blocks.
        for (int b = 0; b < 10000; ++b) {
            auto new_index = std::make_unique<CBlockIndex>();
            // 95% of blocks build on top of the last block; the others fork off randomly.
            if (ctx.randrange(20) != 0) {
                new_index->pprev = block_index.back().get();
            } else {
                new_index->pprev = block_index[ctx.randrange(block_index.size())].get();
            }
            new_index->nHeight = new_index->pprev->nHeight + 1;
            new_index->BuildSkip();
            block_index.push_back(std::move(new_index));
        }
        // Run 10000 random GetAncestor queries.
        for (int q = 0; q < 10000; ++q) {
            const CBlockIndex* block = block_index[ctx.randrange(block_index.size())].get();
            unsigned height = ctx.randrange<unsigned>(block->nHeight + 1);
            const CBlockIndex* result = block->GetAncestor(height);
            BOOST_CHECK(result == NaiveGetAncestor(block, height));
        }
        // Run 10000 random LastCommonAncestor queries.
        for (int q = 0; q < 10000; ++q) {
            const CBlockIndex* block1 = block_index[ctx.randrange(block_index.size())].get();
            const CBlockIndex* block2 = block_index[ctx.randrange(block_index.size())].get();
            const CBlockIndex* result = LastCommonAncestor(block1, block2);
            BOOST_CHECK(result == NaiveLastCommonAncestor(block1, block2));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
