// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"
#include "uint256.h"
#include "test/test_bitcoin.h"
#include "chainparams.h"

#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(Checkpoints_tests, BasicTestingSetup)

// The hash of the genesis block it's the genesis checkpoint and chain id
BOOST_AUTO_TEST_CASE(genesisblockhash_test)
{
    std::map<std::string, uint256>::const_iterator iter;
    for (iter = CChainParams::supportedChains.begin(); iter != CChainParams::supportedChains.end(); ++iter) {
        const CChainParams& chainparams = Params(iter->first);
        std::string hashStr = chainparams.GenesisBlock().GetHash().GetHex();
        BOOST_CHECK_EQUAL(hashStr, iter->second.GetHex());
        BOOST_CHECK_EQUAL(hashStr, chainparams.GetConsensus().hashGenesisBlock.GetHex());
        const uint256& genesisCheckpoint = chainparams.Checkpoints().mapCheckpoints.find(0)->second;
        BOOST_CHECK_EQUAL(hashStr, genesisCheckpoint.GetHex());
    }
}    

BOOST_AUTO_TEST_CASE(sanity)
{
    const Checkpoints::CCheckpointData& checkpoints = Params(CBaseChainParams::MAIN).Checkpoints();
    uint256 p11111 = uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d");
    uint256 p134444 = uint256S("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe");
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111, p11111));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444, p134444));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 11111, p134444));
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 134444, p11111));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111+1, p134444));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444+1, p11111));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate(checkpoints) >= 134444);
}    

BOOST_AUTO_TEST_SUITE_END()
