// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <bit>
#include <cmath>
#include <vector>

#include <boost/test/unit_test.hpp>

#define SKIPLIST_LENGTH 300000

BOOST_FIXTURE_TEST_SUITE(skiplist_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(skiplist_test)
{
    std::vector<CBlockIndex> vIndex(SKIPLIST_LENGTH);

    for (int i=0; i<SKIPLIST_LENGTH; i++) {
        vIndex[i].nHeight = i;
        vIndex[i].pprev = (i == 0) ? nullptr : &vIndex[i - 1];
        vIndex[i].BuildSkip();
    }

    for (int i=0; i<SKIPLIST_LENGTH; i++) {
        if (i > 0) {
            BOOST_CHECK(vIndex[i].pskip == &vIndex[vIndex[i].pskip->nHeight]);
            BOOST_CHECK(vIndex[i].pskip->nHeight < i);
        } else {
            BOOST_CHECK(vIndex[i].pskip == nullptr);
        }
    }

    for (int i=0; i < 1000; i++) {
        int from = m_rng.randrange(SKIPLIST_LENGTH - 1);
        int to = m_rng.randrange(from + 1);

        BOOST_CHECK(vIndex[SKIPLIST_LENGTH - 1].GetAncestor(from) == &vIndex[from]);
        BOOST_CHECK(vIndex[from].GetAncestor(to) == &vIndex[to]);
        BOOST_CHECK(vIndex[from].GetAncestor(0) == vIndex.data());
    }
}

BOOST_AUTO_TEST_CASE(getlocator_test)
{
    // Build a main chain 100000 blocks long.
    std::vector<uint256> vHashMain(100000);
    std::vector<CBlockIndex> vBlocksMain(100000);
    for (unsigned int i=0; i<vBlocksMain.size(); i++) {
        vHashMain[i] = ArithToUint256(i); // Set the hash equal to the height, so we can quickly check the distances.
        vBlocksMain[i].nHeight = i;
        vBlocksMain[i].pprev = i ? &vBlocksMain[i - 1] : nullptr;
        vBlocksMain[i].phashBlock = &vHashMain[i];
        vBlocksMain[i].BuildSkip();
        BOOST_CHECK_EQUAL((int)UintToArith256(vBlocksMain[i].GetBlockHash()).GetLow64(), vBlocksMain[i].nHeight);
        BOOST_CHECK(vBlocksMain[i].pprev == nullptr || vBlocksMain[i].nHeight == vBlocksMain[i].pprev->nHeight + 1);
    }

    // Build a branch that splits off at block 49999, 50000 blocks long.
    std::vector<uint256> vHashSide(50000);
    std::vector<CBlockIndex> vBlocksSide(50000);
    for (unsigned int i=0; i<vBlocksSide.size(); i++) {
        vHashSide[i] = ArithToUint256(i + 50000 + (arith_uint256(1) << 128)); // Add 1<<128 to the hashes, so GetLow64() still returns the height.
        vBlocksSide[i].nHeight = i + 50000;
        vBlocksSide[i].pprev = i ? &vBlocksSide[i - 1] : (vBlocksMain.data()+49999);
        vBlocksSide[i].phashBlock = &vHashSide[i];
        vBlocksSide[i].BuildSkip();
        BOOST_CHECK_EQUAL((int)UintToArith256(vBlocksSide[i].GetBlockHash()).GetLow64(), vBlocksSide[i].nHeight);
        BOOST_CHECK(vBlocksSide[i].pprev == nullptr || vBlocksSide[i].nHeight == vBlocksSide[i].pprev->nHeight + 1);
    }

    // Build a CChain for the main branch.
    CChain chain;
    chain.SetTip(vBlocksMain.back());

    // Test 100 random starting points for locators.
    for (int n=0; n<100; n++) {
        int r = m_rng.randrange(150000);
        CBlockIndex* tip = (r < 100000) ? &vBlocksMain[r] : &vBlocksSide[r - 100000];
        CBlockLocator locator = GetLocator(tip);

        // The first result must be the block itself, the last one must be genesis.
        BOOST_CHECK(locator.vHave.front() == tip->GetBlockHash());
        BOOST_CHECK(locator.vHave.back() == vBlocksMain[0].GetBlockHash());

        // Entries 1 through 11 (inclusive) go back one step each.
        for (unsigned int i = 1; i < 12 && i < locator.vHave.size() - 1; i++) {
            BOOST_CHECK_EQUAL(UintToArith256(locator.vHave[i]).GetLow64(), tip->nHeight - i);
        }

        // The further ones (excluding the last one) go back with exponential steps.
        unsigned int dist = 2;
        for (unsigned int i = 12; i < locator.vHave.size() - 1; i++) {
            BOOST_CHECK_EQUAL(UintToArith256(locator.vHave[i - 1]).GetLow64() - UintToArith256(locator.vHave[i]).GetLow64(), dist);
            dist *= 2;
        }
    }
}

BOOST_AUTO_TEST_CASE(findearliestatleast_test)
{
    std::vector<uint256> vHashMain(100000);
    std::vector<CBlockIndex> vBlocksMain(100000);
    for (unsigned int i=0; i<vBlocksMain.size(); i++) {
        vHashMain[i] = ArithToUint256(i); // Set the hash equal to the height
        vBlocksMain[i].nHeight = i;
        vBlocksMain[i].pprev = i ? &vBlocksMain[i - 1] : nullptr;
        vBlocksMain[i].phashBlock = &vHashMain[i];
        vBlocksMain[i].BuildSkip();
        if (i < 10) {
            vBlocksMain[i].nTime = i;
            vBlocksMain[i].nTimeMax = i;
        } else {
            // randomly choose something in the range [MTP, MTP*2]
            int64_t medianTimePast = vBlocksMain[i].GetMedianTimePast();
            int r{int(m_rng.randrange(medianTimePast))};
            vBlocksMain[i].nTime = uint32_t(r + medianTimePast);
            vBlocksMain[i].nTimeMax = std::max(vBlocksMain[i].nTime, vBlocksMain[i-1].nTimeMax);
        }
    }
    // Check that we set nTimeMax up correctly.
    unsigned int curTimeMax = 0;
    for (unsigned int i=0; i<vBlocksMain.size(); ++i) {
        curTimeMax = std::max(curTimeMax, vBlocksMain[i].nTime);
        BOOST_CHECK(curTimeMax == vBlocksMain[i].nTimeMax);
    }

    // Build a CChain for the main branch.
    CChain chain;
    chain.SetTip(vBlocksMain.back());

    // Verify that FindEarliestAtLeast is correct.
    for (unsigned int i=0; i<10000; ++i) {
        // Pick a random element in vBlocksMain.
        int r = m_rng.randrange(vBlocksMain.size());
        int64_t test_time = vBlocksMain[r].nTime;
        CBlockIndex* ret = chain.FindEarliestAtLeast(test_time, 0);
        BOOST_CHECK(ret->nTimeMax >= test_time);
        BOOST_CHECK((ret->pprev==nullptr) || ret->pprev->nTimeMax < test_time);
        BOOST_CHECK(vBlocksMain[r].GetAncestor(ret->nHeight) == ret);
    }
}

BOOST_AUTO_TEST_CASE(findearliestatleast_edge_test)
{
    std::list<CBlockIndex> blocks;
    for (const unsigned int timeMax : {100, 100, 100, 200, 200, 200, 300, 300, 300}) {
        CBlockIndex* prev = blocks.empty() ? nullptr : &blocks.back();
        blocks.emplace_back();
        blocks.back().nHeight = prev ? prev->nHeight + 1 : 0;
        blocks.back().pprev = prev;
        blocks.back().BuildSkip();
        blocks.back().nTimeMax = timeMax;
    }

    CChain chain;
    chain.SetTip(blocks.back());

    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(50, 0)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(100, 0)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(150, 0)->nHeight, 3);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(200, 0)->nHeight, 3);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(250, 0)->nHeight, 6);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(300, 0)->nHeight, 6);
    BOOST_CHECK(!chain.FindEarliestAtLeast(350, 0));

    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(0, 0)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(-1, 0)->nHeight, 0);

    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(std::numeric_limits<int64_t>::min(), 0)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(-int64_t(std::numeric_limits<unsigned int>::max()) - 1, 0)->nHeight, 0);
    BOOST_CHECK(!chain.FindEarliestAtLeast(std::numeric_limits<int64_t>::max(), 0));
    BOOST_CHECK(!chain.FindEarliestAtLeast(std::numeric_limits<unsigned int>::max(), 0));
    BOOST_CHECK(!chain.FindEarliestAtLeast(int64_t(std::numeric_limits<unsigned int>::max()) + 1, 0));

    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(0, -1)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(0, 0)->nHeight, 0);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(0, 3)->nHeight, 3);
    BOOST_CHECK_EQUAL(chain.FindEarliestAtLeast(0, 8)->nHeight, 8);
    BOOST_CHECK(!chain.FindEarliestAtLeast(0, 9));

    CBlockIndex* ret1 = chain.FindEarliestAtLeast(100, 2);
    BOOST_CHECK(ret1->nTimeMax >= 100 && ret1->nHeight == 2);
    BOOST_CHECK(!chain.FindEarliestAtLeast(300, 9));
    CBlockIndex* ret2 = chain.FindEarliestAtLeast(200, 4);
    BOOST_CHECK(ret2->nTimeMax >= 200 && ret2->nHeight == 4);
}

