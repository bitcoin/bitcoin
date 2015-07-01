// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"
#include "uint256.h"
#include "test/test_bitcoin.h"
#include "chainparams.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(Checkpoints_tests, BasicTestingSetup)

// The hash of the genesis block it's the genesis checkpoint and chain id
BOOST_AUTO_TEST_CASE(genesisblockhash_test)
{
    std::map<std::string, uint256>::const_iterator iter;
    for (iter = CChainParams::supportedChains.begin(); iter != CChainParams::supportedChains.end(); ++iter) {
        const boost::scoped_ptr<CChainParams> testChainParams(CChainParams::Factory(iter->first));
        const CChainParams& chainparams = *testChainParams;
        std::string hashStr = chainparams.GenesisBlock().GetHash().GetHex();
        BOOST_CHECK_EQUAL(hashStr, iter->second.GetHex());
        BOOST_CHECK_EQUAL(hashStr, chainparams.GetConsensus().hashGenesisBlock.GetHex());
        const uint256& genesisCheckpoint = chainparams.Checkpoints().mapCheckpoints.find(0)->second;
        BOOST_CHECK_EQUAL(hashStr, genesisCheckpoint.GetHex());
    }
}    

BOOST_AUTO_TEST_CASE(sanity)
{
    const boost::scoped_ptr<CChainParams> testChainParams(CChainParams::Factory(CBaseChainParams::MAIN));
    const CCheckpointData& checkpoints = testChainParams->Checkpoints();
    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate(checkpoints) >= 134444);
}

BOOST_AUTO_TEST_SUITE_END()
