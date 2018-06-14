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

    void FillBlock(CBlockIndex& block, CBlockIndex* prevBlock, const uint256& hash)
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

    void FillBlock(CBlockIndex& block, uint256& hash, CBlockIndex* prevBlock, size_t height)
    {
        FillHash(hash, height);
        FillBlock(block, height ? prevBlock : NULL, hash);

        assert(static_cast<int>(UintToArith256(block.GetBlockHash()).GetLow64()) == block.nHeight);
        assert(block.pprev == NULL || block.nHeight == block.pprev->nHeight + 1);

    }

    struct CalculateScoreFixture
    {
        const CMasternode mn;
        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;

        CalculateScoreFixture()
            : mn(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))))
            , hashes(1000)
            , blocks(1000)
        {
            BuildChain();
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

        ~CalculateScoreFixture()
        {
            chainActive = CChain();
        }
    };
}

BOOST_FIXTURE_TEST_SUITE(CalculateScore, CalculateScoreFixture)

    BOOST_AUTO_TEST_CASE(NullTip)
    {
        chainActive = CChain();
        BOOST_CHECK(mn.CalculateScore(1) == arith_uint256());
    }

    BOOST_AUTO_TEST_CASE(NotExistingBlock)
    {
        BOOST_CHECK(mn.CalculateScore(1001) == arith_uint256());
    }

    BOOST_AUTO_TEST_CASE(ScoreChanges)
    {
        CMasternode mn = CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN)));
        arith_uint256 score = mn.CalculateScore(100);

        // Change masternode vin
        mn.vin = CTxIn(COutPoint(ArithToUint256(2), 1 * COIN));

        // Calculate new score
        arith_uint256 newScore = mn.CalculateScore(100);

        BOOST_CHECK(score != newScore);
    }

    BOOST_AUTO_TEST_CASE(DistributionCheck)
    {
        std::vector<CMasternode> masternodes;
        masternodes.push_back(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))));
        masternodes.push_back(CreateMasternode(CTxIn(COutPoint(ArithToUint256(2), 1 * COIN))));
        masternodes.push_back(CreateMasternode(CTxIn(COutPoint(ArithToUint256(3), 1 * COIN))));
        masternodes.push_back(CreateMasternode(CTxIn(COutPoint(ArithToUint256(4), 1 * COIN))));
        masternodes.push_back(CreateMasternode(CTxIn(COutPoint(ArithToUint256(5), 1 * COIN))));

        // Find winner masternode for all 1000 blocks and
        // make sure all masternodes have the same probability to win
        std::vector<pair<int64_t, CTxIn> > vecMasternodeScores;
        std::map<int, int> winningCount;
        for (int i = 0; i <= chainActive.Height(); ++i)
        {
            arith_uint256 score = 0;
            int index = 0;
            for (size_t j = 0; j < masternodes.size(); ++j)
            {
                arith_uint256 s = masternodes[j].CalculateScore(i);
                if (s > score)
                {
                    score = s;
                    index = j;
                }
            }
            winningCount[index]++;
        }

        std::map<int, int>::iterator it = winningCount.begin();
        const int averageWinCount = chainActive.Height() / masternodes.size();
        for (; it != winningCount.end(); ++it)
        {
            // +- 15%
            BOOST_CHECK(it->second < averageWinCount + 0.15 * averageWinCount);
            BOOST_CHECK(it->second > averageWinCount - 0.15 * averageWinCount);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
