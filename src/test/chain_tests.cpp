// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <test/util/setup_common.h>

#include <memory>

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

BOOST_AUTO_TEST_CASE(skip_height_properties_test)
{
    // The test collects and analyses the distribution of the
    // skip difference numbers, and checks some properties:
    // non-uniform distribution with most values small but
    // with some large values as well.

    FastRandomContext ctx;
    std::vector<std::unique_ptr<CBlockIndex>> block_index;

    // Create genesis block
    auto genesis = std::make_unique<CBlockIndex>();
    genesis->nHeight = 0;
    block_index.push_back(std::move(genesis));

    // Build a chain
    const auto n{30000};
    for(int i = 0; i < n; ++i) {
        auto new_index = std::make_unique<CBlockIndex>();
        new_index->pprev = block_index.back().get();
        BOOST_CHECK(new_index->pprev);
        new_index->nHeight = new_index->pprev->nHeight + 1;
        new_index->BuildSkip();
        block_index.push_back(std::move(new_index));
    }

    // Analyze the diff (in height) of each element and its pskip
    int count_diff_smaller_16 = 0;
    int max_diff = 0;
    int total_diff = 0;
    for(int i = 1; i < n; ++i) {
        BOOST_CHECK(block_index[i]->nHeight == i);
        auto skip_height = block_index[i]->pskip->nHeight;
        BOOST_CHECK(skip_height <= i && skip_height >= 0);
        auto diff = i - skip_height;

        if (diff < 16) {
            ++count_diff_smaller_16;
        }
        max_diff = diff > max_diff ? diff : max_diff;
        total_diff += diff;
    }

    // Most skip diffs are small (more than 67% are below 16):
    BOOST_CHECK(count_diff_smaller_16 > n * 2 / 3);
    // There are some large skip diffs (more than 75% of n):
    BOOST_CHECK(max_diff > n * 3 / 4);
    // The average skip diff is relatively high:
    BOOST_CHECK(double(total_diff) / double(n) > 50.0);
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
