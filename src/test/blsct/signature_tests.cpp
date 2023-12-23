// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST
#define BLS_ETH 1

#include <blsct/common.h>
#include <blsct/private_key.h>
#include <boost/test/unit_test.hpp>
#include <streams.h>
#include <test/util/setup_common.h>

namespace blsct {

BOOST_FIXTURE_TEST_SUITE(signature_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_serialization_with_func_calls)
{
    PrivateKey sk(12345);
    auto sig = sk.SignBalance();
    DataStream st{};
    sig.Serialize(st);
    Signature recovered_sig;
    recovered_sig.Unserialize(st);

    BOOST_CHECK(mclBnG2_isEqual(&sig.m_data.v, &recovered_sig.m_data.v) == 1);
}

BOOST_AUTO_TEST_CASE(test_serialization_with_operators)
{
    PrivateKey sk(12345);
    auto sig = sk.SignBalance();
    DataStream st{};
    st << sig;
    Signature recovered_sig;
    st >> recovered_sig;

    BOOST_CHECK(mclBnG2_isEqual(&sig.m_data.v, &recovered_sig.m_data.v) == 1);
}

BOOST_AUTO_TEST_CASE(test_constructor)
{
    Signature s;
    BOOST_CHECK(mclBnG2_isZero(&s.m_data.v));

    Signature s2;
    BOOST_CHECK(s.GetVch() == s2.GetVch());
    BOOST_CHECK(s == s2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace blsct
