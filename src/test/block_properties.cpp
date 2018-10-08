// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <boost/test/unit_test.hpp>
#include <rapidcheck/Gen.h>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>

#include <primitives/block.h>
#include <streams.h>
#include <test/gen/block_gen.h>
#include <test/setup_common.h>


BOOST_FIXTURE_TEST_SUITE(block_properties, BasicTestingSetup)

RC_BOOST_PROP(blockheader_serialization_symmetry, (const CBlockHeader& header))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << header;
    CBlockHeader header2;
    ss >> header2;
    CDataStream ss1(SER_NETWORK, PROTOCOL_VERSION);
    ss << header;
    ss1 << header2;
    RC_ASSERT(ss.str() == ss1.str());
}

RC_BOOST_PROP(block_serialization_symmetry, (const CBlock& block))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << block;
    CBlock block2;
    ss >> block2;
    RC_ASSERT(block.GetHash() == block2.GetHash());
}

BOOST_AUTO_TEST_SUITE_END()
