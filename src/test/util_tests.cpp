// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <common/signmessage.h>
#include <hash.h>
#include <key.h>
#include <script/parsing.h>
#include <span.h>
#include <sync.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/bitdeque.h>
#include <util/byte_units.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/moneystr.h>
#include <util/overflow.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/vector.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <univalue.h>
#include <utility>
#include <vector>

#include <sys/types.h>

#ifndef WIN32
#include <sys/wait.h>
#endif

#include <boost/test/unit_test.hpp>

using namespace std::literals;
using namespace util::hex_literals;
using util::ConstevalHexDigit;
using util::Join;
using util::RemovePrefix;
using util::RemovePrefixView;
using util::ReplaceAll;
using util::Split;
using util::SplitString;
using util::TrimString;
using util::TrimStringView;

static const std::string STRING_WITH_EMBEDDED_NULL_CHAR{"1"s "\0" "1"s};

/* defined in logging.cpp */
namespace BCLog {
    std::string LogEscapeMessage(std::string_view str);
}

BOOST_FIXTURE_TEST_SUITE(util_tests, BasicTestingSetup)

namespace {
class NoCopyOrMove
{
public:
    int i;
    explicit NoCopyOrMove(int i) : i{i} { }

    NoCopyOrMove() = delete;
    NoCopyOrMove(const NoCopyOrMove&) = delete;
    NoCopyOrMove(NoCopyOrMove&&) = delete;
    NoCopyOrMove& operator=(const NoCopyOrMove&) = delete;
    NoCopyOrMove& operator=(NoCopyOrMove&&) = delete;

    operator bool() const { return i != 0; }

    int get_ip1() { return i + 1; }
    bool test()
    {
        // Check that Assume can be used within a lambda and still call methods
        [&]() { Assume(get_ip1()); }();
        return Assume(get_ip1() != 5);
    }
};
} // namespace

BOOST_AUTO_TEST_CASE(util_check)
{
    // Check that Assert can forward
    const std::unique_ptr<int> p_two = Assert(std::make_unique<int>(2));
    // Check that Assert works on lvalues and rvalues
    const int two = *Assert(p_two);
    Assert(two == 2);
    Assert(true);
    // Check that Assume can be used as unary expression
    const bool result{Assume(two == 2)};
    Assert(result);

    // Check that Assert doesn't require copy/move
    NoCopyOrMove x{9};
    Assert(x).i += 3;
    Assert(x).test();

    // Check nested Asserts
    BOOST_CHECK_EQUAL(Assert((Assert(x).test() ? 3 : 0)), 3);

    // Check -Wdangling-gsl does not trigger when copying the int. (It would
    // trigger on "const int&")
    const int nine{*Assert(std::optional<int>{9})};
    BOOST_CHECK_EQUAL(9, nine);
}

BOOST_AUTO_TEST_CASE(util_criticalsection)
{
    RecursiveMutex cs;

    do {
        LOCK(cs);
        break;

        BOOST_ERROR("break was swallowed!");
    } while(0);

    do {
        TRY_LOCK(cs, lockTest);
        if (lockTest) {
            BOOST_CHECK(true); // Needed to suppress "Test case [...] did not check any assertions"
            break;
        }

        BOOST_ERROR("break was swallowed!");
    } while(0);
}

constexpr char HEX_PARSE_INPUT[] = "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f";
constexpr uint8_t HEX_PARSE_OUTPUT[] = {
    0x04, 0x67, 0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19, 0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7,
    0x10, 0x5c, 0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79, 0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde,
    0xb6, 0x49, 0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3, 0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12,
    0xde, 0x5c, 0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a, 0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d,
    0x5f
};
static_assert((sizeof(HEX_PARSE_INPUT) - 1) == 2 * sizeof(HEX_PARSE_OUTPUT));
BOOST_AUTO_TEST_CASE(parse_hex)
{
    std::vector<unsigned char> result;

    // Basic test vector
    std::vector<unsigned char> expected(std::begin(HEX_PARSE_OUTPUT), std::end(HEX_PARSE_OUTPUT));
    constexpr std::array<std::byte, 65> hex_literal_array{operator""_hex<util::detail::Hex(HEX_PARSE_INPUT)>()};
    auto hex_literal_span{MakeUCharSpan(hex_literal_array)};
    BOOST_CHECK_EQUAL_COLLECTIONS(hex_literal_span.begin(), hex_literal_span.end(), expected.begin(), expected.end());

    const std::vector<std::byte> hex_literal_vector{operator""_hex_v<util::detail::Hex(HEX_PARSE_INPUT)>()};
    auto hex_literal_vec_span = MakeUCharSpan(hex_literal_vector);
    BOOST_CHECK_EQUAL_COLLECTIONS(hex_literal_vec_span.begin(), hex_literal_vec_span.end(), expected.begin(), expected.end());

    constexpr std::array<uint8_t, 65> hex_literal_array_uint8{operator""_hex_u8<util::detail::Hex(HEX_PARSE_INPUT)>()};
    BOOST_CHECK_EQUAL_COLLECTIONS(hex_literal_array_uint8.begin(), hex_literal_array_uint8.end(), expected.begin(), expected.end());

    result = operator""_hex_v_u8<util::detail::Hex(HEX_PARSE_INPUT)>();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    result = ParseHex(HEX_PARSE_INPUT);
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    result = TryParseHex<uint8_t>(HEX_PARSE_INPUT).value();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    // Spaces between bytes must be supported
    expected = {0x12, 0x34, 0x56, 0x78};
    result = ParseHex("12 34 56 78");
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    result = TryParseHex<uint8_t>("12 34 56 78").value();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    // Leading space must be supported
    expected = {0x89, 0x34, 0x56, 0x78};
    result = ParseHex(" 89 34 56 78");
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    result = TryParseHex<uint8_t>(" 89 34 56 78").value();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    // Mixed case and spaces are supported
    expected = {0xff, 0xaa};
    result = ParseHex("     Ff        aA    ");
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    result = TryParseHex<uint8_t>("     Ff        aA    ").value();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    // Empty string is supported
    static_assert(""_hex.empty());
    static_assert(""_hex_u8.empty());
    BOOST_CHECK_EQUAL(""_hex_v.size(), 0);
    BOOST_CHECK_EQUAL(""_hex_v_u8.size(), 0);
    BOOST_CHECK_EQUAL(ParseHex("").size(), 0);
    BOOST_CHECK_EQUAL(TryParseHex<uint8_t>("").value().size(), 0);

    // Spaces between nibbles is treated as invalid
    BOOST_CHECK_EQUAL(ParseHex("AAF F").size(), 0);
    BOOST_CHECK(!TryParseHex("AAF F").has_value());

    // Embedded null is treated as invalid
    const std::string with_embedded_null{" 11 "s
                                         " \0 "
                                         " 22 "s};
    BOOST_CHECK_EQUAL(with_embedded_null.size(), 11);
    BOOST_CHECK_EQUAL(ParseHex(with_embedded_null).size(), 0);
    BOOST_CHECK(!TryParseHex(with_embedded_null).has_value());

    // Non-hex is treated as invalid
    BOOST_CHECK_EQUAL(ParseHex("1234 invalid 1234").size(), 0);
    BOOST_CHECK(!TryParseHex("1234 invalid 1234").has_value());

    // Truncated input is treated as invalid
    BOOST_CHECK_EQUAL(ParseHex("12 3").size(), 0);
    BOOST_CHECK(!TryParseHex("12 3").has_value());
}

BOOST_AUTO_TEST_CASE(consteval_hex_digit)
{
    BOOST_CHECK_EQUAL(ConstevalHexDigit('0'), 0);
    BOOST_CHECK_EQUAL(ConstevalHexDigit('9'), 9);
    BOOST_CHECK_EQUAL(ConstevalHexDigit('a'), 0xa);
    BOOST_CHECK_EQUAL(ConstevalHexDigit('f'), 0xf);
}

