// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(bech32_tests)

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
        "test1zg69w7y6hn0aqy352euf40x77qddq3dc",
        std::string(100, 'q'),
    };
    static const std::pair<std::string, std::vector<int>> ERRORS[] = {
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Bech32 string too long", {90}},
        {"Missing separator", {}},
        {"Invalid separator position", {0}},
        {"Invalid Base 32 character", {2}},
        {"Invalid separator position", {2}},
        {"Invalid character or mixed case", {8}},
        {"Invalid checksum", {}}, // The checksum is calculated using the uppercase form so the entire string is invalid, not just a few characters
        {"Invalid separator position", {0}},
        {"Invalid separator position", {0}},
        {"Invalid character or mixed case", {3, 4, 5, 7}},
        {"Invalid character or mixed case", {3}},
        {"Invalid Bech32 checksum", {11}},
        {"Invalid Bech32 checksum", {9, 16}},
        {"Bech32 string too long", {90}},
    };
    static_assert(std::size(CASES) == std::size(ERRORS), "Bech32 CASES and ERRORS should have the same length");

    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK_EQUAL(err.first, error);
        BOOST_CHECK(err.second == error_locations);
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
        "test1zg69v7y60n00qy352euf40x77qcusag6",
    };
    static const std::pair<std::string, std::vector<int>> ERRORS[] = {
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Bech32 string too long", {90}},
        {"Missing separator", {}},
        {"Invalid separator position", {0}},
        {"Invalid Base 32 character", {2}},
        {"Invalid Base 32 character", {3}},
        {"Invalid separator position", {2}},
        {"Invalid Base 32 character", {8}},
        {"Invalid Base 32 character", {7}},
        {"Invalid checksum", {}},
        {"Invalid separator position", {0}},
        {"Invalid separator position", {0}},
        {"Invalid Bech32m checksum", {21}},
        {"Invalid Bech32m checksum", {13, 32}},
    };
    static_assert(std::size(CASES) == std::size(ERRORS), "Bech32m CASES and ERRORS should have the same length");

    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK_EQUAL(err.first, error);
        BOOST_CHECK(err.second == error_locations);
        i++;
    }
}

