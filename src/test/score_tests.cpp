// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode.h"
#include "keystore.h"
#include <boost/test/unit_test.hpp>

namespace
{
    CMasternode CreateMasternode(CTxIn vin)
    {
        CMasternode mn;
        mn.vin = vin;
        mn.activeState = CMasternode::MASTERNODE_ENABLED;
        return mn;
    }

    void FillBlock(CBlockIndex& block, /*const*/ CBlockIndex* prevBlock, const uint256& hash)
    {
        if (prevBlock)
        {
            block.nHeight = prevBlock->nHeight + 1;
            block.pprev = prevBlock;
        }
        else
        {
            block.nHeight = 0;
        }

        block.phashBlock = &hash;
        block.BuildSkip();
    }

    void FillHash(uint256& hash, const arith_uint256& height)
    {
        hash = ArithToUint256(height);
    }

    void FillBlock(CBlockIndex& block, uint256& hash, /*const*/ CBlockIndex* prevBlock, size_t height)
    {
        FillHash(hash, height);
        FillBlock(block, height ? prevBlock : NULL, hash);

        assert(static_cast<int>(UintToArith256(block.GetBlockHash()).GetLow64()) == block.nHeight);
        assert(block.pprev == NULL || block.nHeight == block.pprev->nHeight + 1);

    }

    struct CalculateScoreFixture
    {
        const CMasternode mn1;
        const CMasternode mn2;
        const CMasternode mn3;
        const CMasternode mn4;
        const CMasternode mn5;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;
        std::vector<CMasternode> masternodes;

        CalculateScoreFixture()
            : mn1(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))))
            , mn2(CreateMasternode(CTxIn(COutPoint(ArithToUint256(2), 1 * COIN))))
            , mn3(CreateMasternode(CTxIn(COutPoint(ArithToUint256(3), 1 * COIN))))
            , mn4(CreateMasternode(CTxIn(COutPoint(ArithToUint256(4), 1 * COIN))))
            , mn5(CreateMasternode(CTxIn(COutPoint(ArithToUint256(5), 1 * COIN))))
            , hashes(1000)
            , blocks(1000)
        {
            masternodes.push_back(mn1);
            masternodes.push_back(mn2);
            masternodes.push_back(mn3);
            masternodes.push_back(mn4);
            masternodes.push_back(mn5);
        }

        void BuildChain()
        {
            // Build a main chain 1000 blocks long.
            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks.back());
        }

    };
}

BOOST_FIXTURE_TEST_SUITE(CalculateScore, CalculateScoreFixture)

    BOOST_AUTO_TEST_CASE(NullTip)
    {
        BOOST_CHECK(mn1.CalculateScore(1) == arith_uint256());
    }

    BOOST_AUTO_TEST_CASE(NotExistingBlock)
    {
        BuildChain();
        BOOST_CHECK(mn1.CalculateScore(1001) == arith_uint256());
    }

    BOOST_AUTO_TEST_CASE(ScoreChanges)
    {
        BuildChain();
        CMasternode mn = CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN)));
        int64_t score = mn.CalculateScore(100).GetCompact(false);

        // Change masternode vin
        mn.vin = CTxIn(COutPoint(ArithToUint256(2), 1 * COIN));

        // Calculate new score
        int64_t newScore = mn.CalculateScore(100).GetCompact(false);

        BOOST_CHECK(score != newScore);
    }

    BOOST_AUTO_TEST_CASE(DistributionCheck)
    {
        // Find winner masternode for all 1000 blocks and
        // make sure all masternodes have the same probability to win
        BuildChain();

        std::vector<pair<int64_t, CTxIn> > vecMasternodeScores;
        std::map<int, int> winningCount;
        for (int i = 1; i < 1001; ++i)
        {
            int64_t score = 0;
            int index = 0;
            for (size_t j = 0; j < masternodes.size(); ++j)
            {
                int s = masternodes[j].CalculateScore(i).GetCompact(false);
                if (s > score)
                {
                    score = s;
                    index = j;
                }
            }
            winningCount[index]++;
        }

        std::map<int, int>::iterator it = winningCount.begin();
        for (; it != winningCount.end(); ++it)
        {
            // +- 15%
            BOOST_CHECK(it->second < 200 + 30);
            BOOST_CHECK(it->second > 200 - 30);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