BOOST_AUTO_TEST_CASE(util_HexStr)
{
    BOOST_CHECK_EQUAL(HexStr(HEX_PARSE_OUTPUT), HEX_PARSE_INPUT);
    BOOST_CHECK_EQUAL(HexStr(std::span{HEX_PARSE_OUTPUT}.last(0)), "");
    BOOST_CHECK_EQUAL(HexStr(std::span{HEX_PARSE_OUTPUT}.first(0)), "");

    {
        constexpr std::string_view out_exp{"04678afdb0"};
        constexpr std::span in_s{HEX_PARSE_OUTPUT, out_exp.size() / 2};
        const std::span<const uint8_t> in_u{MakeUCharSpan(in_s)};
        const std::span<const std::byte> in_b{MakeByteSpan(in_s)};

        BOOST_CHECK_EQUAL(HexStr(in_u), out_exp);
        BOOST_CHECK_EQUAL(HexStr(in_s), out_exp);
        BOOST_CHECK_EQUAL(HexStr(in_b), out_exp);
    }

    {
        auto input = std::string();
        for (size_t i=0; i<256; ++i) {
            input.push_back(static_cast<char>(i));
        }

        auto hex = HexStr(input);
        BOOST_TEST_REQUIRE(hex.size() == 512);
        static constexpr auto hexmap = std::string_view("0123456789abcdef");
        for (size_t i = 0; i < 256; ++i) {
            auto upper = hexmap.find(hex[i * 2]);
            auto lower = hexmap.find(hex[i * 2 + 1]);
            BOOST_TEST_REQUIRE(upper != std::string_view::npos);
            BOOST_TEST_REQUIRE(lower != std::string_view::npos);
            BOOST_TEST_REQUIRE(i == upper*16 + lower);
        }
    }
}

BOOST_AUTO_TEST_CASE(span_write_bytes)
{
    std::array mut_arr{uint8_t{0xaa}, uint8_t{0xbb}};
    const auto mut_bytes{MakeWritableByteSpan(mut_arr)};
    mut_bytes[1] = std::byte{0x11};
    BOOST_CHECK_EQUAL(mut_arr.at(0), 0xaa);
    BOOST_CHECK_EQUAL(mut_arr.at(1), 0x11);
}

BOOST_AUTO_TEST_CASE(util_Join)
{
    // Normal version
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{}, ", "), "");
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{"foo"}, ", "), "foo");
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{"foo", "bar"}, ", "), "foo, bar");

    // Version with unary operator
    const auto op_upper = [](const std::string& s) { return ToUpper(s); };
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{}, ", ", op_upper), "");
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{"foo"}, ", ", op_upper), "FOO");
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{"foo", "bar"}, ", ", op_upper), "FOO, BAR");
}

BOOST_AUTO_TEST_CASE(util_ReplaceAll)
{
    const std::string original("A test \"%s\" string '%s'.");
    auto test_replaceall = [&original](const std::string& search, const std::string& substitute, const std::string& expected) {
        auto test = original;
        ReplaceAll(test, search, substitute);
        BOOST_CHECK_EQUAL(test, expected);
    };

    test_replaceall("", "foo", original);
    test_replaceall(original, "foo", "foo");
    test_replaceall("%s", "foo", "A test \"foo\" string 'foo'.");
    test_replaceall("\"", "foo", "A test foo%sfoo string '%s'.");
    test_replaceall("'", "foo", "A test \"%s\" string foo%sfoo.");
}

BOOST_AUTO_TEST_CASE(util_TrimString)
{
    BOOST_CHECK_EQUAL(TrimString(" foo bar "), "foo bar");
    BOOST_CHECK_EQUAL(TrimStringView("\t \n  \n \f\n\r\t\v\tfoo \n \f\n\r\t\v\tbar\t  \n \f\n\r\t\v\t\n "), "foo \n \f\n\r\t\v\tbar");
    BOOST_CHECK_EQUAL(TrimString("\t \n foo \n\tbar\t \n "), "foo \n\tbar");
    BOOST_CHECK_EQUAL(TrimStringView("\t \n foo \n\tbar\t \n ", "fobar"), "\t \n foo \n\tbar\t \n ");
    BOOST_CHECK_EQUAL(TrimString("foo bar"), "foo bar");
    BOOST_CHECK_EQUAL(TrimStringView("foo bar", "fobar"), " ");
    BOOST_CHECK_EQUAL(TrimString(std::string("\0 foo \0 ", 8)), std::string("\0 foo \0", 7));
    BOOST_CHECK_EQUAL(TrimStringView(std::string(" foo ", 5)), std::string("foo", 3));
    BOOST_CHECK_EQUAL(TrimString(std::string("\t\t\0\0\n\n", 6)), std::string("\0\0", 2));
    BOOST_CHECK_EQUAL(TrimStringView(std::string("\x05\x04\x03\x02\x01\x00", 6)), std::string("\x05\x04\x03\x02\x01\x00", 6));
    BOOST_CHECK_EQUAL(TrimString(std::string("\x05\x04\x03\x02\x01\x00", 6), std::string("\x05\x04\x03\x02\x01", 5)), std::string("\0", 1));
    BOOST_CHECK_EQUAL(TrimStringView(std::string("\x05\x04\x03\x02\x01\x00", 6), std::string("\x05\x04\x03\x02\x01\x00", 6)), "");
}

BOOST_AUTO_TEST_CASE(util_ParseISO8601DateTime)
{
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1969-12-31T23:59:59Z").value(), -1);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1970-01-01T00:00:00Z").value(), 0);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1970-01-01T00:00:01Z").value(), 1);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T00:00:01Z").value(), 946684801);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2011-09-30T23:36:17Z").value(), 1317425777);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2100-12-31T23:59:59Z").value(), 4133980799);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("9999-12-31T23:59:59Z").value(), 253402300799);

    // Accept edge-cases, where the time overflows. They are not produced by
    // FormatISO8601DateTime, so this can be changed in the future, if needed.
    // For now, keep compatibility with the previous implementation.
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T99:00:00Z").value(), 947041200);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T00:99:00Z").value(), 946690740);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T00:00:99Z").value(), 946684899);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T99:99:99Z").value(), 947047239);

    // Reject date overflows.
    BOOST_CHECK(!ParseISO8601DateTime("2000-99-01T00:00:00Z"));
    BOOST_CHECK(!ParseISO8601DateTime("2000-01-99T00:00:00Z"));

    // Reject out-of-range years
    BOOST_CHECK(!ParseISO8601DateTime("32768-12-31T23:59:59Z"));
    BOOST_CHECK(!ParseISO8601DateTime("32767-12-31T23:59:59Z"));
    BOOST_CHECK(!ParseISO8601DateTime("32767-12-31T00:00:00Z"));
    BOOST_CHECK(!ParseISO8601DateTime("999-12-31T00:00:00Z"));

    // Reject invalid format
    const std::string valid{"2000-01-01T00:00:01Z"};
    BOOST_CHECK(ParseISO8601DateTime(valid).has_value());
    for (auto mut{0U}; mut < valid.size(); ++mut) {
        std::string invalid{valid};
        invalid[mut] = 'a';
        BOOST_CHECK(!ParseISO8601DateTime(invalid));
    }
}

BOOST_AUTO_TEST_CASE(util_FormatISO8601DateTime)
{
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(971890963199), "32767-12-31T23:59:59Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(971890876800), "32767-12-31T00:00:00Z");

    BOOST_CHECK_EQUAL(FormatISO8601DateTime(-1), "1969-12-31T23:59:59Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(0), "1970-01-01T00:00:00Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(1), "1970-01-01T00:00:01Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(946684801), "2000-01-01T00:00:01Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(1317425777), "2011-09-30T23:36:17Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(4133980799), "2100-12-31T23:59:59Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(253402300799), "9999-12-31T23:59:59Z");
}

BOOST_AUTO_TEST_CASE(util_FormatISO8601Date)
{
    BOOST_CHECK_EQUAL(FormatISO8601Date(971890963199), "32767-12-31");
    BOOST_CHECK_EQUAL(FormatISO8601Date(971890876800), "32767-12-31");

    BOOST_CHECK_EQUAL(FormatISO8601Date(0), "1970-01-01");
    BOOST_CHECK_EQUAL(FormatISO8601Date(1317425777), "2011-09-30");
}

BOOST_AUTO_TEST_CASE(util_FormatMoney)
{
    BOOST_CHECK_EQUAL(FormatMoney(0), "0.00");
    BOOST_CHECK_EQUAL(FormatMoney((COIN/10000)*123456789), "12345.6789");
    BOOST_CHECK_EQUAL(FormatMoney(-COIN), "-1.00");

    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000000), "100000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000000), "10000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000000), "1000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000), "100000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000), "10000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000), "1000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100), "100.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10), "10.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN), "1.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10), "0.10");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100), "0.01");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000), "0.001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000), "0.0001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000), "0.00001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000000), "0.000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000000), "0.0000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000000), "0.00000001");

    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max()), "92233720368.54775807");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 1), "92233720368.54775806");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 2), "92233720368.54775805");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 3), "92233720368.54775804");
    // ...
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 3), "-92233720368.54775805");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 2), "-92233720368.54775806");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 1), "-92233720368.54775807");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min()), "-92233720368.54775808");
}

