// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <codex32.h>
#include <test/util/str.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <string>

BOOST_AUTO_TEST_SUITE(codex32_tests)

BOOST_AUTO_TEST_CASE(codex32_bip93_misc_invalid)
{
    // This example uses a "0" threshold with a non-"s" index
    const std::string input1 = "ms10fauxxxxxxxxxxxxxxxxxxxxxxxxxxxx0z26tfn0ulw3p";
    const auto dec1 = codex32::Result{input1};
    BOOST_CHECK_EQUAL(dec1.error(), codex32::INVALID_SHARE_IDX);

    // This example has a threshold that is not a digit.
    const std::string input2 = "ms1fauxxxxxxxxxxxxxxxxxxxxxxxxxxxxxda3kr3s0s2swg";
    const auto dec2 = codex32::Result{input2};
    BOOST_CHECK_EQUAL(dec2.error(), codex32::INVALID_K);
}

BOOST_AUTO_TEST_CASE(codex32_bip93_vector_1)
{
    const std::string input = "ms10testsxxxxxxxxxxxxxxxxxxxxxxxxxx4nzvca9cmczlw";

    const auto dec = codex32::Result{input};
    BOOST_CHECK(dec.IsValid());
    BOOST_CHECK_EQUAL(dec.error(), codex32::OK);
    BOOST_CHECK_EQUAL(dec.GetHrp(), "ms");
    BOOST_CHECK_EQUAL(dec.GetK(), 0);
    BOOST_CHECK_EQUAL(dec.GetIdString(), "test");
    BOOST_CHECK_EQUAL(dec.GetShareIndex(), 's');

    const auto payload = dec.GetPayload();
    BOOST_CHECK_EQUAL(payload.size(), 16);
    BOOST_CHECK_EQUAL(HexStr(payload), "318c6318c6318c6318c6318c6318c631");

    // Try re-encoding
    BOOST_CHECK_EQUAL(input, dec.Encode());

    // Try directly constructing the share
    const auto direct = codex32::Result("ms", 0, "test", 's', payload);
    BOOST_CHECK(direct.IsValid());
    BOOST_CHECK_EQUAL(direct.error(), codex32::OK);
    BOOST_CHECK_EQUAL(direct.GetIdString(), "test");
    BOOST_CHECK_EQUAL(direct.GetShareIndex(), 's');

    // We cannot check that the codex32 string is exactly the same as the
    // input, since it will not be -- the test vector has nonzero trailing
    // bits while our code will always choose zero trailing bits. But we
    // can at least check that the payloads are identical.
    const auto payload_direct = direct.GetPayload();
    BOOST_CHECK_EQUAL(HexStr(payload), HexStr(payload_direct));
}

BOOST_AUTO_TEST_CASE(codex32_bip93_vector_2)
{
    const std::string input_a = "MS12NAMEA320ZYXWVUTSRQPNMLKJHGFEDCAXRPP870HKKQRM";
    const auto dec_a = codex32::Result{input_a};
    BOOST_CHECK(dec_a.IsValid());
    BOOST_CHECK_EQUAL(dec_a.GetHrp(), "ms");
    BOOST_CHECK_EQUAL(dec_a.GetK(), 2);
    BOOST_CHECK_EQUAL(dec_a.GetIdString(), "name");
    BOOST_CHECK_EQUAL(dec_a.GetShareIndex(), 'a');
    BOOST_CHECK(CaseInsensitiveEqual(input_a, dec_a.Encode()));

    const std::string input_c = "MS12NAMECACDEFGHJKLMNPQRSTUVWXYZ023FTR2GDZMPY6PN";
    const auto dec_c = codex32::Result{input_c};
    BOOST_CHECK(dec_c.IsValid());
    BOOST_CHECK_EQUAL(dec_c.GetHrp(), "ms");
    BOOST_CHECK_EQUAL(dec_c.GetK(), 2);
    BOOST_CHECK_EQUAL(dec_c.GetIdString(), "name");
    BOOST_CHECK_EQUAL(dec_c.GetShareIndex(), 'c');
    BOOST_CHECK(CaseInsensitiveEqual(input_c, dec_c.Encode()));
}

BOOST_AUTO_TEST_SUITE_END()
