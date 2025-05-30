// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/transaction_identifier.h>

#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

BOOST_AUTO_TEST_SUITE(uint256_tests)

const unsigned char R1Array[] =
    "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
    "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
const uint256 R1L = uint256(std::vector<unsigned char>(R1Array,R1Array+32));
const uint160 R1S = uint160(std::vector<unsigned char>(R1Array,R1Array+20));

const unsigned char R2Array[] =
    "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
    "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
const uint256 R2L = uint256(std::vector<unsigned char>(R2Array,R2Array+32));
const uint160 R2S = uint160(std::vector<unsigned char>(R2Array,R2Array+20));

const unsigned char ZeroArray[] =
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const uint256 ZeroL = uint256(std::vector<unsigned char>(ZeroArray,ZeroArray+32));
const uint160 ZeroS = uint160(std::vector<unsigned char>(ZeroArray,ZeroArray+20));

const unsigned char OneArray[] =
    "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const uint256 OneL = uint256(std::vector<unsigned char>(OneArray,OneArray+32));
const uint160 OneS = uint160(std::vector<unsigned char>(OneArray,OneArray+20));

const unsigned char MaxArray[] =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
const uint256 MaxL = uint256(std::vector<unsigned char>(MaxArray,MaxArray+32));
const uint160 MaxS = uint160(std::vector<unsigned char>(MaxArray,MaxArray+20));

static std::string ArrayToString(const unsigned char A[], unsigned int width)
{
    std::stringstream Stream;
    Stream << std::hex;
    for (unsigned int i = 0; i < width; ++i)
    {
        Stream<<std::setw(2)<<std::setfill('0')<<(unsigned int)A[width-i-1];
    }
    return Stream.str();
}

BOOST_AUTO_TEST_CASE( basics ) // constructors, equality, inequality
{
    // constructor uint256(vector<char>):
    BOOST_CHECK_EQUAL(R1L.ToString(), ArrayToString(R1Array,32));
    BOOST_CHECK_EQUAL(R1S.ToString(), ArrayToString(R1Array,20));
    BOOST_CHECK_EQUAL(R2L.ToString(), ArrayToString(R2Array,32));
    BOOST_CHECK_EQUAL(R2S.ToString(), ArrayToString(R2Array,20));
    BOOST_CHECK_EQUAL(ZeroL.ToString(), ArrayToString(ZeroArray,32));
    BOOST_CHECK_EQUAL(ZeroS.ToString(), ArrayToString(ZeroArray,20));
    BOOST_CHECK_EQUAL(OneL.ToString(), ArrayToString(OneArray,32));
    BOOST_CHECK_EQUAL(OneS.ToString(), ArrayToString(OneArray,20));
    BOOST_CHECK_EQUAL(MaxL.ToString(), ArrayToString(MaxArray,32));
    BOOST_CHECK_EQUAL(MaxS.ToString(), ArrayToString(MaxArray,20));
    BOOST_CHECK_NE(OneL.ToString(), ArrayToString(ZeroArray,32));
    BOOST_CHECK_NE(OneS.ToString(), ArrayToString(ZeroArray,20));

    // == and !=
    BOOST_CHECK_NE(R1L, R2L); BOOST_CHECK_NE(R1S, R2S);
    BOOST_CHECK_NE(ZeroL, OneL); BOOST_CHECK_NE(ZeroS, OneS);
    BOOST_CHECK_NE(OneL, ZeroL); BOOST_CHECK_NE(OneS, ZeroS);
    BOOST_CHECK_NE(MaxL, ZeroL); BOOST_CHECK_NE(MaxS, ZeroS);

    // String Constructor and Copy Constructor
    BOOST_CHECK_EQUAL(uint256::FromHex(R1L.ToString()).value(), R1L);
    BOOST_CHECK_EQUAL(uint256::FromHex(R2L.ToString()).value(), R2L);
    BOOST_CHECK_EQUAL(uint256::FromHex(ZeroL.ToString()).value(), ZeroL);
    BOOST_CHECK_EQUAL(uint256::FromHex(OneL.ToString()).value(), OneL);
    BOOST_CHECK_EQUAL(uint256::FromHex(MaxL.ToString()).value(), MaxL);
    BOOST_CHECK_EQUAL(uint256::FromHex(R1ArrayHex).value(), R1L);
    BOOST_CHECK_EQUAL(uint256(R1L), R1L);
    BOOST_CHECK_EQUAL(uint256(ZeroL), ZeroL);
    BOOST_CHECK_EQUAL(uint256(OneL), OneL);

    BOOST_CHECK_EQUAL(uint160::FromHex(R1S.ToString()).value(), R1S);
    BOOST_CHECK_EQUAL(uint160::FromHex(R2S.ToString()).value(), R2S);
    BOOST_CHECK_EQUAL(uint160::FromHex(ZeroS.ToString()).value(), ZeroS);
    BOOST_CHECK_EQUAL(uint160::FromHex(OneS.ToString()).value(), OneS);
    BOOST_CHECK_EQUAL(uint160::FromHex(MaxS.ToString()).value(), MaxS);
    BOOST_CHECK_EQUAL(uint160::FromHex(std::string_view{R1ArrayHex + 24, 40}).value(), R1S);

    BOOST_CHECK_EQUAL(uint160(R1S), R1S);
    BOOST_CHECK_EQUAL(uint160(ZeroS), ZeroS);
    BOOST_CHECK_EQUAL(uint160(OneS), OneS);
}