BOOST_AUTO_TEST_CASE(bech32m_error_detection_guarantees)
{
    auto gen_valid_string = [](const std::string & hrp, int length) {
        const std::vector<uint8_t> data(length - 3 - bech32::CHECKSUM_SIZE, 0);
        const std::string valid = bech32::Encode(bech32::Encoding::BECH32M, hrp, data);
        return valid;
    };

    const auto valid_90 = gen_valid_string("bc", bech32::CharLimit::BECH32);
    const auto valid_91 = gen_valid_string("sp", bech32::CharLimit::BECH32 + 1);
    const auto valid_1023 = gen_valid_string("sp", bech32::CharLimit::SILENT_PAYMENTS);

    //Check valid strings of max lengths
    BOOST_CHECK(bech32::Decode(valid_90, bech32::CharLimit::BECH32).encoding == bech32::Encoding::BECH32M);
    BOOST_CHECK(bech32::Decode(valid_91, bech32::CharLimit::BECH32).encoding == bech32::Encoding::INVALID);
    BOOST_CHECK(bech32::Decode(valid_91, bech32::CharLimit::SILENT_PAYMENTS).encoding == bech32::Encoding::BECH32M);
    BOOST_CHECK(bech32::Decode(valid_1023, bech32::CharLimit::SILENT_PAYMENTS).encoding == bech32::Encoding::BECH32M);

    //Check error detection guarantees at base max length (90)
    //- up to 4 changes to a valid string cannot generate a valid checksum
    //- up to 2 error locations can be found, at the exact locations
    auto introduce_errors = [](const std::string& valid, const std::vector<int>& locations) {
        std::string invalid{valid};
        for (const int location : locations) {
            invalid[location] = invalid[location] == 'q' ? 'p' : 'q';
        }
        return invalid;
    };

    // 1 char changes are detected and can be located
    auto invalid = introduce_errors(valid_90, {3});
    auto errors = bech32::LocateErrors(invalid);
    BOOST_CHECK_EQUAL("Invalid Bech32m checksum", errors.first);
    BOOST_CHECK(errors.second == std::vector<int>{3});
    // 2 char changes are detected and can be located
    invalid = introduce_errors(valid_90, {3, 30});
    errors = bech32::LocateErrors(invalid);
    BOOST_CHECK_EQUAL("Invalid Bech32m checksum", errors.first);
    BOOST_CHECK(errors.second == std::vector<int>({3, 30}));
    //3 char differences fail checksum, errors can't be located anymore
    invalid = introduce_errors(valid_90, {3, 30, 60});
    errors = bech32::LocateErrors(invalid);
    BOOST_CHECK_EQUAL("Invalid checksum", errors.first);
    BOOST_CHECK(errors.second.empty()); // can only detect up to 2 locations
    // 4 char differences fail checksum, errors can't be located anymore
    invalid = introduce_errors(valid_90, {3, 30, 60, 80});
    errors = bech32::LocateErrors(invalid);
    BOOST_CHECK_EQUAL("Invalid checksum", errors.first);
    BOOST_CHECK(errors.second.empty());//can only detect up to 2 locations
    // 5 char changes might generate another valid checksum
    std::string collision{valid_90};
    collision[6] = 'j';
    collision[13] = '0';
    collision[23] = 'p';
    collision[33] = 'w';
    collision[49] = '5';
    BOOST_CHECK(bech32::Decode(collision, bech32::CharLimit::BECH32).encoding == bech32::Encoding::BECH32M);

    // Check error detection guarantees at silent payments max length (1023)
    //- up to 3 changes to a valid string cannot generate a valid checksum
    //- up to 2 error locations can be found, but not always at the exact locations
    //1 char change fails checksum and errors can be located
    invalid = introduce_errors(valid_1023, {3});
    errors = bech32::LocateErrors(invalid, bech32::CharLimit::SILENT_PAYMENTS);
    BOOST_CHECK_EQUAL("Invalid Bech32m checksum", errors.first);
    BOOST_CHECK(errors.second == std::vector<int>{3});
    //2 char errors fail checksum, and 2 errors can be located (but not guaranteed to be the correct ones)
    invalid = introduce_errors(valid_1023, {3, 500});
    errors = bech32::LocateErrors(invalid, bech32::CharLimit::SILENT_PAYMENTS);
    BOOST_CHECK_EQUAL("Invalid Bech32m checksum", errors.first);
    BOOST_CHECK(errors.second == std::vector<int>({3, 500}));
    // 3 char errors fail checksum, and we can't locate all 3 errors.
    invalid = introduce_errors(valid_1023, {3, 500, 1022});
    BOOST_CHECK(bech32::Decode(invalid, bech32::CharLimit::SILENT_PAYMENTS).encoding == bech32::Encoding::INVALID);
    errors = bech32::LocateErrors(invalid, bech32::CharLimit::SILENT_PAYMENTS);
    BOOST_CHECK_EQUAL("Invalid Bech32 checksum", errors.first);
    //here, a 3 char change can be fixed by only 2 char changes
    BOOST_CHECK(errors.second.size() == 2);
    // 4 char changes might generate another valid checksum
    collision = valid_1023;
    collision[3] = 'g';
    collision[63] = 'p';
    collision[113] = 'r';
    collision[413] = '6';
    std::size_t differences{0};
    for (std::size_t i{0}; i < valid_1023.size(); ++i) {
        differences += valid_1023[i] != collision[i];
    }
    BOOST_REQUIRE_EQUAL(differences, 4);
    BOOST_CHECK(bech32::Decode(collision, bech32::CharLimit::SILENT_PAYMENTS).encoding == bech32::Encoding::BECH32M);
}

BOOST_AUTO_TEST_SUITE_END()
