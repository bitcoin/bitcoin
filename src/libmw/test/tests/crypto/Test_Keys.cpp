// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/crypto/Blinds.h>
#include <mw/crypto/Keys.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestKeys, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(KeysTest)
{
    SecretKey key1 = SecretKey::Random();
    SecretKey key2 = SecretKey::Random();
    SecretKey sum_keys = Blinds().Add(key1).Add(key2).ToKey();
    PublicKey pubsum1 = Keys::From(key1).Add(key2).PubKey();

    BOOST_REQUIRE(PublicKey::From(sum_keys) == pubsum1);
}

BOOST_AUTO_TEST_SUITE_END()