BOOST_AUTO_TEST_CASE(util_ParseMoney)
{
    BOOST_CHECK_EQUAL(ParseMoney("0.0").value(), 0);
    BOOST_CHECK_EQUAL(ParseMoney(".").value(), 0);
    BOOST_CHECK_EQUAL(ParseMoney("0.").value(), 0);
    BOOST_CHECK_EQUAL(ParseMoney(".0").value(), 0);
    BOOST_CHECK_EQUAL(ParseMoney(".6789").value(), 6789'0000);
    BOOST_CHECK_EQUAL(ParseMoney("12345.").value(), COIN * 12345);

    BOOST_CHECK_EQUAL(ParseMoney("12345.6789").value(), (COIN/10000)*123456789);

    BOOST_CHECK_EQUAL(ParseMoney("10000000.00").value(), COIN*10000000);
    BOOST_CHECK_EQUAL(ParseMoney("1000000.00").value(), COIN*1000000);
    BOOST_CHECK_EQUAL(ParseMoney("100000.00").value(), COIN*100000);
    BOOST_CHECK_EQUAL(ParseMoney("10000.00").value(), COIN*10000);
    BOOST_CHECK_EQUAL(ParseMoney("1000.00").value(), COIN*1000);
    BOOST_CHECK_EQUAL(ParseMoney("100.00").value(), COIN*100);
    BOOST_CHECK_EQUAL(ParseMoney("10.00").value(), COIN*10);
    BOOST_CHECK_EQUAL(ParseMoney("1.00").value(), COIN);
    BOOST_CHECK_EQUAL(ParseMoney("1").value(), COIN);
    BOOST_CHECK_EQUAL(ParseMoney("   1").value(), COIN);
    BOOST_CHECK_EQUAL(ParseMoney("1   ").value(), COIN);
    BOOST_CHECK_EQUAL(ParseMoney("  1 ").value(), COIN);
    BOOST_CHECK_EQUAL(ParseMoney("0.1").value(), COIN/10);
    BOOST_CHECK_EQUAL(ParseMoney("0.01").value(), COIN/100);
    BOOST_CHECK_EQUAL(ParseMoney("0.001").value(), COIN/1000);
    BOOST_CHECK_EQUAL(ParseMoney("0.0001").value(), COIN/10000);
    BOOST_CHECK_EQUAL(ParseMoney("0.00001").value(), COIN/100000);
    BOOST_CHECK_EQUAL(ParseMoney("0.000001").value(), COIN/1000000);
    BOOST_CHECK_EQUAL(ParseMoney("0.0000001").value(), COIN/10000000);
    BOOST_CHECK_EQUAL(ParseMoney("0.00000001").value(), COIN/100000000);
    BOOST_CHECK_EQUAL(ParseMoney(" 0.00000001 ").value(), COIN/100000000);
    BOOST_CHECK_EQUAL(ParseMoney("0.00000001 ").value(), COIN/100000000);
    BOOST_CHECK_EQUAL(ParseMoney(" 0.00000001").value(), COIN/100000000);

    // Parsing amount that cannot be represented should fail
    BOOST_CHECK(!ParseMoney("100000000.00"));
    BOOST_CHECK(!ParseMoney("0.000000001"));

    // Parsing empty string should fail
    BOOST_CHECK(!ParseMoney(""));
    BOOST_CHECK(!ParseMoney(" "));
    BOOST_CHECK(!ParseMoney("  "));

    // Parsing two numbers should fail
    BOOST_CHECK(!ParseMoney(".."));
    BOOST_CHECK(!ParseMoney("0..0"));
    BOOST_CHECK(!ParseMoney("1 2"));
    BOOST_CHECK(!ParseMoney(" 1 2 "));
    BOOST_CHECK(!ParseMoney(" 1.2 3 "));
    BOOST_CHECK(!ParseMoney(" 1 2.3 "));

    // Embedded whitespace should fail
    BOOST_CHECK(!ParseMoney(" -1 .2  "));
    BOOST_CHECK(!ParseMoney("  1 .2  "));
    BOOST_CHECK(!ParseMoney(" +1 .2  "));

    // Attempted 63 bit overflow should fail
    BOOST_CHECK(!ParseMoney("92233720368.54775808"));

    // Parsing negative amounts must fail
    BOOST_CHECK(!ParseMoney("-1"));

    // Parsing strings with embedded NUL characters should fail
    BOOST_CHECK(!ParseMoney("\0-1"s));
    BOOST_CHECK(!ParseMoney(STRING_WITH_EMBEDDED_NULL_CHAR));
    BOOST_CHECK(!ParseMoney("1\0"s));
}

BOOST_AUTO_TEST_CASE(util_IsHex)
{
    BOOST_CHECK(IsHex("00"));
    BOOST_CHECK(IsHex("00112233445566778899aabbccddeeffAABBCCDDEEFF"));
    BOOST_CHECK(IsHex("ff"));
    BOOST_CHECK(IsHex("FF"));

    BOOST_CHECK(!IsHex(""));
    BOOST_CHECK(!IsHex("0"));
    BOOST_CHECK(!IsHex("a"));
    BOOST_CHECK(!IsHex("eleven"));
    BOOST_CHECK(!IsHex("00xx00"));
    BOOST_CHECK(!IsHex("0x0000"));
}

BOOST_AUTO_TEST_CASE(util_seed_insecure_rand)
{
    SeedRandomForTest(SeedRand::ZEROS);
    for (int mod=2;mod<11;mod++)
    {
        int mask = 1;
        // Really rough binomial confidence approximation.
        int err = 30*10000./mod*sqrt((1./mod*(1-1./mod))/10000.);
        //mask is 2^ceil(log2(mod))-1
        while(mask<mod-1)mask=(mask<<1)+1;

        int count = 0;
        //How often does it get a zero from the uniform range [0,mod)?
        for (int i = 0; i < 10000; i++) {
            uint32_t rval;
            do{
                rval=m_rng.rand32()&mask;
            }while(rval>=(uint32_t)mod);
            count += rval==0;
        }
        BOOST_CHECK(count<=10000/mod+err);
        BOOST_CHECK(count>=10000/mod-err);
    }
}

BOOST_AUTO_TEST_CASE(util_TimingResistantEqual)
{
    BOOST_CHECK(TimingResistantEqual(std::string(""), std::string("")));
    BOOST_CHECK(!TimingResistantEqual(std::string("abc"), std::string("")));
    BOOST_CHECK(!TimingResistantEqual(std::string(""), std::string("abc")));
    BOOST_CHECK(!TimingResistantEqual(std::string("a"), std::string("aa")));
    BOOST_CHECK(!TimingResistantEqual(std::string("aa"), std::string("a")));
    BOOST_CHECK(TimingResistantEqual(std::string("abc"), std::string("abc")));
    BOOST_CHECK(!TimingResistantEqual(std::string("abc"), std::string("aba")));
}

/* Test strprintf formatting directives.
 * Put a string before and after to ensure sanity of element sizes on stack. */
#define B "check_prefix"
#define E "check_postfix"
BOOST_AUTO_TEST_CASE(strprintf_numbers)
{
    int64_t s64t = -9223372036854775807LL; /* signed 64 bit test value */
    uint64_t u64t = 18446744073709551615ULL; /* unsigned 64 bit test value */
    BOOST_CHECK(strprintf("%s %d %s", B, s64t, E) == B" -9223372036854775807 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, u64t, E) == B" 18446744073709551615 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, u64t, E) == B" ffffffffffffffff " E);

    size_t st = 12345678; /* unsigned size_t test value */
    ssize_t sst = -12345678; /* signed size_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, sst, E) == B" -12345678 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, st, E) == B" 12345678 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, st, E) == B" bc614e " E);

    ptrdiff_t pt = 87654321; /* positive ptrdiff_t test value */
    ptrdiff_t spt = -87654321; /* negative ptrdiff_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, spt, E) == B" -87654321 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, pt, E) == B" 87654321 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, pt, E) == B" 5397fb1 " E);
}
#undef B
#undef E

BOOST_AUTO_TEST_CASE(util_mocktime)
{
    SetMockTime(111s);
    // Check that mock time does not change after a sleep
    for (const auto& num_sleep : {0ms, 1ms}) {
        UninterruptibleSleep(num_sleep);
        BOOST_CHECK_EQUAL(111, GetTime()); // Deprecated time getter
        BOOST_CHECK_EQUAL(111, Now<NodeSeconds>().time_since_epoch().count());
        BOOST_CHECK_EQUAL(111, TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()));
        BOOST_CHECK_EQUAL(111, TicksSinceEpoch<SecondsDouble>(Now<NodeSeconds>()));
        BOOST_CHECK_EQUAL(111, GetTime<std::chrono::seconds>().count());
        BOOST_CHECK_EQUAL(111000, GetTime<std::chrono::milliseconds>().count());
        BOOST_CHECK_EQUAL(111000, TicksSinceEpoch<std::chrono::milliseconds>(NodeClock::now()));
        BOOST_CHECK_EQUAL(111000000, GetTime<std::chrono::microseconds>().count());
    }
    SetMockTime(0s);
}

BOOST_AUTO_TEST_CASE(test_IsDigit)
{
    BOOST_CHECK_EQUAL(IsDigit('0'), true);
    BOOST_CHECK_EQUAL(IsDigit('1'), true);
    BOOST_CHECK_EQUAL(IsDigit('8'), true);
    BOOST_CHECK_EQUAL(IsDigit('9'), true);

    BOOST_CHECK_EQUAL(IsDigit('0' - 1), false);
    BOOST_CHECK_EQUAL(IsDigit('9' + 1), false);
    BOOST_CHECK_EQUAL(IsDigit(0), false);
    BOOST_CHECK_EQUAL(IsDigit(1), false);
    BOOST_CHECK_EQUAL(IsDigit(8), false);
    BOOST_CHECK_EQUAL(IsDigit(9), false);
}

/* Check for overflow */
template <typename T>
static void TestAddMatrixOverflow()
{
    constexpr T MAXI{std::numeric_limits<T>::max()};
    BOOST_CHECK(!CheckedAdd(T{1}, MAXI));
    BOOST_CHECK(!CheckedAdd(MAXI, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{1}, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(MAXI, MAXI));

    BOOST_CHECK_EQUAL(0, CheckedAdd(T{0}, T{0}).value());
    BOOST_CHECK_EQUAL(MAXI, CheckedAdd(T{0}, MAXI).value());
    BOOST_CHECK_EQUAL(MAXI, CheckedAdd(T{1}, MAXI - 1).value());
    BOOST_CHECK_EQUAL(MAXI - 1, CheckedAdd(T{1}, MAXI - 2).value());
    BOOST_CHECK_EQUAL(0, SaturatingAdd(T{0}, T{0}));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{0}, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{1}, MAXI - 1));
    BOOST_CHECK_EQUAL(MAXI - 1, SaturatingAdd(T{1}, MAXI - 2));
}

