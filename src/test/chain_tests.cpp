// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <test/util/setup_common.h>

#include <cmath>
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

BOOST_AUTO_TEST_CASE(skip_height_analysis)
{
    // This test case indirectly tests the `GetSkipHeight` function,
    // and asserts that its properties are suitable for performant
    // ancestor search.
    // It does so by imitating the `CBlockIndex::GetAncestor()`
    // function to look for an ancestor, and ensuring that an arbitrary
    // ancestor is found in a number of steps that is much lower
    // than simple linear traversal.
    // This could be tested by a performance test as well, but here the
    // number of iterations can be measured, that gives a more precice
    // metric than CPU time.

    // Build a chain
    const int chain_size{1'000'000};
    std::vector<std::unique_ptr<CBlockIndex>> block_index;
    block_index.reserve(chain_size);
    // Create genesis block
    block_index.push_back(std::make_unique<CBlockIndex>());
    for(auto i{1}; i < chain_size; ++i) {
        auto new_index{std::make_unique<CBlockIndex>()};
        new_index->pprev = block_index.back().get();
        BOOST_CHECK(new_index->pprev);
        new_index->nHeight = new_index->pprev->nHeight + 1;
        new_index->BuildSkip();
        block_index.push_back(std::move(new_index));
    }

    const auto min_distance{100};
    FastRandomContext ctx(/*fDeterministic =*/true);
    for (auto i{0}; i < 1'000; ++i) {
        // pick a height in the second half of the chain
        const int start_height{chain_size - 1 - ctx.randrange(chain_size * 9 / 10)};
        // pick a target height, earlier by at least min_distance
        const int delta{min_distance + ctx.randrange(start_height - min_distance)};
        const int target_height{start_height - delta};
        BOOST_REQUIRE(start_height > 0 && start_height < chain_size);
        BOOST_REQUIRE(target_height > 0 && target_height < chain_size && target_height < start_height);

        // Perform traversal from start to target, skipping, as implemented in `GetAncestor`
        const auto& start_p{block_index[start_height]};
        const CBlockIndex* walk_p{start_p.get()};
        auto walk_height{start_height};
        auto iteration_count{0};
        while (walk_height > target_height) {
            BOOST_REQUIRE(walk_p->pskip);
            BOOST_REQUIRE(walk_p->pprev);
            BOOST_REQUIRE(walk_p->pprev->pskip);
            const int skip_height{walk_p->pskip->nHeight};
            const int prev_skip_height{walk_p->pprev->pskip->nHeight};
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            if (skip_height == target_height ||
                (skip_height > target_height &&
                !(prev_skip_height < skip_height - 2 && prev_skip_height >= target_height)
            )) {
                BOOST_REQUIRE_LT(skip_height, walk_height);
                walk_p = walk_p->pskip;
                walk_height = skip_height;
            } else {
                walk_p = walk_p->pprev;
                --walk_height;
            }
            ++iteration_count;
        }

        // Asserts on the number of iterations:
        // Absolute value maximum (derived empirically)
        BOOST_CHECK_LE(iteration_count, 115);
        // Dynamic bound, function of distance. Formula derived empirically.
        const auto bound_log_distance{std::ceil(8*std::log(delta)/std::log(2) - 40)};
        BOOST_CHECK_LE(double(iteration_count), bound_log_distance);

        // Also check that `GetAncestor` gives the same result
        auto p_ancestor{start_p->GetAncestor(target_height)};
        BOOST_CHECK_EQUAL(p_ancestor, block_index[target_height].get());
    }
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
