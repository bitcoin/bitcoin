// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>
#include <test/util/random.h>

#include <string>

using namespace std::literals;

BOOST_AUTO_TEST_SUITE(base64_tests)

BOOST_AUTO_TEST_CASE(base64_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","Zg==","Zm8=","Zm9v","Zm9vYg==","Zm9vYmE=","Zm9vYmFy"};
    for (unsigned int i=0; i<std::size(vstrIn); i++)
    {
        std::string strEnc = EncodeBase64(vstrIn[i]);
        BOOST_CHECK_EQUAL(strEnc, vstrOut[i]);
        auto dec = DecodeBase64(strEnc);
        BOOST_REQUIRE(dec);
        BOOST_CHECK_MESSAGE(MakeByteSpan(*dec) == MakeByteSpan(vstrIn[i]), vstrOut[i]);
    }

    {
        const std::vector<uint8_t> in_u{0xff, 0x01, 0xff};
        const std::vector<std::byte> in_b{std::byte{0xff}, std::byte{0x01}, std::byte{0xff}};
        const std::string in_s{"\xff\x01\xff"};
        const std::string out_exp{"/wH/"};
        BOOST_CHECK_EQUAL(EncodeBase64(in_u), out_exp);
        BOOST_CHECK_EQUAL(EncodeBase64(in_b), out_exp);
        BOOST_CHECK_EQUAL(EncodeBase64(in_s), out_exp);
    }

    // Decoding strings with embedded NUL characters should fail
    BOOST_CHECK(!DecodeBase64("invalid\0"s)); // correct size, invalid due to \0
    BOOST_CHECK( DecodeBase64("nQB/pZw="s)); // valid
    BOOST_CHECK(!DecodeBase64("nQB/pZw=\0invalid"s)); // correct size, invalid due to \0
    BOOST_CHECK(!DecodeBase64("nQB/pZw=invalid\0"s)); // invalid size
}

BOOST_AUTO_TEST_CASE(base64_padding)
{
    // Is valid without padding
    {
        const std::vector<uint8_t> in{'f', 'o', 'o', 'b', 'a', 'r'};
        const std::string out{"Zm9vYmFy"};
        BOOST_CHECK_EQUAL(EncodeBase64(in), out);
    }

    // Valid size
    BOOST_CHECK(!DecodeBase64("===="s));
    BOOST_CHECK(!DecodeBase64("a==="s));
    BOOST_CHECK( DecodeBase64("YQ=="s));
    BOOST_CHECK( DecodeBase64("YWE="s));
}

BOOST_AUTO_TEST_CASE(base64_roundtrip) {
    FastRandomContext rng;
    for (int i = 0; i < 100; ++i) {
        auto chars = rng.randbytes(rng.randrange(64));
        std::string randomStr(chars.begin(), chars.end());
        auto encoded = EncodeBase64(randomStr);

        auto decodedOpt = DecodeBase64(encoded);
        BOOST_REQUIRE_MESSAGE(decodedOpt, "Decoding failed for " + encoded);
        auto decoded = *decodedOpt;

        std::string decodedStr(decoded.begin(), decoded.end());
        BOOST_CHECK_MESSAGE(randomStr == decodedStr, "Roundtrip mismatch: original=" + randomStr + ", roundtrip=" + decodedStr);
    }
}

BOOST_AUTO_TEST_SUITE_END()
