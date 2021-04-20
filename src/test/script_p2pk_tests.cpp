// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/test_dash.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(script_p2pk_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(IsPayToPublicKey)
{
    // Test CScript::IsPayToPublicKey()
    static const unsigned char p2pkcompressedeven[] = {
            0x41, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, OP_CHECKSIG
    };
    BOOST_CHECK(CScript(p2pkcompressedeven, p2pkcompressedeven+sizeof(p2pkcompressedeven)).IsPayToPublicKey());

    static const unsigned char p2pkcompressedodd[] = {
            0x41, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, OP_CHECKSIG
    };
    BOOST_CHECK(CScript(p2pkcompressedodd, p2pkcompressedodd+sizeof(p2pkcompressedodd)).IsPayToPublicKey());

    static const unsigned char p2pkuncompressed[] = {
            0x41, 0x04, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, OP_CHECKSIG
    };
    BOOST_CHECK(CScript(p2pkuncompressed, p2pkuncompressed+sizeof(p2pkuncompressed)).IsPayToPublicKey());

    static const unsigned char missingop[] = {
            0x41, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    BOOST_CHECK(!CScript(missingop, missingop+sizeof(missingop)).IsPayToPublicKey());

    static const unsigned char wrongop[] = {
            0x41, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, OP_EQUALVERIFY
    };
    BOOST_CHECK(!CScript(wrongop, wrongop+sizeof(wrongop)).IsPayToPublicKey());

    static const unsigned char tooshort[] = {
            0x41, 0x02, 0, 0, OP_CHECKSIG
    };
    BOOST_CHECK(!CScript(tooshort, tooshort+sizeof(tooshort)).IsPayToPublicKey());

}

BOOST_AUTO_TEST_SUITE_END()
