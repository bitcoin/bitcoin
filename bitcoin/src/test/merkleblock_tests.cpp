// Copyright (c) 2012-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <merkleblock.h>
#include <uint256.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(merkleblock_tests, BasicTestingSetup)

/**
 * Create a CMerkleBlock using a list of txids which will be found in the
 * given block.
 */
BOOST_AUTO_TEST_CASE(merkleblock_construct_from_txids_found)
{
    CBlock block = getBlock13b8a();

    std::set<uint256> txids;

    // Last txn in block.
    uint256 txhash1 = uint256S("0x74d681e0e03bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20");

    // Second txn in block.
    uint256 txhash2 = uint256S("0xf9fc751cb7dc372406a9f8d738d5e6f8f63bab71986a39cf36ee70ee17036d07");

    txids.insert(txhash1);
    txids.insert(txhash2);

    CMerkleBlock merkleBlock(block, txids);

    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());

    // vMatchedTxn is only used when bloom filter is specified.
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);

    std::vector<uint256> vMatched;
    std::vector<unsigned int> vIndex;

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 2U);

    // Ordered by occurrence in depth-first tree traversal.
    BOOST_CHECK_EQUAL(vMatched[0].ToString(), txhash2.ToString());
    BOOST_CHECK_EQUAL(vIndex[0], 1U);

    BOOST_CHECK_EQUAL(vMatched[1].ToString(), txhash1.ToString());
    BOOST_CHECK_EQUAL(vIndex[1], 8U);
}


/**
 * Create a CMerkleBlock using a list of txids which will not be found in the
 * given block.
 */
BOOST_AUTO_TEST_CASE(merkleblock_construct_from_txids_not_found)
{
    CBlock block = getBlock13b8a();

    std::set<uint256> txids2;
    txids2.insert(uint256S("0xc0ffee00003bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20"));
    CMerkleBlock merkleBlock(block, txids2);

    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);

    std::vector<uint256> vMatched;
    std::vector<unsigned int> vIndex;

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 0U);
    BOOST_CHECK_EQUAL(vIndex.size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