/* Check for overflow or underflow */
template <typename T>
static void TestAddMatrix()
{
    TestAddMatrixOverflow<T>();
    constexpr T MINI{std::numeric_limits<T>::min()};
    constexpr T MAXI{std::numeric_limits<T>::max()};
    BOOST_CHECK(!CheckedAdd(T{-1}, MINI));
    BOOST_CHECK(!CheckedAdd(MINI, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{-1}, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(MINI, MINI));

    BOOST_CHECK_EQUAL(MINI, CheckedAdd(T{0}, MINI).value());
    BOOST_CHECK_EQUAL(MINI, CheckedAdd(T{-1}, MINI + 1).value());
    BOOST_CHECK_EQUAL(-1, CheckedAdd(MINI, MAXI).value());
    BOOST_CHECK_EQUAL(MINI + 1, CheckedAdd(T{-1}, MINI + 2).value());
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{0}, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{-1}, MINI + 1));
    BOOST_CHECK_EQUAL(MINI + 1, SaturatingAdd(T{-1}, MINI + 2));
    BOOST_CHECK_EQUAL(-1, SaturatingAdd(MINI, MAXI));
}

BOOST_AUTO_TEST_CASE(util_overflow)
{
    TestAddMatrixOverflow<unsigned>();
    TestAddMatrix<signed>();
}

template <typename T>
static void RunToIntegralTests()
{
    BOOST_CHECK(!ToIntegral<T>(STRING_WITH_EMBEDDED_NULL_CHAR));
    BOOST_CHECK(!ToIntegral<T>(" 1"));
    BOOST_CHECK(!ToIntegral<T>("1 "));
    BOOST_CHECK(!ToIntegral<T>("1a"));
    BOOST_CHECK(!ToIntegral<T>("1.1"));
    BOOST_CHECK(!ToIntegral<T>("1.9"));
    BOOST_CHECK(!ToIntegral<T>("+01.9"));
    BOOST_CHECK(!ToIntegral<T>("-"));
    BOOST_CHECK(!ToIntegral<T>("+"));
    BOOST_CHECK(!ToIntegral<T>(" -1"));
    BOOST_CHECK(!ToIntegral<T>("-1 "));
    BOOST_CHECK(!ToIntegral<T>(" -1 "));
    BOOST_CHECK(!ToIntegral<T>("+1"));
    BOOST_CHECK(!ToIntegral<T>(" +1"));
    BOOST_CHECK(!ToIntegral<T>(" +1 "));
    BOOST_CHECK(!ToIntegral<T>("+-1"));
    BOOST_CHECK(!ToIntegral<T>("-+1"));
    BOOST_CHECK(!ToIntegral<T>("++1"));
    BOOST_CHECK(!ToIntegral<T>("--1"));
    BOOST_CHECK(!ToIntegral<T>(""));
    BOOST_CHECK(!ToIntegral<T>("aap"));
    BOOST_CHECK(!ToIntegral<T>("0x1"));
    BOOST_CHECK(!ToIntegral<T>("-32482348723847471234"));
    BOOST_CHECK(!ToIntegral<T>("32482348723847471234"));
}

BOOST_AUTO_TEST_CASE(test_ToIntegral)
{
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("1234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("0").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("01234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("00000000000000001234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-00000000000000001234").value(), -1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("00000000000000000000").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-00000000000000000000").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-1234").value(), -1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-1").value(), -1);

    RunToIntegralTests<uint64_t>();
    RunToIntegralTests<int64_t>();
    RunToIntegralTests<uint32_t>();
    RunToIntegralTests<int32_t>();
    RunToIntegralTests<uint16_t>();
    RunToIntegralTests<int16_t>();
    RunToIntegralTests<uint8_t>();
    RunToIntegralTests<int8_t>();

    BOOST_CHECK(!ToIntegral<int64_t>("-9223372036854775809"));
    BOOST_CHECK_EQUAL(ToIntegral<int64_t>("-9223372036854775808").value(), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(ToIntegral<int64_t>("9223372036854775807").value(), 9'223'372'036'854'775'807);
    BOOST_CHECK(!ToIntegral<int64_t>("9223372036854775808"));

    BOOST_CHECK(!ToIntegral<uint64_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint64_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint64_t>("18446744073709551615").value(), 18'446'744'073'709'551'615ULL);
    BOOST_CHECK(!ToIntegral<uint64_t>("18446744073709551616"));

    BOOST_CHECK(!ToIntegral<int32_t>("-2147483649"));
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-2147483648").value(), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("2147483647").value(), 2'147'483'647);
    BOOST_CHECK(!ToIntegral<int32_t>("2147483648"));

    BOOST_CHECK(!ToIntegral<uint32_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint32_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint32_t>("4294967295").value(), 4'294'967'295U);
    BOOST_CHECK(!ToIntegral<uint32_t>("4294967296"));

    BOOST_CHECK(!ToIntegral<int16_t>("-32769"));
    BOOST_CHECK_EQUAL(ToIntegral<int16_t>("-32768").value(), -32'768);
    BOOST_CHECK_EQUAL(ToIntegral<int16_t>("32767").value(), 32'767);
    BOOST_CHECK(!ToIntegral<int16_t>("32768"));

    BOOST_CHECK(!ToIntegral<uint16_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint16_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint16_t>("65535").value(), 65'535U);
    BOOST_CHECK(!ToIntegral<uint16_t>("65536"));

    BOOST_CHECK(!ToIntegral<int8_t>("-129"));
    BOOST_CHECK_EQUAL(ToIntegral<int8_t>("-128").value(), -128);
    BOOST_CHECK_EQUAL(ToIntegral<int8_t>("127").value(), 127);
    BOOST_CHECK(!ToIntegral<int8_t>("128"));

    BOOST_CHECK(!ToIntegral<uint8_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint8_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint8_t>("255").value(), 255U);
    BOOST_CHECK(!ToIntegral<uint8_t>("256"));
}

int64_t atoi64_legacy(const std::string& str)
{
    return strtoll(str.c_str(), nullptr, 10);
}

BOOST_AUTO_TEST_CASE(test_LocaleIndependentAtoi)
{
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1234"), 1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("0"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("01234"), 1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1234"), -1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" 1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1 "), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1a"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1.1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1.9"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+01.9"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1"), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" -1"), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1 "), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" -1 "), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" +1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" +1 "), 1);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+-1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-+1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("++1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("--1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(""), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("aap"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("0x1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-32482348723847471234"), -2'147'483'647 - 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("32482348723847471234"), 2'147'483'647);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("-9223372036854775809"), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("-9223372036854775808"), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("9223372036854775807"), 9'223'372'036'854'775'807);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("9223372036854775808"), 9'223'372'036'854'775'807);

    std::map<std::string, int64_t> atoi64_test_pairs = {
        {"-9223372036854775809", std::numeric_limits<int64_t>::min()},
        {"-9223372036854775808", -9'223'372'036'854'775'807LL - 1LL},
        {"9223372036854775807", 9'223'372'036'854'775'807},
        {"9223372036854775808", std::numeric_limits<int64_t>::max()},
        {"+-", 0},
        {"0x1", 0},
        {"ox1", 0},
        {"", 0},
    };

    for (const auto& pair : atoi64_test_pairs) {
        BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>(pair.first), pair.second);
    }

    // Ensure legacy compatibility with previous versions of Bitcoin Core's atoi64
    for (const auto& pair : atoi64_test_pairs) {
        BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>(pair.first), atoi64_legacy(pair.first));
    }

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("18446744073709551615"), 18'446'744'073'709'551'615ULL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("18446744073709551616"), 18'446'744'073'709'551'615ULL);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-2147483649"), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-2147483648"), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("2147483647"), 2'147'483'647);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("2147483648"), 2'147'483'647);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("4294967295"), 4'294'967'295U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("4294967296"), 4'294'967'295U);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("-32769"), -32'768);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("-32768"), -32'768);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("32767"), 32'767);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("32768"), 32'767);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("65535"), 65'535U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("65536"), 65'535U);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("-129"), -128);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("-128"), -128);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("127"), 127);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("128"), 127);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("255"), 255U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("256"), 255U);
}

