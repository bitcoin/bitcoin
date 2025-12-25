// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/util/str.h>

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

/** BIP-173 specifies the valid character set for bech32 data part.
 *  This test verifies that all 32 valid characters encode and decode correctly.
 *  Reference: https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki
 */
BOOST_AUTO_TEST_CASE(bech32_charset_encoding)
{
    // The BIP-173 character set: qpzry9x8gf2tvdw0s3jn54khce6mua7l
    // Each character maps to a 5-bit value (0-31)
    const std::string CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    // Test that each value 0-31 encodes to the correct character
    for (uint8_t i = 0; i < 32; ++i) {
        std::vector<uint8_t> data = {i};
        std::string encoded = bech32::Encode(bech32::Encoding::BECH32, "test", data);
        BOOST_CHECK(!encoded.empty());

        // The data character should be at position 5 (after "test1")
        BOOST_CHECK_EQUAL(encoded[5], CHARSET[i]);

        // Verify round-trip
        const auto decoded = bech32::Decode(encoded);
        BOOST_CHECK(decoded.encoding == bech32::Encoding::BECH32);
        BOOST_CHECK_EQUAL(decoded.hrp, "test");
        BOOST_REQUIRE_EQUAL(decoded.data.size(), 1U);
        BOOST_CHECK_EQUAL(decoded.data[0], i);
    }
}

/** Test round-trip encoding for various data sizes.
 *  Verifies that Encode followed by Decode returns the original data.
 */