BOOST_AUTO_TEST_CASE( comparison ) // <= >= < >
{
    uint256 LastL;
    for (int i = 255; i >= 0; --i) {
        uint256 TmpL;
        *(TmpL.begin() + (i>>3)) |= 1<<(7-(i&7));
        BOOST_CHECK_LT(LastL, TmpL);
        LastL = TmpL;
    }

    BOOST_CHECK_LT(ZeroL, R1L);
    BOOST_CHECK_LT(R2L, R1L);
    BOOST_CHECK_LT(ZeroL, OneL);
    BOOST_CHECK_LT(OneL, MaxL);
    BOOST_CHECK_LT(R1L, MaxL);
    BOOST_CHECK_LT(R2L, MaxL);

    uint160 LastS;
    for (int i = 159; i >= 0; --i) {
        uint160 TmpS;
        *(TmpS.begin() + (i>>3)) |= 1<<(7-(i&7));
        BOOST_CHECK_LT(LastS, TmpS);
        LastS = TmpS;
    }
    BOOST_CHECK_LT(ZeroS, R1S);
    BOOST_CHECK_LT(R2S, R1S);
    BOOST_CHECK_LT(ZeroS, OneS);
    BOOST_CHECK_LT(OneS, MaxS);
    BOOST_CHECK_LT(R1S, MaxS);
    BOOST_CHECK_LT(R2S, MaxS);

    // Non-arithmetic uint256s compare from the beginning of their inner arrays:
    BOOST_CHECK_LT(R2L, R1L);
    // Ensure first element comparisons give the same order as above:
    BOOST_CHECK_LT(*R2L.begin(), *R1L.begin());
    // Ensure last element comparisons give a different result (swapped params):
    BOOST_CHECK_LT(*(R1L.end()-1), *(R2L.end()-1));
    // Hex strings represent reverse-encoded bytes, with lexicographic ordering:
    BOOST_CHECK_LT(uint256{"1000000000000000000000000000000000000000000000000000000000000000"},
                   uint256{"0000000000000000000000000000000000000000000000000000000000000001"});
}

