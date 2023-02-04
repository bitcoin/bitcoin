// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST
#define BLS_ETH 1

#include <boost/test/unit_test.hpp>
#include <streams.h>
#include <test/util/setup_common.h>
#include <blsct/bls_common.h>
#include <blsct/private_key.h>

namespace blsct {

BOOST_FIXTURE_TEST_SUITE(signature_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_serialization)
{
    PrivateKey sk(12345);
    auto sig = sk.SignBalance();
    CDataStream st(0, 0);
    sig.Serialize(st);

    Signature recovered_sig;
    recovered_sig.Unserialize(st);

    BOOST_CHECK(mclBnG2_isEqual(&sig.m_data.v, &recovered_sig.m_data.v) == 1);
}

BOOST_AUTO_TEST_SUITE_END()

}