BOOST_AUTO_TEST_CASE(bech32_roundtrip_encoding)
{
    const std::string hrp = "bc";

    // Test various data lengths
    for (size_t data_len = 0; data_len <= 40; ++data_len) {
        std::vector<uint8_t> data(data_len);
        // Fill with deterministic pattern
        for (size_t i = 0; i < data_len; ++i) {
            data[i] = static_cast<uint8_t>(i % 32);
        }

        // Test both Bech32 and Bech32m
        for (auto encoding : {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
            std::string encoded = bech32::Encode(encoding, hrp, data);
            BOOST_CHECK(!encoded.empty());

            const auto decoded = bech32::Decode(encoded);
            BOOST_CHECK(decoded.encoding == encoding);
            BOOST_CHECK_EQUAL(decoded.hrp, hrp);
            BOOST_CHECK(decoded.data == data);
        }
    }
}

/** Test the 90-character limit specified in BIP-173.
 *  Strings at exactly 90 characters should be valid.
 *  Strings exceeding 90 characters should be invalid.
 */
BOOST_AUTO_TEST_CASE(bech32_length_limits)
{
    // Create a string of exactly 90 characters
    // HRP (1) + separator (1) + data (82) + checksum (6) = 90
    std::vector<uint8_t> max_data(82);
    for (size_t i = 0; i < max_data.size(); ++i) {
        max_data[i] = static_cast<uint8_t>(i % 32);
    }

    std::string at_limit = bech32::Encode(bech32::Encoding::BECH32, "a", max_data);
    BOOST_CHECK_EQUAL(at_limit.size(), 90U);

    const auto dec_at_limit = bech32::Decode(at_limit);
    BOOST_CHECK(dec_at_limit.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_at_limit.hrp, "a");
    BOOST_CHECK(dec_at_limit.data == max_data);

    // Verify that 91 characters is rejected
    // Use the existing test vector from BIP-173
    const std::string too_long = "an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx";
    BOOST_CHECK(too_long.size() > 90U);
    const auto dec_too_long = bech32::Decode(too_long);
    BOOST_CHECK(dec_too_long.encoding == bech32::Encoding::INVALID);
}

/** Test HRP edge cases as specified in BIP-173.
 *  HRP must be 1-83 characters, using ASCII 33-126.
 */
BOOST_AUTO_TEST_CASE(bech32_hrp_limits)
{
    std::vector<uint8_t> data = {0, 1, 2};

    // Minimum HRP length (1 character)
    std::string min_hrp = bech32::Encode(bech32::Encoding::BECH32, "a", data);
    BOOST_CHECK(!min_hrp.empty());
    auto dec_min = bech32::Decode(min_hrp);
    BOOST_CHECK(dec_min.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_min.hrp, "a");

    // HRP with boundary ASCII characters (33 '!' and 126 '~')
    std::string boundary_hrp = bech32::Encode(bech32::Encoding::BECH32, "!~", data);
    BOOST_CHECK(!boundary_hrp.empty());
    auto dec_boundary = bech32::Decode(boundary_hrp);
    BOOST_CHECK(dec_boundary.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_boundary.hrp, "!~");

    // HRP containing the separator character '1' (last '1' should be the separator)
    std::string hrp_with_one = bech32::Encode(bech32::Encoding::BECH32, "a1b", data);
    BOOST_CHECK(!hrp_with_one.empty());
    auto dec_with_one = bech32::Decode(hrp_with_one);
    BOOST_CHECK(dec_with_one.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_with_one.hrp, "a1b");

    // Empty HRP should fail to decode
    const std::string empty_hrp = "1qqqqqqqqqq3nmt9y";
    auto dec_empty = bech32::Decode(empty_hrp);
    BOOST_CHECK(dec_empty.encoding == bech32::Encoding::INVALID);
}

/** Test that single-character errors are detected.
 *  BIP-173 guarantees detection of any error affecting at most 4 characters.
 */
BOOST_AUTO_TEST_CASE(bech32_single_error_detection)
{
    // Valid test string
    const std::string valid = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    auto dec_valid = bech32::Decode(valid);
    BOOST_CHECK(dec_valid.encoding == bech32::Encoding::BECH32);

    // Introduce a single character error at various positions in the data part
    const size_t data_start = 4; // After "bc1"
    const size_t data_end = valid.size() - 1; // Before last checksum char

    for (size_t pos = data_start; pos < data_end; ++pos) {
        std::string corrupted = valid;
        // Change the character to a different valid bech32 character
        char orig = corrupted[pos];
        corrupted[pos] = (orig == 'q') ? 'p' : 'q';

        auto dec_corrupted = bech32::Decode(corrupted);
        BOOST_CHECK_MESSAGE(dec_corrupted.encoding == bech32::Encoding::INVALID,
            "Single error at position " + std::to_string(pos) + " was not detected");
    }
}

/** Test that adjacent character transpositions are detected.
 *  This is a common type of human error when transcribing addresses.
 */
BOOST_AUTO_TEST_CASE(bech32_transposition_detection)
{
    // Valid test string with distinct adjacent characters
    const std::string valid = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    auto dec_valid = bech32::Decode(valid);
    BOOST_CHECK(dec_valid.encoding == bech32::Encoding::BECH32);

    // Test transpositions in the data part
    const size_t data_start = 4;
    const size_t data_end = valid.size() - 2;

    for (size_t pos = data_start; pos < data_end; ++pos) {
        if (valid[pos] == valid[pos + 1]) continue; // Skip if chars are same

        std::string transposed = valid;
        std::swap(transposed[pos], transposed[pos + 1]);

        auto dec_transposed = bech32::Decode(transposed);
        BOOST_CHECK_MESSAGE(dec_transposed.encoding == bech32::Encoding::INVALID,
            "Transposition at position " + std::to_string(pos) + " was not detected");
    }
}

/** Test error detection with multiple errors (up to 4).
 *  BIP-173 guarantees detection of up to 4 errors.
 */
BOOST_AUTO_TEST_CASE(bech32_multiple_error_detection)
{
    const std::string valid = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    auto dec_valid = bech32::Decode(valid);
    BOOST_CHECK(dec_valid.encoding == bech32::Encoding::BECH32);

    // Test with 2 errors
    {
        std::string corrupted = valid;
        corrupted[5] = (corrupted[5] == 'q') ? 'p' : 'q';
        corrupted[10] = (corrupted[10] == 'q') ? 'p' : 'q';
        auto dec = bech32::Decode(corrupted);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
    }

    // Test with 3 errors
    {
        std::string corrupted = valid;
        corrupted[5] = (corrupted[5] == 'q') ? 'p' : 'q';
        corrupted[10] = (corrupted[10] == 'q') ? 'p' : 'q';
        corrupted[15] = (corrupted[15] == 'q') ? 'p' : 'q';
        auto dec = bech32::Decode(corrupted);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
    }

    // Test with 4 errors
    {
        std::string corrupted = valid;
        corrupted[5] = (corrupted[5] == 'q') ? 'p' : 'q';
        corrupted[10] = (corrupted[10] == 'q') ? 'p' : 'q';
        corrupted[15] = (corrupted[15] == 'q') ? 'p' : 'q';
        corrupted[20] = (corrupted[20] == 'q') ? 'p' : 'q';
        auto dec = bech32::Decode(corrupted);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
    }
}

/** Test case sensitivity as specified in BIP-173.
 *  Encoders must always output lowercase.
 *  Decoders must not accept mixed case.
 */
BOOST_AUTO_TEST_CASE(bech32_case_sensitivity)
{
    // All uppercase should decode successfully (BIP-173 allows this)
    const std::string all_upper = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
    auto dec_upper = bech32::Decode(all_upper);
    BOOST_CHECK(dec_upper.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_upper.hrp, "bc");

    // All lowercase should decode successfully
    const std::string all_lower = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    auto dec_lower = bech32::Decode(all_lower);
    BOOST_CHECK(dec_lower.encoding == bech32::Encoding::BECH32);
    BOOST_CHECK_EQUAL(dec_lower.hrp, "bc");

    // Mixed case should fail
    const std::string mixed_case = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kV8f3t4";
    auto dec_mixed = bech32::Decode(mixed_case);
    BOOST_CHECK(dec_mixed.encoding == bech32::Encoding::INVALID);

    // Mixed case in HRP only should also fail
    const std::string mixed_hrp = "Bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    auto dec_mixed_hrp = bech32::Decode(mixed_hrp);
    BOOST_CHECK(dec_mixed_hrp.encoding == bech32::Encoding::INVALID);

    // Encoding should always produce lowercase
    std::vector<uint8_t> data = {0, 1, 2, 3};
    std::string encoded = bech32::Encode(bech32::Encoding::BECH32, "test", data);
    for (char c : encoded) {
        BOOST_CHECK(c < 'A' || c > 'Z'); // No uppercase letters
    }
}

/** Test checksum properties.
 *  Verify that the checksum is exactly 6 characters and changes with input.
 */
BOOST_AUTO_TEST_CASE(bech32_checksum_properties)
{
    std::vector<uint8_t> data1 = {0, 1, 2, 3};
    std::vector<uint8_t> data2 = {0, 1, 2, 4}; // One bit different

    std::string enc1 = bech32::Encode(bech32::Encoding::BECH32, "test", data1);
    std::string enc2 = bech32::Encode(bech32::Encoding::BECH32, "test", data2);

    // Both should encode successfully
    BOOST_CHECK(!enc1.empty());
    BOOST_CHECK(!enc2.empty());

    // Checksums should be different (last 6 characters)
    std::string checksum1 = enc1.substr(enc1.size() - 6);
    std::string checksum2 = enc2.substr(enc2.size() - 6);
    BOOST_CHECK(checksum1 != checksum2);

    // Verify checksum is exactly 6 characters
    BOOST_CHECK_EQUAL(checksum1.size(), 6U);
    BOOST_CHECK_EQUAL(checksum2.size(), 6U);

    // Same data should produce same checksum
    std::string enc1_again = bech32::Encode(bech32::Encoding::BECH32, "test", data1);
    BOOST_CHECK_EQUAL(enc1, enc1_again);

    // Different HRP should produce different result
    std::string enc_diff_hrp = bech32::Encode(bech32::Encoding::BECH32, "tset", data1);
    BOOST_CHECK(enc1 != enc_diff_hrp);
}

BOOST_AUTO_TEST_SUITE_END()
