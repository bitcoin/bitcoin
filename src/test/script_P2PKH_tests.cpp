// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/script.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(script_P2PKH_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(IsPayToPublicKeyHash)
{
    // Test CScript::IsPayToPublicKeyHash()
    uint160 dummy;
    CScript p2pkh;
    p2pkh << OP_DUP << OP_HASH160 << ToByteVector(dummy) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(p2pkh.IsPayToPublicKeyHash());

    static const unsigned char direct[] = {
        OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG
    };
    BOOST_CHECK(CScript(direct, direct+sizeof(direct)).IsPayToPublicKeyHash());

    static const unsigned char notp2pkh1[] = {
        OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_CHECKSIG
    };
    BOOST_CHECK(!CScript(notp2pkh1, notp2pkh1+sizeof(notp2pkh1)).IsPayToPublicKeyHash());

    static const unsigned char p2sh[] = {
        OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL
    };
    BOOST_CHECK(!CScript(p2sh, p2sh+sizeof(p2sh)).IsPayToPublicKeyHash());

    static const unsigned char extra[] = {
        OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_CHECKSIG
    };
    BOOST_CHECK(!CScript(extra, extra+sizeof(extra)).IsPayToPublicKeyHash());

    static const unsigned char missing[] = {
        OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_RETURN
    };
    BOOST_CHECK(!CScript(missing, missing+sizeof(missing)).IsPayToPublicKeyHash());

    static const unsigned char missing2[] = {
        OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    BOOST_CHECK(!CScript(missing2, missing2+sizeof(missing)).IsPayToPublicKeyHash());

    static const unsigned char tooshort[] = {
        OP_DUP, OP_HASH160, 2, 0,0, OP_EQUALVERIFY, OP_CHECKSIG
    };
    BOOST_CHECK(!CScript(tooshort, tooshort+sizeof(direct)).IsPayToPublicKeyHash());

}

BOOST_AUTO_TEST_SUITE_END()
