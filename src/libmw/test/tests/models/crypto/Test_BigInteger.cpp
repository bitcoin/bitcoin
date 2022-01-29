// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/models/crypto/BigInteger.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestBigInt, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(BigIntTest)
{
    BigInt<8> bigInt1 = BigInt<8>::FromHex("0123456789AbCdEf");
    BOOST_REQUIRE(bigInt1.ToHex() == "0123456789abcdef");
    BOOST_REQUIRE(bigInt1.size() == 8);
    BOOST_REQUIRE(!bigInt1.IsZero());
    BOOST_REQUIRE(BigInt<8>().IsZero());
    BOOST_REQUIRE(bigInt1.vec() == std::vector<uint8_t>({ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }));
    BOOST_REQUIRE((bigInt1.ToArray() == std::array<uint8_t, 8>({ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef })));

    BigInt<4> bigInt2 = BigInt<4>::Max();
    BOOST_REQUIRE(bigInt2.ToHex() == "ffffffff");

    BigInt<8> bigInt3 = BigInt<8>::ValueOf(12);
    BOOST_REQUIRE(bigInt3.ToHex() == "000000000000000c");

    BOOST_REQUIRE(bigInt1[3] == 0x67);
    BOOST_REQUIRE(bigInt1 > bigInt3);
    BOOST_REQUIRE(!(bigInt1 < bigInt3));
    BOOST_REQUIRE(!(bigInt1 < bigInt1));
    BOOST_REQUIRE(!(bigInt1 > bigInt1));
    BOOST_REQUIRE(bigInt1 >= bigInt1);
    BOOST_REQUIRE(bigInt1 <= bigInt1);
    BOOST_REQUIRE(bigInt1 == bigInt1);
    BOOST_REQUIRE(!(bigInt1 != bigInt1));

    BigInt<8> bigInt4 = bigInt1;
    BOOST_REQUIRE(bigInt1 == bigInt4);
    BOOST_REQUIRE(bigInt1 != bigInt3);

    BigInt<8> bigInt5 = bigInt1.vec();
    BOOST_REQUIRE(bigInt1 == bigInt5);
    bigInt5 = bigInt1.ToArray();
    BOOST_REQUIRE(bigInt1 == bigInt5);
    bigInt5 = BigInt<8>(bigInt1.data());
    BOOST_REQUIRE(bigInt1 == bigInt5);

    BOOST_REQUIRE((bigInt1 ^ bigInt3).ToHex() == "0123456789abcde3");
    bigInt1 ^= bigInt3;
    BOOST_REQUIRE(bigInt1.ToHex() == "0123456789abcde3");
    bigInt1 ^= bigInt3;
    BOOST_REQUIRE(bigInt1.ToHex() == "0123456789abcdef");

    std::vector<uint8_t> serialized = bigInt1.Serialized();
    BOOST_REQUIRE(bigInt1 == BigInt<8>::Deserialize(serialized));
}

BOOST_AUTO_TEST_SUITE_END()