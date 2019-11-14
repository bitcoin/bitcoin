// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/message.h>

#include <hash.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <string>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(util_message_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(hashes_tx_as_message)
{
    const std::string unsigned_tx = "...";
    const uint256 signature_hash = Hash(unsigned_tx.begin(), unsigned_tx.end());

    const uint256 message_hash = HashMessage(unsigned_tx);
    BOOST_CHECK_NE(message_hash, signature_hash);
}

BOOST_AUTO_TEST_SUITE_END()