BOOST_AUTO_TEST_CASE(test_FormatParagraph)
{
    BOOST_CHECK_EQUAL(FormatParagraph("", 79, 0), "");
    BOOST_CHECK_EQUAL(FormatParagraph("test", 79, 0), "test");
    BOOST_CHECK_EQUAL(FormatParagraph(" test", 79, 0), " test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 79, 0), "test test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 0), "test\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("testerde test", 4, 0), "testerde\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 4), "test\n    test");

    // Make sure we don't indent a fully-new line following a too-long line ending
    BOOST_CHECK_EQUAL(FormatParagraph("test test\nabc", 4, 4), "test\n    test\nabc");

    BOOST_CHECK_EQUAL(FormatParagraph("This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length until it gets here", 79), "This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length\nuntil it gets here");

    // Test wrap length is exact
    BOOST_CHECK_EQUAL(FormatParagraph("a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    // Indent should be included in length of lines
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg h i j k", 79, 4), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\n    f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg\n    h i j k");

    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string. This is a second sentence in the very long test string.", 79), "This is a very long test string. This is a second sentence in the very long\ntest string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("Testing that normal newlines do not get indented.\nLike here.", 79), "Testing that normal newlines do not get indented.\nLike here.");
}

BOOST_AUTO_TEST_CASE(test_FormatSubVersion)
{
    std::vector<std::string> comments;
    comments.emplace_back("comment1");
    std::vector<std::string> comments2;
    comments2.emplace_back("comment1");
    comments2.push_back(SanitizeString(std::string("Comment2; .,_?@-; !\"#$%&'()*+/<=>[]\\^`{|}~"), SAFE_CHARS_UA_COMMENT)); // Semicolon is discouraged but not forbidden by BIP-0014
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, std::vector<std::string>()),std::string("/Test:9.99.0/"));
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, comments),std::string("/Test:9.99.0(comment1)/"));
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, comments2),std::string("/Test:9.99.0(comment1; Comment2; .,_?@-; )/"));
}