BOOST_AUTO_TEST_CASE(methods) // GetHex FromHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    BOOST_CHECK_EQUAL(R1L.GetHex(), R1L.ToString());
    BOOST_CHECK_EQUAL(R2L.GetHex(), R2L.ToString());
    BOOST_CHECK_EQUAL(OneL.GetHex(), OneL.ToString());
    BOOST_CHECK_EQUAL(MaxL.GetHex(), MaxL.ToString());
    uint256 TmpL(R1L);
    BOOST_CHECK_EQUAL(TmpL, R1L);
    BOOST_CHECK_EQUAL(uint256::FromHex(R2L.ToString()).value(), R2L);
    BOOST_CHECK_EQUAL(uint256::FromHex(ZeroL.ToString()).value(), uint256());

    TmpL = uint256::FromHex(R1L.ToString()).value();
    BOOST_CHECK_EQUAL_COLLECTIONS(R1L.begin(), R1L.end(), R1Array, R1Array + uint256::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(TmpL.begin(), TmpL.end(), R1Array, R1Array + uint256::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(R2L.begin(), R2L.end(), R2Array, R2Array + uint256::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(ZeroL.begin(), ZeroL.end(), ZeroArray, ZeroArray + uint256::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(OneL.begin(), OneL.end(), OneArray, OneArray + uint256::size());
    BOOST_CHECK_EQUAL(R1L.size(), sizeof(R1L));
    BOOST_CHECK_EQUAL(sizeof(R1L), 32);
    BOOST_CHECK_EQUAL(R1L.size(), 32);
    BOOST_CHECK_EQUAL(R2L.size(), 32);
    BOOST_CHECK_EQUAL(ZeroL.size(), 32);
    BOOST_CHECK_EQUAL(MaxL.size(), 32);
    BOOST_CHECK_EQUAL(R1L.begin() + 32, R1L.end());
    BOOST_CHECK_EQUAL(R2L.begin() + 32, R2L.end());
    BOOST_CHECK_EQUAL(OneL.begin() + 32, OneL.end());
    BOOST_CHECK_EQUAL(MaxL.begin() + 32, MaxL.end());
    BOOST_CHECK_EQUAL(TmpL.begin() + 32, TmpL.end());
    BOOST_CHECK_EQUAL(GetSerializeSize(R1L), 32);
    BOOST_CHECK_EQUAL(GetSerializeSize(ZeroL), 32);

    DataStream ss{};
    ss << R1L;
    BOOST_CHECK_EQUAL(ss.str(), std::string(R1Array,R1Array+32));
    ss >> TmpL;
    BOOST_CHECK_EQUAL(R1L, TmpL);
    ss.clear();
    ss << ZeroL;
    BOOST_CHECK_EQUAL(ss.str(), std::string(ZeroArray,ZeroArray+32));
    ss >> TmpL;
    BOOST_CHECK_EQUAL(ZeroL, TmpL);
    ss.clear();
    ss << MaxL;
    BOOST_CHECK_EQUAL(ss.str(), std::string(MaxArray,MaxArray+32));
    ss >> TmpL;
    BOOST_CHECK_EQUAL(MaxL, TmpL);
    ss.clear();

    BOOST_CHECK_EQUAL(R1S.GetHex(), R1S.ToString());
    BOOST_CHECK_EQUAL(R2S.GetHex(), R2S.ToString());
    BOOST_CHECK_EQUAL(OneS.GetHex(), OneS.ToString());
    BOOST_CHECK_EQUAL(MaxS.GetHex(), MaxS.ToString());
    uint160 TmpS(R1S);
    BOOST_CHECK_EQUAL(TmpS, R1S);
    BOOST_CHECK_EQUAL(uint160::FromHex(R2S.ToString()).value(), R2S);
    BOOST_CHECK_EQUAL(uint160::FromHex(ZeroS.ToString()).value(), uint160());

    TmpS = uint160::FromHex(R1S.ToString()).value();
    BOOST_CHECK_EQUAL_COLLECTIONS(R1S.begin(), R1S.end(), R1Array, R1Array + uint160::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(TmpS.begin(), TmpS.end(), R1Array, R1Array + uint160::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(R2S.begin(), R2S.end(), R2Array, R2Array + uint160::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(ZeroS.begin(), ZeroS.end(), ZeroArray, ZeroArray + uint160::size());
    BOOST_CHECK_EQUAL_COLLECTIONS(OneS.begin(), OneS.end(), OneArray, OneArray + uint160::size());
    BOOST_CHECK_EQUAL(R1S.size(), sizeof(R1S));
    BOOST_CHECK_EQUAL(sizeof(R1S), 20);
    BOOST_CHECK_EQUAL(R1S.size(), 20);
    BOOST_CHECK_EQUAL(R2S.size(), 20);
    BOOST_CHECK_EQUAL(ZeroS.size(), 20);
    BOOST_CHECK_EQUAL(MaxS.size(), 20);
    BOOST_CHECK_EQUAL(R1S.begin() + 20, R1S.end());
    BOOST_CHECK_EQUAL(R2S.begin() + 20, R2S.end());
    BOOST_CHECK_EQUAL(OneS.begin() + 20, OneS.end());
    BOOST_CHECK_EQUAL(MaxS.begin() + 20, MaxS.end());
    BOOST_CHECK_EQUAL(TmpS.begin() + 20, TmpS.end());
    BOOST_CHECK_EQUAL(GetSerializeSize(R1S), 20);
    BOOST_CHECK_EQUAL(GetSerializeSize(ZeroS), 20);

    ss << R1S;
    BOOST_CHECK_EQUAL(ss.str(), std::string(R1Array,R1Array+20));
    ss >> TmpS;
    BOOST_CHECK_EQUAL(R1S, TmpS);
    ss.clear();
    ss << ZeroS;
    BOOST_CHECK_EQUAL(ss.str(), std::string(ZeroArray,ZeroArray+20));
    ss >> TmpS;
    BOOST_CHECK_EQUAL(ZeroS, TmpS);
    ss.clear();
    ss << MaxS;
    BOOST_CHECK_EQUAL(ss.str(), std::string(MaxArray,MaxArray+20));
    ss >> TmpS;
    BOOST_CHECK_EQUAL(MaxS, TmpS);
    ss.clear();
}

/**
 * Implemented as a templated function so it can be reused by other classes that have a FromHex()
 * method that wraps base_blob::FromHex(), such as transaction_identifier::FromHex().
 */
template <typename T>
void TestFromHex()
{
    constexpr unsigned int num_chars{T::size() * 2};
    static_assert(num_chars <= 64); // this test needs to be modified to allow for more than 64 hex chars
    const std::string valid_64char_input{"0123456789abcdef0123456789ABCDEF0123456789abcdef0123456789ABCDEF"};
    const auto valid_input{valid_64char_input.substr(0, num_chars)};
    {
        // check that lower and upper case hex characters are accepted
        auto valid_result{T::FromHex(valid_input)};
        BOOST_REQUIRE(valid_result);
        BOOST_CHECK_EQUAL(valid_result->ToString(), ToLower(valid_input));
    }
    {
        // check that only strings of size num_chars are accepted
        BOOST_CHECK(!T::FromHex(""));
        BOOST_CHECK(!T::FromHex("0"));
        BOOST_CHECK(!T::FromHex(valid_input.substr(0, num_chars / 2)));
        BOOST_CHECK(!T::FromHex(valid_input.substr(0, num_chars - 1)));
        BOOST_CHECK(!T::FromHex(valid_input + "0"));
    }
    {
        // check that non-hex characters are not accepted
        std::string invalid_chars{R"( !"#$%&'()*+,-./:;<=>?@GHIJKLMNOPQRSTUVWXYZ[\]^_`ghijklmnopqrstuvwxyz{|}~)"};
        for (auto c : invalid_chars) {
            BOOST_CHECK(!T::FromHex(valid_input.substr(0, num_chars - 1) + c));
        }
        // 0x prefixes are invalid
        std::string invalid_prefix{"0x" + valid_input};
        BOOST_CHECK(!T::FromHex(std::string_view(invalid_prefix.data(), num_chars)));
        BOOST_CHECK(!T::FromHex(invalid_prefix));
    }
    {
        // check that string_view length is respected
        std::string chars_68{valid_64char_input + "0123"};
        BOOST_CHECK_EQUAL(T::FromHex(std::string_view(chars_68.data(), num_chars)).value().ToString(), ToLower(valid_input));
        BOOST_CHECK(!T::FromHex(std::string_view(chars_68.data(), num_chars - 1))); // too short
        BOOST_CHECK(!T::FromHex(std::string_view(chars_68.data(), num_chars + 1))); // too long
    }
}

BOOST_AUTO_TEST_CASE(from_hex)
{
    TestFromHex<uint160>();
    TestFromHex<uint256>();
    TestFromHex<Txid>();
    TestFromHex<Wtxid>();
}

BOOST_AUTO_TEST_CASE(from_user_hex)
{
    BOOST_CHECK_EQUAL(uint256::FromUserHex(""), uint256::ZERO);
    BOOST_CHECK_EQUAL(uint256::FromUserHex("0x"), uint256::ZERO);
    BOOST_CHECK_EQUAL(uint256::FromUserHex("0"), uint256::ZERO);
    BOOST_CHECK_EQUAL(uint256::FromUserHex("00"), uint256::ZERO);
    BOOST_CHECK_EQUAL(uint256::FromUserHex("1"), uint256::ONE);
    BOOST_CHECK_EQUAL(uint256::FromUserHex("0x10"), uint256{0x10});
    BOOST_CHECK_EQUAL(uint256::FromUserHex("10"), uint256{0x10});
    BOOST_CHECK_EQUAL(uint256::FromUserHex("0xFf"), uint256{0xff});
    BOOST_CHECK_EQUAL(uint256::FromUserHex("Ff"), uint256{0xff});
    const std::string valid_hex_64{"0x0123456789abcdef0123456789abcdef0123456789ABDCEF0123456789ABCDEF"};
    BOOST_REQUIRE_EQUAL(valid_hex_64.size(), 2 + 64); // 0x prefix and 64 hex digits
    BOOST_CHECK_EQUAL(uint256::FromUserHex(valid_hex_64.substr(2)).value().ToString(), ToLower(valid_hex_64.substr(2)));
    BOOST_CHECK_EQUAL(uint256::FromUserHex(valid_hex_64.substr(0)).value().ToString(), ToLower(valid_hex_64.substr(2)));

    BOOST_CHECK(!uint256::FromUserHex("0x0 "));                       // no spaces at end,
    BOOST_CHECK(!uint256::FromUserHex(" 0x0"));                       // or beginning,
    BOOST_CHECK(!uint256::FromUserHex("0x 0"));                       // or middle,
    BOOST_CHECK(!uint256::FromUserHex(" "));                          // etc.
    BOOST_CHECK(!uint256::FromUserHex("0x0ga"));                      // invalid character
    BOOST_CHECK(!uint256::FromUserHex("x0"));                         // broken prefix
    BOOST_CHECK(!uint256::FromUserHex("0x0x00"));                     // two prefixes not allowed
    BOOST_CHECK(!uint256::FromUserHex(valid_hex_64.substr(2) + "0")); // 1 hex digit too many
    BOOST_CHECK(!uint256::FromUserHex(valid_hex_64 + "a"));           // 1 hex digit too many
    BOOST_CHECK(!uint256::FromUserHex(valid_hex_64 + " "));           // whitespace after max length
    BOOST_CHECK(!uint256::FromUserHex(valid_hex_64 + "z"));           // invalid character after max length
}

BOOST_AUTO_TEST_CASE( check_ONE )
{
    uint256 one = uint256{"0000000000000000000000000000000000000000000000000000000000000001"};
    BOOST_CHECK_EQUAL(one, uint256::ONE);
}

BOOST_AUTO_TEST_CASE(FromHex_vs_uint256)
{
    auto runtime_uint{uint256::FromHex("4A5E1E4BAAB89F3A32518A88C31BC87F618f76673e2cc77ab2127b7afdeda33b")};
    constexpr uint256 consteval_uint{  "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    BOOST_CHECK_EQUAL(consteval_uint, runtime_uint);
}

BOOST_AUTO_TEST_SUITE_END()
