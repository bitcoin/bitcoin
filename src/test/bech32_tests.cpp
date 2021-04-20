// Copyright (c) 2017 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/util/setup_common.h>
#include <test/util/str.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(bech32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bech32_testvectors_valid)
{
    static const std::string CASES[] = {
        "A12UEL5L",
        "a12uel5l",
        "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs",
        "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
        "11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j",
        "split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w",
        "?1ezyfcl",
    };
    for (const std::string& str : CASES) {
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::BECH32);
        std::string recode = bech32::Encode(bech32::Encoding::BECH32, dec.hrp, dec.data);
        BOOST_CHECK(!recode.empty());
        BOOST_CHECK(CaseInsensitiveEqual(str, recode));
    }
}

BOOST_AUTO_TEST_CASE(bech32m_testvectors_valid)
{
    static const std::string CASES[] = {
        "A1LQFN3A",
        "a1lqfn3a",
        "an83characterlonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11sg7hg6",
        "abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx",
        "11llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllludsr8",
        "split1checkupstagehandshakeupstreamerranterredcaperredlc445v",
        "?1v759aa"
    };
    for (const std::string& str : CASES) {
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::BECH32M);
        std::string recode = bech32::Encode(bech32::Encoding::BECH32M, dec.hrp, dec.data);
        BOOST_CHECK(!recode.empty());
        BOOST_CHECK(CaseInsensitiveEqual(str, recode));
    }
}

BOOST_AUTO_TEST_CASE(bech32_testvectors_invalid)
{
    static const std::string CASES[] = {
        " 1nwldj5",
        "\x7f""1axkwrx",
        "\x80""1eym55h",
        "an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx",
        "pzry9x0s0muk",
        "1pzry9x0s0muk",
        "x1b4n0q5v",
        "li1dgmt3",
        "de1lg7wt\xff",
        "A1G7SGD8",
        "10a06t8",
        "1qzzfhee",
        "a12UEL5L",
        "A12uEL5L",
        "abcdef1qpzrz9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
    };
    static const std::pair<std::string, int> ERRORS[] = {
        {"Invalid character or mixed case", 0},
        {"Invalid character or mixed case", 0},
        {"Invalid character or mixed case", 0},
        {"Bech32 string too long", 90},
        {"Missing separator", -1},
        {"Invalid separator position", 0},
        {"Invalid Base 32 character", 2},
        {"Invalid separator position", 2},
        {"Invalid character or mixed case", 8},
        {"Invalid checksum", -1}, // The checksum is calculated using the uppercase form so the entire string is invalid, not just a few characters
        {"Invalid separator position", 0},
        {"Invalid separator position", 0},
        {"Invalid character or mixed case", 3},
        {"Invalid character or mixed case", 3},
        {"Invalid checksum", 11}
    };
    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        std::vector<int> error_locations;
        std::string error = bech32::LocateErrors(str, error_locations);
        BOOST_CHECK_EQUAL(err.first, error);
        if (err.second == -1) BOOST_CHECK(error_locations.empty());
        else BOOST_CHECK_EQUAL(err.second, error_locations[0]);
        i++;
    }
}

BOOST_AUTO_TEST_CASE(bech32m_testvectors_invalid)
{
    static const std::string CASES[] = {
        " 1xj0phk",
        "\x7f""1g6xzxy",
        "\x80""1vctc34",
        "an84characterslonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11d6pts4",
        "qyrz8wqd2c9m",
        "1qyrz8wqd2c9m",
        "y1b0jsk6g",
        "lt1igcx5c0",
        "in1muywd",
        "mm1crxm3i",
        "au1s5cgom",
        "M1VUXWEZ",
        "16plkw9",
        "1p2gdwpf",
        "abcdef1l7aum6echk45nj2s0wdvt2fg8x9yrzpqzd3ryx",
    };
    static const std::pair<std::string, int> ERRORS[] = {
        {"Invalid character or mixed case", 0},
        {"Invalid character or mixed case", 0},
        {"Invalid character or mixed case", 0},
        {"Bech32 string too long", 90},
        {"Missing separator", -1},
        {"Invalid separator position", 0},
        {"Invalid Base 32 character", 2},
        {"Invalid Base 32 character", 3},
        {"Invalid separator position", 2},
        {"Invalid Base 32 character", 8},
        {"Invalid Base 32 character", 7},
        {"Invalid checksum", -1},
        {"Invalid separator position", 0},
        {"Invalid separator position", 0},
        {"Invalid checksum", 21},
    };
    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        std::vector<int> error_locations;
        std::string error = bech32::LocateErrors(str, error_locations);
        BOOST_CHECK_EQUAL(err.first, error);
        if (err.second == -1) BOOST_CHECK(error_locations.empty());
        else BOOST_CHECK_EQUAL(err.second, error_locations[0]);
        i++;
    }
}

BOOST_AUTO_TEST_SUITE_END()
