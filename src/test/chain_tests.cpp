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

BOOST_AUTO_TEST_CASE(cchain_basic_tests)
{
    auto genesis = CBlockIndex{};
    genesis.nHeight = 0;
    auto bi1 = CBlockIndex{};
    bi1.nHeight = 1;

    // Empty chain
    {
        auto chain_0 = CChain{};

        BOOST_CHECK_EQUAL(chain_0.Height(), -1);
        BOOST_CHECK_EQUAL(chain_0.Genesis(), nullptr);
        BOOST_CHECK_EQUAL(chain_0.Tip(), nullptr);
        BOOST_CHECK_EQUAL(chain_0[0], nullptr);
        BOOST_CHECK_EQUAL(chain_0.Contains(&genesis), false);
        BOOST_CHECK_EQUAL(chain_0.Contains(nullptr), false);
        BOOST_CHECK_EQUAL(chain_0.Next(&genesis), nullptr);
        BOOST_CHECK_EQUAL(chain_0.Next(nullptr), nullptr);
    }

    // Chain with 1 block
    {
        auto chain_1 = CChain{};
        chain_1.SetTip(genesis);

        BOOST_CHECK_EQUAL(chain_1.Height(), 0);
        BOOST_CHECK_EQUAL(chain_1.Genesis(), &genesis);
        BOOST_CHECK_EQUAL(chain_1.Tip(), &genesis);
        BOOST_CHECK_EQUAL(chain_1[-1], nullptr);
        BOOST_CHECK_EQUAL(chain_1[0], &genesis);
        BOOST_CHECK_EQUAL(chain_1[1], nullptr);
        BOOST_CHECK_EQUAL(chain_1.Contains(&genesis), true);
        BOOST_CHECK_EQUAL(chain_1.Contains(&bi1), false);
        BOOST_CHECK_EQUAL(chain_1.Next(&genesis), nullptr);
        BOOST_CHECK_EQUAL(chain_1.Next(&bi1), nullptr);
    }

    // Chain with 2 blocks
    {
        auto chain_2 = CChain{};
        chain_2.SetTip(genesis);
        chain_2.SetTip(bi1);

        BOOST_CHECK_EQUAL(chain_2.Height(), 1);
        BOOST_CHECK_EQUAL(chain_2.Genesis(), &genesis);
        BOOST_CHECK_EQUAL(chain_2.Tip(), &bi1);
        BOOST_CHECK_EQUAL(chain_2[-1], nullptr);
        BOOST_CHECK_EQUAL(chain_2[0], &genesis);
        BOOST_CHECK_EQUAL(chain_2[1], &bi1);
        BOOST_CHECK_EQUAL(chain_2[2], nullptr);
        BOOST_CHECK_EQUAL(chain_2.Contains(&genesis), true);
        BOOST_CHECK_EQUAL(chain_2.Contains(&bi1), true);
        BOOST_CHECK_EQUAL(chain_2.Next(&genesis), &bi1);
        BOOST_CHECK_EQUAL(chain_2.Next(&bi1), nullptr);
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
