// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <test/util/pretty_data.h>
#include <test/util/setup_common.h>
#include <test/util/traits.h>
#include <test/util/vector.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using namespace test::util::literals;

BOOST_FIXTURE_TEST_SUITE(test_util_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(as_string_tests)
{
    // Some typical values, independent of integral type
    BOOST_TEST(as_string(0) == "0"s);
    BOOST_TEST(as_string(1) == "1"s);
    BOOST_TEST(as_string(-1) == "-1"s);
    BOOST_TEST(as_string(12) == "12"s);
    BOOST_TEST(as_string(123) == "123"s);
    BOOST_TEST(as_string(1234) == "1234"s);

    // Now some larger values
    BOOST_TEST(as_string(1'000'000'000'000ULL) == "1000000000000"s);
    BOOST_TEST(as_string(-1'234'567'890'123LL) == "-1234567890123"s);
}

BOOST_AUTO_TEST_CASE(hex_to_stream)
{
    auto ToStream = [](auto v) {
        std::ostringstream oss;
        oss << Hex(v);
        return oss.str();
    };

    {
        // integral types
        BOOST_TEST(ToStream(static_cast<int8_t>(0x75)) == "0x75");
        BOOST_TEST(ToStream(static_cast<uint8_t>(0x75)) == "0x75");
        BOOST_TEST(ToStream(0x75) == "0x00000075");
        BOOST_TEST(ToStream(0x75U) == "0x00000075");
        BOOST_TEST(ToStream(0x75LL) == "0x0000000000000075");
        BOOST_TEST(ToStream(0x75ULL) == "0x0000000000000075");
    }

    {
        // `uint256` which acts like a container
        BOOST_TEST(ToStream(uint256::ZERO) == "0x0000000000000000000000000000000000000000000000000000000000000000");
        BOOST_TEST(ToStream(uint256::ONE) == "0x0000000000000000000000000000000000000000000000000000000000000001");
        BOOST_TEST(ToStream(uint256S("fedcba9876543210"s)) == "0x000000000000000000000000000000000000000000000000fedcba9876543210");
        BOOST_TEST(ToStream(uint256S("0123456789abcdef00000000000000000000000000000000fedcba9876543210")) == "0x0123456789abcdef00000000000000000000000000000000fedcba9876543210");
    }

    {
        // `uint160` also acts like a container

        // when initialized from a vector it is little-endian; but when written as a hex string it is big-endian
        std::vector<unsigned char> v160{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7, 0xf6};
        BOOST_TEST(ToStream(uint160(v160)) == "0xf6f7f8f9fafbfcfdfeff0a090807060504030201");
    }

    {
        // Span of unsigned char
        std::vector<unsigned char> v{0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
        BOOST_TEST(ToStream(Span(v)) == "0x101112131415");
    }
}

BOOST_AUTO_TEST_CASE(string_hex_to_bytes_and_back)
{
    using valtype = std::vector<unsigned char>;

    // hex strings to byte vector
    BOOST_TEST("0A"_hex == valtype(1, 0x0a));
    BOOST_TEST("0a"_hex == valtype(1, 0x0a));
    BOOST_TEST("A0"_hex == valtype(1, 0xa0));
    BOOST_TEST("a0"_hex == valtype(1, 0xa0));
    BOOST_TEST("aA"_hex == valtype(1, 0xaa));
    BOOST_TEST("Aa"_hex == valtype(1, 0xaa));

    BOOST_TEST(("12"_hex == valtype{0x12}));
    BOOST_TEST(("1234"_hex == valtype{0x12, 0x34}));
    BOOST_TEST(("123456"_hex == valtype{0x12, 0x34, 0x56}));

    // Invalid hex string literals
    BOOST_CHECK_THROW("1"_hex, std::logic_error);
    BOOST_CHECK_THROW("123"_hex, std::logic_error);
    BOOST_CHECK_THROW("1234xyz"_hex, std::logic_error);

    // Binary to hex string
    BOOST_TEST("12345678abcd"s == HexStr("12345678abcd"_hex));
    BOOST_TEST(std::string(64, '0') == HexStr(uint256::ZERO));
    BOOST_TEST((std::string{'0', '1'} + std::string(62, '0') == HexStr(uint256::ONE)));
}

BOOST_AUTO_TEST_CASE(string_to_bytes)
{
    using valtype = std::vector<unsigned char>;

    BOOST_TEST(("Aa"_bv == valtype{'A', 'a'}));
    BOOST_TEST(("ABCD-0123-xyz"_bv == valtype{'A', 'B', 'C', 'D', '-', '0', '1', '2', '3', '-', 'x', 'y', 'z'}));
}

BOOST_AUTO_TEST_CASE(HexStr_problem)
{
    std::vector<unsigned char> a5(1, 0xa5);
    auto a5_r = HexStr(Span<unsigned char>(a5));
    BOOST_TEST(a5_r == "a5"s);
}

BOOST_AUTO_TEST_CASE(ex_vector_tests)
{
    using valtype = std::vector<unsigned char>;

    using namespace test::util::vector_ops;

    {
        // Conversion via unary `+`
        std::vector<int> v{10, 20, 30};
        auto ev = +v;
        BOOST_TEST(ev == v);
    }

    {
        // Subrange operation
        ex_vector<unsigned char> c1{"ABCDEFGH"_bv};

        BOOST_TEST((c1[{0, 2}] == "AB"_bv));
        BOOST_TEST((c1[{1, 4}] == "BCD"_bv));
        BOOST_TEST((c1[{3, 7}] == "DEFG"_bv));
        BOOST_TEST((c1[{0, 8}] == c1));
    }

    {
        // Creating ex_vector from a `uint256`.
        //
        // (w.r.t. these tests note there is an endianness issue - HexStr will print little-endian,
        // as a byte array, NOT big-endian like an integer
        ex_vector<unsigned char> exv_ZERO = from_base_blob(uint256::ZERO);
        BOOST_TEST(HexStr(exv_ZERO) == "0000000000000000000000000000000000000000000000000000000000000000");
        ex_vector<unsigned char> exv_ONE = from_base_blob(uint256::ONE);
        BOOST_TEST(HexStr(exv_ONE) == "0100000000000000000000000000000000000000000000000000000000000000");
        ex_vector<unsigned char> exv_other = from_base_blob(uint256S(
            "0123456789abcdef00000000000000000000000000000000fedcba9876543210"));
        BOOST_TEST(HexStr(exv_other) == "1032547698badcfe00000000000000000000000000000000efcdab8967452301");
    }

    {
        // Concatenating vectors via `operator||` (producing ex_vectors)
        valtype c0{};
        valtype c1{"ABCDEFGH"_bv};
        valtype c2{"1234"_bv};

        BOOST_TEST(((c0 || c0) == c0));
        BOOST_TEST(((c0 || c1) == c1));
        BOOST_TEST(((c2 || c0) == c2));
        BOOST_TEST(((c1 || c2) == "ABCDEFGH1234"_bv));
        BOOST_TEST(((c2 || c1) == "1234ABCDEFGH"_bv));
    }
}

BOOST_AUTO_TEST_CASE(pretty_flags)
{
    // A small assortment
    BOOST_TEST(SCRIPT_VERIFY_P2SH == ParseScriptFlags("P2SH"sv, false));
    BOOST_TEST(SCRIPT_VERIFY_SIGPUSHONLY == ParseScriptFlags("SIGPUSHONLY"sv, false));
    BOOST_TEST(SCRIPT_VERIFY_TAPROOT == ParseScriptFlags("TAPROOT"sv, false));
    BOOST_TEST((SCRIPT_VERIFY_MINIMALIF | SCRIPT_VERIFY_WITNESS) == ParseScriptFlags("MINIMALIF,WITNESS"sv, false));
    BOOST_TEST((SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_LOW_S | SCRIPT_VERIFY_DERSIG) == ParseScriptFlags("TAPROOT,LOW_S,DERSIG"sv, false));
    BOOST_TEST(0 == ParseScriptFlags("F00BAR"sv, false));
    BOOST_TEST(0 == ParseScriptFlags("witness"sv, false));        // case-sensitive
    BOOST_TEST(0 == ParseScriptFlags("P2SH , WITNESS"sv, false)); // Underlying function "split" doesn't trim

    BOOST_TEST("STRICTENC"sv == FormatScriptFlags(SCRIPT_VERIFY_STRICTENC));
    BOOST_TEST("WITNESS_PUBKEYTYPE"sv == FormatScriptFlags(SCRIPT_VERIFY_WITNESS_PUBKEYTYPE));
    BOOST_TEST("CONST_SCRIPTCODE,TAPROOT"sv == FormatScriptFlags(SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_CONST_SCRIPTCODE));
    BOOST_TEST("CLEANSTACK,NULLDUMMY,NULLFAIL"sv == FormatScriptFlags(SCRIPT_VERIFY_NULLDUMMY | SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_NULLFAIL));
    BOOST_TEST(""sv == FormatScriptFlags(0));
    BOOST_TEST(""sv == FormatScriptFlags(1U << 30)); // invalid flags just ignored
    BOOST_TEST("WITNESS"sv == FormatScriptFlags((1U << 30) | SCRIPT_VERIFY_WITNESS));

    // Systematic
    auto flag_map = MapFlagNames();
    for (auto [name1, value1] : flag_map) {
        BOOST_TEST(value1 == ParseScriptFlags(name1));
        BOOST_TEST(name1 == FormatScriptFlags(value1));

        for (auto [name2, value2] : flag_map) {
            if (value1 == value2) continue;
            BOOST_TEST((value1 | value2) == ParseScriptFlags(std::string(name1) + "," + std::string(name2), false));
            BOOST_TEST(std::string(name1 < name2 ? name1 : name2) + "," + std::string(name1 < name2 ? name2 : name1) == FormatScriptFlags(value1 | value2));
        }
    }
}

BOOST_AUTO_TEST_CASE(pretty_script_errors)
{
    // A small assortment
    BOOST_TEST("OK"sv == FormatScriptError(SCRIPT_ERR_OK, false));
    BOOST_TEST("UNKNOWN_ERROR"sv == FormatScriptError(SCRIPT_ERR_UNKNOWN_ERROR, false));
    BOOST_TEST(""sv == FormatScriptError(SCRIPT_ERR_ERROR_COUNT, false));

    BOOST_TEST(SCRIPT_ERR_OK == ParseScriptError("OK"sv, false));
    BOOST_TEST(SCRIPT_ERR_SCHNORR_SIG_SIZE == ParseScriptError("SCHNORR_SIG_SIZE"sv, false));
    BOOST_TEST(SCRIPT_ERR_UNKNOWN_ERROR == ParseScriptError("F00Bar"sv, false));
    BOOST_TEST(SCRIPT_ERR_UNKNOWN_ERROR == ParseScriptError("Schnorr_Sig"sv, false)); // case-sensitive
    BOOST_TEST(SCRIPT_ERR_UNKNOWN_ERROR == ParseScriptError("MINIMALIF "sv, false));  // doesn't trim arg

    // Systematic
    for (size_t i = 0; i < SCRIPT_ERR_ERROR_COUNT; ++i) {
        const auto name = FormatScriptError(static_cast<ScriptError_t>(i));
        BOOST_TEST(!name.empty());
        BOOST_TEST(i == ParseScriptError(name));
    }
}

#define EVAL(...) []() -> bool { return __VA_ARGS__; }()

BOOST_AUTO_TEST_CASE(is_base_of_trait)
{
    class A
    {
    };
    class B : A
    {
    };
    class C : B
    {
    };
    class D
    {
    };

    BOOST_TEST(EVAL((std::is_base_of_v<A, A>)));
    BOOST_TEST(EVAL((std::is_base_of_v<A, B>)));
    BOOST_TEST(EVAL((std::is_base_of_v<A, C>)));
    BOOST_TEST(EVAL(!(std::is_base_of_v<A, D>)));
    BOOST_TEST(EVAL(!(std::is_base_of_v<B, A>)));
    BOOST_TEST(EVAL(!(std::is_base_of_v<int, int>)));
}

template <unsigned int UI>
struct base_template {
};

BOOST_AUTO_TEST_CASE(is_base_template_of_template_trait)
{
    struct d_1010 : public base_template<1010> {
    };
    struct d_2022 : public base_template<2022> {
    };

    BOOST_TEST(EVAL((traits::is_base_of_template_of_uint_v<base_template, d_1010>)));
    BOOST_TEST(EVAL((traits::is_base_of_template_of_uint_v<base_template, d_2022>)));
    BOOST_TEST(EVAL(!(traits::is_base_of_template_of_uint_v<base_template, std::ostringstream>)));
    BOOST_TEST(EVAL(!(traits::is_base_of_template_of_uint_v<base_template, int>)));
}

template <typename T>
using copy_assign_t = decltype(std::declval<T&>() = std::declval<const T&>());

template <typename C>
bool fun(...)
{
    return false;
}

template <typename C, typename = traits::ok_for_range_based_for<C>>
bool fun(int)
{
    return true;
}

BOOST_AUTO_TEST_CASE(is_detected)
{
    struct Meow {
    };
    struct Purr {
        void operator=(const Purr&) = delete;
    };

    BOOST_TEST(EVAL((traits::is_detected_v<copy_assign_t, Meow>)));
    BOOST_TEST(EVAL(!(traits::is_detected_v<copy_assign_t, Purr>)));
}

BOOST_AUTO_TEST_CASE(ok_for_range_based_for)
{
    BOOST_TEST(EVAL((fun<std::vector<int>>(5))));
    BOOST_TEST(EVAL(!(fun<std::allocator<int>>(5))));
}

BOOST_AUTO_TEST_SUITE_END()