BOOST_AUTO_TEST_CASE(test_ParseFixedPoint)
{
    int64_t amount = 0;
    BOOST_CHECK(ParseFixedPoint("0", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 0LL);
    BOOST_CHECK(ParseFixedPoint("1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000LL);
    BOOST_CHECK(ParseFixedPoint("0.0", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 0LL);
    BOOST_CHECK(ParseFixedPoint("-0.1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -10000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 110000000LL);
    BOOST_CHECK(ParseFixedPoint("1.10000000000000000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 110000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1e1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1100000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1e-1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 11000000LL);
    BOOST_CHECK(ParseFixedPoint("1000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000000LL);
    BOOST_CHECK(ParseFixedPoint("-1000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -100000000000LL);
    BOOST_CHECK(ParseFixedPoint("0.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1LL);
    BOOST_CHECK(ParseFixedPoint("0.0000000100000000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1LL);
    BOOST_CHECK(ParseFixedPoint("-0.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -1LL);
    BOOST_CHECK(ParseFixedPoint("1000000000.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000000000001LL);
    BOOST_CHECK(ParseFixedPoint("9999999999.99999999", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 999999999999999999LL);
    BOOST_CHECK(ParseFixedPoint("-9999999999.99999999", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -999999999999999999LL);

    BOOST_CHECK(!ParseFixedPoint("", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("a-1000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-a1000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-1000a", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-01000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("00.1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint(".1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("--0.1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("0.000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-0.000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("0.00000001000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000009", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000009", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-99999999999.99999999", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("99999909999.09999999", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("92233720368.54775807", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("92233720368.54775808", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-92233720368.54775808", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-92233720368.54775809", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.1e", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.1e-", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.", 8, &amount));

    // Test with 3 decimal places for fee rates in sat/vB.
    BOOST_CHECK(ParseFixedPoint("0.001", 3, &amount));
    BOOST_CHECK_EQUAL(amount, CAmount{1});
    BOOST_CHECK(!ParseFixedPoint("0.0009", 3, &amount));
    BOOST_CHECK(!ParseFixedPoint("31.00100001", 3, &amount));
    BOOST_CHECK(!ParseFixedPoint("31.0011", 3, &amount));
    BOOST_CHECK(!ParseFixedPoint("31.99999999", 3, &amount));
    BOOST_CHECK(!ParseFixedPoint("31.999999999999999999999", 3, &amount));
}

#ifndef WIN32 // Cannot do this test on WIN32 due to lack of fork()
static constexpr char LockCommand = 'L';
static constexpr char UnlockCommand = 'U';
static constexpr char ExitCommand = 'X';
enum : char {
    ResSuccess = 2, // Start with 2 to avoid accidental collision with common values 0 and 1
    ResErrorWrite,
    ResErrorLock,
    ResUnlockSuccess,
};

[[noreturn]] static void TestOtherProcess(fs::path dirname, fs::path lockname, int fd)
{
    char ch;
    while (true) {
        int rv = read(fd, &ch, 1); // Wait for command
        assert(rv == 1);
        switch (ch) {
        case LockCommand:
            ch = [&] {
                switch (util::LockDirectory(dirname, lockname)) {
                case util::LockResult::Success: return ResSuccess;
                case util::LockResult::ErrorWrite: return ResErrorWrite;
                case util::LockResult::ErrorLock: return ResErrorLock;
                } // no default case, so the compiler can warn about missing cases
                assert(false);
            }();
            rv = write(fd, &ch, 1);
            assert(rv == 1);
            break;
        case UnlockCommand:
            ReleaseDirectoryLocks();
            ch = ResUnlockSuccess; // Always succeeds
            rv = write(fd, &ch, 1);
            assert(rv == 1);
            break;
        case ExitCommand:
            close(fd);
            exit(0);
        default:
            assert(0);
        }
    }
}
#endif

BOOST_AUTO_TEST_CASE(test_LockDirectory)
{
    fs::path dirname = m_args.GetDataDirBase() / "lock_dir";
    const fs::path lockname = ".lock";
#ifndef WIN32
    // Fork another process for testing before creating the lock, so that we
    // won't fork while holding the lock (which might be undefined, and is not
    // relevant as test case as that is avoided with -daemonize).
    int fd[2];
    BOOST_CHECK_EQUAL(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
    pid_t pid = fork();
    if (!pid) {
        BOOST_CHECK_EQUAL(close(fd[1]), 0); // Child: close parent end
        TestOtherProcess(dirname, lockname, fd[0]);
    }
    BOOST_CHECK_EQUAL(close(fd[0]), 0); // Parent: close child end

    char ch;
    // Lock on non-existent directory should fail
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL(ch, ResErrorWrite);
#endif
    // Lock on non-existent directory should fail
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname), util::LockResult::ErrorWrite);

    fs::create_directories(dirname);

    // Probing lock on new directory should succeed
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname, true), util::LockResult::Success);

    // Persistent lock on new directory should succeed
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname), util::LockResult::Success);

    // Another lock on the directory from the same thread should succeed
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname), util::LockResult::Success);

    // Another lock on the directory from a different thread within the same process should succeed
    util::LockResult threadresult;
    std::thread thr([&] { threadresult = util::LockDirectory(dirname, lockname); });
    thr.join();
    BOOST_CHECK_EQUAL(threadresult, util::LockResult::Success);
#ifndef WIN32
    // Try to acquire lock in child process while we're holding it, this should fail.
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL(ch, ResErrorLock);

    // Give up our lock
    ReleaseDirectoryLocks();
    // Probing lock from our side now should succeed, but not hold on to the lock.
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname, true), util::LockResult::Success);

    // Try to acquire the lock in the child process, this should be successful.
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL(ch, ResSuccess);

    // When we try to probe the lock now, it should fail.
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname, true), util::LockResult::ErrorLock);

    // Unlock the lock in the child process
    BOOST_CHECK_EQUAL(write(fd[1], &UnlockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL(ch, ResUnlockSuccess);

    // When we try to probe the lock now, it should succeed.
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname, true), util::LockResult::Success);

    // Re-lock the lock in the child process, then wait for it to exit, check
    // successful return. After that, we check that exiting the process
    // has released the lock as we would expect by probing it.
    int processstatus;
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    // The following line invokes the ~CNetCleanup dtor without
    // a paired SetupNetworking call. This is acceptable as long as
    // ~CNetCleanup is a no-op for non-Windows platforms.
    BOOST_CHECK_EQUAL(write(fd[1], &ExitCommand, 1), 1);
    BOOST_CHECK_EQUAL(waitpid(pid, &processstatus, 0), pid);
    BOOST_CHECK_EQUAL(processstatus, 0);
    BOOST_CHECK_EQUAL(util::LockDirectory(dirname, lockname, true), util::LockResult::Success);

    BOOST_CHECK_EQUAL(close(fd[1]), 0); // Close our side of the socketpair
#endif
    // Clean up
    ReleaseDirectoryLocks();
    fs::remove_all(dirname);
}

BOOST_AUTO_TEST_CASE(test_ToLower)
{
    BOOST_CHECK_EQUAL(ToLower('@'), '@');
    BOOST_CHECK_EQUAL(ToLower('A'), 'a');
    BOOST_CHECK_EQUAL(ToLower('Z'), 'z');
    BOOST_CHECK_EQUAL(ToLower('['), '[');
    BOOST_CHECK_EQUAL(ToLower(0), 0);
    BOOST_CHECK_EQUAL(ToLower('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToLower(""), "");
    BOOST_CHECK_EQUAL(ToLower("#HODL"), "#hodl");
    BOOST_CHECK_EQUAL(ToLower("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_ToUpper)
{
    BOOST_CHECK_EQUAL(ToUpper('`'), '`');
    BOOST_CHECK_EQUAL(ToUpper('a'), 'A');
    BOOST_CHECK_EQUAL(ToUpper('z'), 'Z');
    BOOST_CHECK_EQUAL(ToUpper('{'), '{');
    BOOST_CHECK_EQUAL(ToUpper(0), 0);
    BOOST_CHECK_EQUAL(ToUpper('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToUpper(""), "");
    BOOST_CHECK_EQUAL(ToUpper("#hodl"), "#HODL");
    BOOST_CHECK_EQUAL(ToUpper("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_Capitalize)
{
    BOOST_CHECK_EQUAL(Capitalize(""), "");
    BOOST_CHECK_EQUAL(Capitalize("bitcoin"), "Bitcoin");
    BOOST_CHECK_EQUAL(Capitalize("\x00\xfe\xff"), "\x00\xfe\xff");
}

static std::string SpanToStr(const std::span<const char>& span)
{
    return std::string(span.begin(), span.end());
}

BOOST_AUTO_TEST_CASE(test_script_parsing)
{
    using namespace script;
    std::string input;
    std::span<const char> sp;
    bool success;

    // Const(...): parse a constant, update span to skip it if successful
    input = "MilkToastHoney";
    sp = input;
    success = Const("", sp); // empty
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "MilkToastHoney");

    success = Const("Milk", sp, /*skip=*/false);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "MilkToastHoney");

    success = Const("Milk", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "ToastHoney");

    success = Const("Bread", sp, /*skip=*/false);
    BOOST_CHECK(!success);

    success = Const("Bread", sp);
    BOOST_CHECK(!success);

    success = Const("Toast", sp, /*skip=*/false);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "ToastHoney");

    success = Const("Toast", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Honey");

    success = Const("Honeybadger", sp);
    BOOST_CHECK(!success);

    success = Const("Honey", sp, /*skip=*/false);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Honey");

    success = Const("Honey", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");
    // Func(...): parse a function call, update span to argument if successful
    input = "Foo(Bar(xy,z()))";
    sp = input;

    success = Func("FooBar", sp);
    BOOST_CHECK(!success);

    success = Func("Foo(", sp);
    BOOST_CHECK(!success);

    success = Func("Foo", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Bar(xy,z())");

    success = Func("Bar", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "xy,z()");

    success = Func("xy", sp);
    BOOST_CHECK(!success);

    // Expr(...): return expression that span begins with, update span to skip it
    std::span<const char> result;

    input = "(n*(n-1))/2";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(n*(n-1))/2");
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");

    input = "foo,bar";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",bar");

    input = "(aaaaa,bbbbb()),c";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(aaaaa,bbbbb())");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",c");

    input = "xyz)foo";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "xyz");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ")foo");

    input = "((a),(b),(c)),xxx";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "((a),(b),(c))");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",xxx");

    // Split(...): split a string on every instance of sep, return vector
    std::vector<std::span<const char>> results;

    input = "xxx";
    results = Split(input, 'x');
    BOOST_CHECK_EQUAL(results.size(), 4U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");

    input = "one#two#three";
    results = Split(input, '-');
    BOOST_CHECK_EQUAL(results.size(), 1U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one#two#three");

    input = "one#two#three";
    results = Split(input, '#');
    BOOST_CHECK_EQUAL(results.size(), 3U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "two");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "three");

    results = Split(input, '#', /*include_sep=*/true);
    BOOST_CHECK_EQUAL(results.size(), 3U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one#");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "two#");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "three");

    input = "*foo*bar*";
    results = Split(input, '*');
    BOOST_CHECK_EQUAL(results.size(), 4U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "bar");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");

    results = Split(input, '*', /*include_sep=*/true);
    BOOST_CHECK_EQUAL(results.size(), 4U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "*");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "foo*");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "bar*");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");
}

BOOST_AUTO_TEST_CASE(test_SplitString)
{
    // Empty string.
    {
        std::vector<std::string> result = SplitString("", '-');
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(result[0], "");
    }

    // Empty items.
    {
        std::vector<std::string> result = SplitString("-", '-');
        BOOST_CHECK_EQUAL(result.size(), 2);
        BOOST_CHECK_EQUAL(result[0], "");
        BOOST_CHECK_EQUAL(result[1], "");
    }

    // More empty items.
    {
        std::vector<std::string> result = SplitString("--", '-');
        BOOST_CHECK_EQUAL(result.size(), 3);
        BOOST_CHECK_EQUAL(result[0], "");
        BOOST_CHECK_EQUAL(result[1], "");
        BOOST_CHECK_EQUAL(result[2], "");
    }

    // Separator is not present.
    {
        std::vector<std::string> result = SplitString("abc", '-');
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(result[0], "abc");
    }

    // Basic behavior.
    {
        std::vector<std::string> result = SplitString("a-b", '-');
        BOOST_CHECK_EQUAL(result.size(), 2);
        BOOST_CHECK_EQUAL(result[0], "a");
        BOOST_CHECK_EQUAL(result[1], "b");
    }

    // Case-sensitivity of the separator.
    {
        std::vector<std::string> result = SplitString("AAA", 'a');
        BOOST_CHECK_EQUAL(result.size(), 1);
        BOOST_CHECK_EQUAL(result[0], "AAA");
    }

    // multiple split characters
    {
        using V = std::vector<std::string>;
        BOOST_TEST(SplitString("a,b.c:d;e", ",;") == V({"a", "b.c:d", "e"}));
        BOOST_TEST(SplitString("a,b.c:d;e", ",;:.") == V({"a", "b", "c", "d", "e"}));
        BOOST_TEST(SplitString("a,b.c:d;e", "") == V({"a,b.c:d;e"}));
        BOOST_TEST(SplitString("aaa", "bcdefg") == V({"aaa"}));
        BOOST_TEST(SplitString("x\0a,b"s, "\0"s) == V({"x", "a,b"}));
        BOOST_TEST(SplitString("x\0a,b"s, '\0') == V({"x", "a,b"}));
        BOOST_TEST(SplitString("x\0a,b"s, "\0,"s) == V({"x", "a", "b"}));
        BOOST_TEST(SplitString("abcdefg", "bcd") == V({"a", "", "", "efg"}));
    }
}

BOOST_AUTO_TEST_CASE(test_LogEscapeMessage)
{
    // ASCII and UTF-8 must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Valid log message貓"), "Valid log message貓");
    // Newlines must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Message\n with newlines\n"), "Message\n with newlines\n");
    // Other control characters are escaped in C syntax.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("\x01\x7f Corrupted log message\x0d"), R"(\x01\x7f Corrupted log message\x0d)");
    // Embedded NULL characters are escaped too.
    const std::string NUL("O\x00O", 3);
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage(NUL), R"(O\x00O)");
}

namespace {

struct Tracker
{
    //! Points to the original object (possibly itself) we moved/copied from
    const Tracker* origin;
    //! How many copies where involved between the original object and this one (moves are not counted)
    int copies{0};

    Tracker() noexcept : origin(this) {}
    Tracker(const Tracker& t) noexcept : origin(t.origin), copies(t.copies + 1) {}
    Tracker(Tracker&& t) noexcept : origin(t.origin), copies(t.copies) {}
    Tracker& operator=(const Tracker& t) noexcept
    {
        if (this != &t) {
            origin = t.origin;
            copies = t.copies + 1;
        }
        return *this;
    }
};

}

BOOST_AUTO_TEST_CASE(test_tracked_vector)
{
    Tracker t1;
    Tracker t2;
    Tracker t3;

    BOOST_CHECK(t1.origin == &t1);
    BOOST_CHECK(t2.origin == &t2);
    BOOST_CHECK(t3.origin == &t3);

    auto v1 = Vector(t1);
    BOOST_CHECK_EQUAL(v1.size(), 1U);
    BOOST_CHECK(v1[0].origin == &t1);
    BOOST_CHECK_EQUAL(v1[0].copies, 1);

    auto v2 = Vector(std::move(t2));
    BOOST_CHECK_EQUAL(v2.size(), 1U);
    BOOST_CHECK(v2[0].origin == &t2); // NOLINT(*-use-after-move)
    BOOST_CHECK_EQUAL(v2[0].copies, 0);

    auto v3 = Vector(t1, std::move(t2));
    BOOST_CHECK_EQUAL(v3.size(), 2U);
    BOOST_CHECK(v3[0].origin == &t1);
    BOOST_CHECK(v3[1].origin == &t2); // NOLINT(*-use-after-move)
    BOOST_CHECK_EQUAL(v3[0].copies, 1);
    BOOST_CHECK_EQUAL(v3[1].copies, 0);

    auto v4 = Vector(std::move(v3[0]), v3[1], std::move(t3));
    BOOST_CHECK_EQUAL(v4.size(), 3U);
    BOOST_CHECK(v4[0].origin == &t1);
    BOOST_CHECK(v4[1].origin == &t2);
    BOOST_CHECK(v4[2].origin == &t3); // NOLINT(*-use-after-move)
    BOOST_CHECK_EQUAL(v4[0].copies, 1);
    BOOST_CHECK_EQUAL(v4[1].copies, 1);
    BOOST_CHECK_EQUAL(v4[2].copies, 0);

    auto v5 = Cat(v1, v4);
    BOOST_CHECK_EQUAL(v5.size(), 4U);
    BOOST_CHECK(v5[0].origin == &t1);
    BOOST_CHECK(v5[1].origin == &t1);
    BOOST_CHECK(v5[2].origin == &t2);
    BOOST_CHECK(v5[3].origin == &t3);
    BOOST_CHECK_EQUAL(v5[0].copies, 2);
    BOOST_CHECK_EQUAL(v5[1].copies, 2);
    BOOST_CHECK_EQUAL(v5[2].copies, 2);
    BOOST_CHECK_EQUAL(v5[3].copies, 1);

    auto v6 = Cat(std::move(v1), v3);
    BOOST_CHECK_EQUAL(v6.size(), 3U);
    BOOST_CHECK(v6[0].origin == &t1);
    BOOST_CHECK(v6[1].origin == &t1);
    BOOST_CHECK(v6[2].origin == &t2);
    BOOST_CHECK_EQUAL(v6[0].copies, 1);
    BOOST_CHECK_EQUAL(v6[1].copies, 2);
    BOOST_CHECK_EQUAL(v6[2].copies, 1);

    auto v7 = Cat(v2, std::move(v4));
    BOOST_CHECK_EQUAL(v7.size(), 4U);
    BOOST_CHECK(v7[0].origin == &t2);
    BOOST_CHECK(v7[1].origin == &t1);
    BOOST_CHECK(v7[2].origin == &t2);
    BOOST_CHECK(v7[3].origin == &t3);
    BOOST_CHECK_EQUAL(v7[0].copies, 1);
    BOOST_CHECK_EQUAL(v7[1].copies, 1);
    BOOST_CHECK_EQUAL(v7[2].copies, 1);
    BOOST_CHECK_EQUAL(v7[3].copies, 0);

    auto v8 = Cat(std::move(v2), std::move(v3));
    BOOST_CHECK_EQUAL(v8.size(), 3U);
    BOOST_CHECK(v8[0].origin == &t2);
    BOOST_CHECK(v8[1].origin == &t1);
    BOOST_CHECK(v8[2].origin == &t2);
    BOOST_CHECK_EQUAL(v8[0].copies, 0);
    BOOST_CHECK_EQUAL(v8[1].copies, 1);
    BOOST_CHECK_EQUAL(v8[2].copies, 0);
}

BOOST_AUTO_TEST_CASE(message_sign)
{
    const std::array<unsigned char, 32> privkey_bytes = {
        // just some random data
        // derived address from this private key: 15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs
        0xD9, 0x7F, 0x51, 0x08, 0xF1, 0x1C, 0xDA, 0x6E,
        0xEE, 0xBA, 0xAA, 0x42, 0x0F, 0xEF, 0x07, 0x26,
        0xB1, 0xF8, 0x98, 0x06, 0x0B, 0x98, 0x48, 0x9F,
        0xA3, 0x09, 0x84, 0x63, 0xC0, 0x03, 0x28, 0x66
    };

    const std::string message = "Trust no one";

    const std::string expected_signature =
        "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=";

    CKey privkey;
    std::string generated_signature;

    BOOST_REQUIRE_MESSAGE(!privkey.IsValid(),
        "Confirm the private key is invalid");

    BOOST_CHECK_MESSAGE(!MessageSign(privkey, message, generated_signature),
        "Sign with an invalid private key");

    privkey.Set(privkey_bytes.begin(), privkey_bytes.end(), true);

    BOOST_REQUIRE_MESSAGE(privkey.IsValid(),
        "Confirm the private key is valid");

    BOOST_CHECK_MESSAGE(MessageSign(privkey, message, generated_signature),
        "Sign with a valid private key");

    BOOST_CHECK_EQUAL(expected_signature, generated_signature);
}

BOOST_AUTO_TEST_CASE(message_verify)
{
    BOOST_CHECK_EQUAL(
        MessageVerify(
            "invalid address",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_INVALID_ADDRESS);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "3B5fQsEXEaV8v6U3ejYc8XaKXAkyQj2MjV",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_ADDRESS_NO_KEY);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "invalid signature, not in base64 encoding",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_MALFORMED_SIGNATURE);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "I never signed this"),
        MessageVerificationResult::ERR_NOT_SIGNED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "Trust no one"),
        MessageVerificationResult::OK);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "11canuhp9X2NocwCq7xNrQYTmUgZAnLK3",
            "IIcaIENoYW5jZWxsb3Igb24gYnJpbmsgb2Ygc2Vjb25kIGJhaWxvdXQgZm9yIGJhbmtzIAaHRtbCeDZINyavx14=",
            "Trust me"),
        MessageVerificationResult::OK);
}

BOOST_AUTO_TEST_CASE(message_hash)
{
    const std::string unsigned_tx = "...";
    const std::string prefixed_message =
        std::string(1, (char)MESSAGE_MAGIC.length()) +
        MESSAGE_MAGIC +
        std::string(1, (char)unsigned_tx.length()) +
        unsigned_tx;

    const uint256 signature_hash = Hash(unsigned_tx);
    const uint256 message_hash1 = Hash(prefixed_message);
    const uint256 message_hash2 = MessageHash(unsigned_tx);

    BOOST_CHECK_EQUAL(message_hash1, message_hash2);
    BOOST_CHECK_NE(message_hash1, signature_hash);
}

BOOST_AUTO_TEST_CASE(remove_prefix)
{
    BOOST_CHECK_EQUAL(RemovePrefix("./common/system.h", "./"), "common/system.h");
    BOOST_CHECK_EQUAL(RemovePrefixView("foo", "foo"), "");
    BOOST_CHECK_EQUAL(RemovePrefix("foo", "fo"), "o");
    BOOST_CHECK_EQUAL(RemovePrefixView("foo", "f"), "oo");
    BOOST_CHECK_EQUAL(RemovePrefix("foo", ""), "foo");
    BOOST_CHECK_EQUAL(RemovePrefixView("fo", "foo"), "fo");
    BOOST_CHECK_EQUAL(RemovePrefix("f", "foo"), "f");
    BOOST_CHECK_EQUAL(RemovePrefixView("", "foo"), "");
    BOOST_CHECK_EQUAL(RemovePrefix("", ""), "");
}

BOOST_AUTO_TEST_CASE(util_ParseByteUnits)
{
    auto noop = ByteUnit::NOOP;

    // no multiplier
    BOOST_CHECK_EQUAL(ParseByteUnits("1", noop).value(), 1);
    BOOST_CHECK_EQUAL(ParseByteUnits("0", noop).value(), 0);

    BOOST_CHECK_EQUAL(ParseByteUnits("1k", noop).value(), 1000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("1K", noop).value(), 1ULL << 10);

    BOOST_CHECK_EQUAL(ParseByteUnits("2m", noop).value(), 2'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("2M", noop).value(), 2ULL << 20);

    BOOST_CHECK_EQUAL(ParseByteUnits("3g", noop).value(), 3'000'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("3G", noop).value(), 3ULL << 30);

    BOOST_CHECK_EQUAL(ParseByteUnits("4t", noop).value(), 4'000'000'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("4T", noop).value(), 4ULL << 40);

    // check default multiplier
    BOOST_CHECK_EQUAL(ParseByteUnits("5", ByteUnit::K).value(), 5ULL << 10);

    // NaN
    BOOST_CHECK(!ParseByteUnits("", noop));
    BOOST_CHECK(!ParseByteUnits("foo", noop));

    // whitespace
    BOOST_CHECK(!ParseByteUnits("123m ", noop));
    BOOST_CHECK(!ParseByteUnits(" 123m", noop));

    // no +-
    BOOST_CHECK(!ParseByteUnits("-123m", noop));
    BOOST_CHECK(!ParseByteUnits("+123m", noop));

    // zero padding
    BOOST_CHECK_EQUAL(ParseByteUnits("020M", noop).value(), 20ULL << 20);

    // fractions not allowed
    BOOST_CHECK(!ParseByteUnits("0.5T", noop));

    // overflow
    BOOST_CHECK(!ParseByteUnits("18446744073709551615g", noop));

    // invalid unit
    BOOST_CHECK(!ParseByteUnits("1x", noop));
}

BOOST_AUTO_TEST_CASE(util_ReadBinaryFile)
{
    fs::path tmpfolder = m_args.GetDataDirBase();
    fs::path tmpfile = tmpfolder / "read_binary.dat";
    std::string expected_text;
    for (int i = 0; i < 30; i++) {
        expected_text += "0123456789";
    }
    {
        std::ofstream file{tmpfile.std_path()};
        file << expected_text;
    }
    {
        // read all contents in file
        auto [valid, text] = ReadBinaryFile(tmpfile);
        BOOST_CHECK(valid);
        BOOST_CHECK_EQUAL(text, expected_text);
    }
    {
        // read half contents in file
        auto [valid, text] = ReadBinaryFile(tmpfile, expected_text.size() / 2);
        BOOST_CHECK(valid);
        BOOST_CHECK_EQUAL(text, expected_text.substr(0, expected_text.size() / 2));
    }
    {
        // read from non-existent file
        fs::path invalid_file = tmpfolder / "invalid_binary.dat";
        auto [valid, text] = ReadBinaryFile(invalid_file);
        BOOST_CHECK(!valid);
        BOOST_CHECK(text.empty());
    }
}

BOOST_AUTO_TEST_CASE(util_WriteBinaryFile)
{
    fs::path tmpfolder = m_args.GetDataDirBase();
    fs::path tmpfile = tmpfolder / "write_binary.dat";
    std::string expected_text = "bitcoin";
    auto valid = WriteBinaryFile(tmpfile, expected_text);
    std::string actual_text;
    std::ifstream file{tmpfile.std_path()};
    file >> actual_text;
    BOOST_CHECK(valid);
    BOOST_CHECK_EQUAL(actual_text, expected_text);
}

BOOST_AUTO_TEST_CASE(clearshrink_test)
{
    {
        std::vector<uint8_t> v = {1, 2, 3};
        ClearShrink(v);
        BOOST_CHECK_EQUAL(v.size(), 0);
        BOOST_CHECK_EQUAL(v.capacity(), 0);
    }

    {
        std::vector<bool> v = {false, true, false, false, true, true};
        ClearShrink(v);
        BOOST_CHECK_EQUAL(v.size(), 0);
        BOOST_CHECK_EQUAL(v.capacity(), 0);
    }

    {
        std::deque<int> v = {1, 3, 3, 7};
        ClearShrink(v);
        BOOST_CHECK_EQUAL(v.size(), 0);
        // std::deque has no capacity() we can observe.
    }
}

template <typename T>
void TestCheckedLeftShift()
{
    constexpr auto MAX{std::numeric_limits<T>::max()};

    // Basic operations
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(0, 1), 0);
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(0, 127), 0);
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(1, 1), 2);
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(2, 2), 8);
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(MAX >> 1, 1), MAX - 1);

    // Max left shift
    BOOST_CHECK_EQUAL(CheckedLeftShift<T>(1, std::numeric_limits<T>::digits - 1), MAX / 2 + 1);

    // Overflow cases
    BOOST_CHECK(!CheckedLeftShift<T>((MAX >> 1) + 1, 1));
    BOOST_CHECK(!CheckedLeftShift<T>(MAX, 1));
    BOOST_CHECK(!CheckedLeftShift<T>(1, std::numeric_limits<T>::digits));
    BOOST_CHECK(!CheckedLeftShift<T>(1, std::numeric_limits<T>::digits + 1));

    if constexpr (std::is_signed_v<T>) {
        constexpr auto MIN{std::numeric_limits<T>::min()};
        // Negative input
        BOOST_CHECK_EQUAL(CheckedLeftShift<T>(-1, 1), -2);
        BOOST_CHECK_EQUAL(CheckedLeftShift<T>((MIN >> 2), 1), MIN / 2);
        BOOST_CHECK_EQUAL(CheckedLeftShift<T>((MIN >> 1) + 1, 1), MIN + 2);
        BOOST_CHECK_EQUAL(CheckedLeftShift<T>(MIN >> 1, 1), MIN);
        // Overflow negative
        BOOST_CHECK(!CheckedLeftShift<T>((MIN >> 1) - 1, 1));
        BOOST_CHECK(!CheckedLeftShift<T>(MIN >> 1, 2));
        BOOST_CHECK(!CheckedLeftShift<T>(-1, 100));
    }
}

template <typename T>
void TestSaturatingLeftShift()
{
    constexpr auto MAX{std::numeric_limits<T>::max()};

    // Basic operations
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(0, 1), 0);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(0, 127), 0);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(1, 1), 2);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(2, 2), 8);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(MAX >> 1, 1), MAX - 1);

    // Max left shift
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(1, std::numeric_limits<T>::digits - 1), MAX / 2 + 1);

    // Saturation cases
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>((MAX >> 1) + 1, 1), MAX);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(MAX, 1), MAX);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(1, std::numeric_limits<T>::digits), MAX);
    BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(1, std::numeric_limits<T>::digits + 1), MAX);

    if constexpr (std::is_signed_v<T>) {
        constexpr auto MIN{std::numeric_limits<T>::min()};
        // Negative input
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(-1, 1), -2);
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>((MIN >> 2), 1), MIN / 2);
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>((MIN >> 1) + 1, 1), MIN + 2);
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(MIN >> 1, 1), MIN);
        // Saturation negative
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>((MIN >> 1) - 1, 1), MIN);
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(MIN >> 1, 2), MIN);
        BOOST_CHECK_EQUAL(SaturatingLeftShift<T>(-1, 100), MIN);
    }
}

BOOST_AUTO_TEST_CASE(checked_left_shift_test)
{
    TestCheckedLeftShift<uint8_t>();
    TestCheckedLeftShift<int8_t>();
    TestCheckedLeftShift<size_t>();
    TestCheckedLeftShift<uint64_t>();
    TestCheckedLeftShift<int64_t>();
}

BOOST_AUTO_TEST_CASE(saturating_left_shift_test)
{
    TestSaturatingLeftShift<uint8_t>();
    TestSaturatingLeftShift<int8_t>();
    TestSaturatingLeftShift<size_t>();
    TestSaturatingLeftShift<uint64_t>();
    TestSaturatingLeftShift<int64_t>();
}

BOOST_AUTO_TEST_CASE(mib_string_literal_test)
{
    BOOST_CHECK_EQUAL(0_MiB, 0);
    BOOST_CHECK_EQUAL(1_MiB, 1024 * 1024);
    const auto max_mib{std::numeric_limits<size_t>::max() >> 20};
    BOOST_CHECK_EXCEPTION(operator""_MiB(static_cast<unsigned long long>(max_mib) + 1), std::overflow_error, HasReason("MiB value too large for size_t byte conversion"));
}

BOOST_AUTO_TEST_SUITE_END()