BOOST_AUTO_TEST_CASE(get_skip_height_test)
{
    // Even values: the rightmost set bit is zeroed
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
    // Odd values: the 2nd and 3rd set bits are zeroed
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
    BOOST_CHECK_EQUAL(GetSkipHeight(0),
                                    0);
    BOOST_CHECK_EQUAL(GetSkipHeight(1),
                                    0);
    BOOST_CHECK_EQUAL(GetSkipHeight(2),
                                    0);
    BOOST_CHECK_EQUAL(GetSkipHeight(3),
                                    1);
    BOOST_CHECK_EQUAL(GetSkipHeight(4),
                                    0);
    BOOST_CHECK_EQUAL(GetSkipHeight(5),
                                    1);
    BOOST_CHECK_EQUAL(GetSkipHeight(6),
                                    4);
    BOOST_CHECK_EQUAL(GetSkipHeight(7),
                                    1);
    BOOST_CHECK_EQUAL(GetSkipHeight(8),
                                    0);
    BOOST_CHECK_EQUAL(GetSkipHeight(9),
                                    1);
    BOOST_CHECK_EQUAL(GetSkipHeight(10),
                                    8);
}

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
    // number of iterations can be measured, that gives a more precise
    // metric than CPU time.

    // Build a chain
    constexpr int chain_size{1'000'000};
    std::vector<CBlockIndex> block_index(chain_size);
    for (auto i{0}; i < chain_size; ++i) {
        block_index[i].pprev = i == 0 ? nullptr : &block_index[i - 1];
        block_index[i].nHeight = i;
        block_index[i].BuildSkip();
    }

    constexpr auto min_distance{100};
    FastRandomContext ctx(/*fDeterministic=*/true);
    // Repeat iterations. The test should work with much higher numbers as well,
    // kept reasonable to keep execution time low
    for (auto i{0}; i < 1'000; ++i) {
        // pick a height in the top part of the chain (so there is room to go down)
        const int start_height{chain_size - 1 - ctx.randrange(chain_size * 9 / 10)};
        // pick a target height, earlier by at least min_distance
        const int delta{min_distance + ctx.randrange(start_height - min_distance)};
        const int target_height{start_height - delta};
        BOOST_REQUIRE(start_height > 0 && start_height < chain_size);
        BOOST_REQUIRE(target_height > 0 && target_height < chain_size && target_height < start_height);

        // Perform traversal from start to target, skipping, as implemented in `GetAncestor`
        const auto& start_p{block_index[start_height]};
        const CBlockIndex* walk_p{&start_p};
        auto walk_height{start_height};
        int iteration_count{0};
        while (walk_height > target_height) {
            BOOST_REQUIRE(walk_p->pskip);
            BOOST_REQUIRE(walk_p->pprev);
            BOOST_REQUIRE(walk_p->pprev->pskip);
            const int skip_height{walk_p->pskip->nHeight};
            const int prev_skip_height{walk_p->pprev->pskip->nHeight};
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            if (skip_height == target_height ||
                (skip_height > target_height &&
                 !(prev_skip_height < skip_height - 2 && prev_skip_height >= target_height))) {
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
        BOOST_CHECK_LE(iteration_count, 132);
        // Dynamic bound, function of distance. Formula derived empirically.
        const auto bound_log_distance{8 * std::bit_width<unsigned>(delta) - 28};
        BOOST_CHECK_LE(iteration_count, bound_log_distance);

        // Also check that `GetAncestor` gives the same result
        auto p_ancestor{start_p.GetAncestor(target_height)};
        BOOST_CHECK_EQUAL(p_ancestor, &block_index[target_height]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
