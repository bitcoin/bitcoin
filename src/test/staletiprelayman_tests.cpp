// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <kernel/cs_main.h>
#include <node/staletiprelayman.h>
#include <staletipmessage.h>
#include <test/util/blocktree.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(staletiprelayman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(staletip_make_message)
{
    using node::MakeStaleTipMessage;

    LOCK(::cs_main);

    BlockTree tree;
    CBlockIndex* fork_point{tree.Add(nullptr, /*active=*/true)};
    CBlockIndex* first{tree.Add(fork_point, /*active=*/false)};
    CBlockIndex* tip{tree.Add(first, /*active=*/false)};

    const StaleTipMessage message{MakeStaleTipMessage({.fork_point = fork_point, .tip = tip}, /*have_block=*/true)};

    BOOST_CHECK_EQUAL(message.fork_point.ToString(), fork_point->GetBlockHash().ToString());
    BOOST_REQUIRE_EQUAL(message.headers.size(), 2U);
    BOOST_CHECK_EQUAL(message.headers.front().merkle_root.ToString(), first->hashMerkleRoot.ToString());
    BOOST_CHECK_EQUAL(message.headers.back().nonce, tip->nNonce);
    BOOST_CHECK(message.have_block);

    const auto decompressed{message.DecompressHeaders()};
    BOOST_REQUIRE_EQUAL(decompressed.size(), 2U);
    BOOST_CHECK_EQUAL(decompressed.front().GetHash().ToString(), first->GetBlockHash().ToString());
    BOOST_CHECK_EQUAL(decompressed.back().GetHash().ToString(), tip->GetBlockHash().ToString());
}

BOOST_AUTO_TEST_SUITE_END()
