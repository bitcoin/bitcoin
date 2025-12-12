// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <test/util/setup_common.h>

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

BOOST_AUTO_TEST_CASE(skip_height_analysis)
{
    // This test case indirectly tests the `GetSkipHeight` function,
    // and asserts that its properties are suitable for performant
    // ancestor search.
    // It does so by imitating the `GetAncestor` function to
    // look for an ancestor, and ensuring that an arbitrary
    // ancestor is found in a number of steps that is much lower
    // than simple linear traversal.

    // Build a chain
    const auto chain_size{200'000};
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

    const auto runs{1000};
    const auto min_distance{500};
    FastRandomContext ctx(/*fDeterministic =*/true);
    for (auto i{0}; i < runs; ++i) {
        // pick a height in the second half of the chain
        auto start_height = chain_size / 2 + ctx.randrange(chain_size / 2);
        BOOST_REQUIRE(start_height > 0 && start_height < chain_size);
        auto& start_p = block_index[start_height];
        // pick a target height, earlier by at least min_distance
        auto delta{min_distance + ctx.randrange(start_height - min_distance)};
        auto target_height{start_height - delta};
        BOOST_REQUIRE(target_height > 0 && target_height < chain_size && target_height < start_height);

        // Perform traversal from start to target, skipping, as implemented in `GetAncestor`
        const CBlockIndex* pindexWalk = start_p.get();
        auto heightWalk = start_height;
        auto iteration_count{0};
        while (heightWalk > target_height) {
            int heightSkip = GetSkipHeight(heightWalk);
            int heightSkipPrev = GetSkipHeight(heightWalk - 1);
            if (pindexWalk->pskip != nullptr &&
                (heightSkip == target_height ||
                (heightSkip > target_height && !(heightSkipPrev < heightSkip - 2 &&
                                        heightSkipPrev >= target_height)))) {
                // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
                pindexWalk = pindexWalk->pskip;
                heightWalk = heightSkip;
            } else {
                assert(pindexWalk->pprev);
                pindexWalk = pindexWalk->pprev;
                heightWalk--;
            }
            ++iteration_count;
        }

        // Asserts on the number of iterations:
        // always less than 100 and less than 5% of the distance
        BOOST_CHECK_LT(iteration_count, 100);
        BOOST_REQUIRE(delta > 0);
        auto rel_iter_count = double(iteration_count) / double(delta);
        BOOST_CHECK_LT(rel_iter_count, 0.05);

        // Since we are at it, we can also check that `GetAncestor` works
        // and gives the same result
        auto p_ancestor = start_p->GetAncestor(target_height);
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
