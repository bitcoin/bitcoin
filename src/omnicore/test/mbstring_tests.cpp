#include "omnicore/mbstring.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stddef.h>
#include <string>

namespace mastercore {
namespace mbstring {
// Forward declarations
extern int get_mblen(unsigned char c);
extern int get_mblen(const char* s, size_t n);
extern bool check_mb(const char* s, size_t n);

/** Helper to check if the given characters qualify as valid UTF-8 encoded. */
bool check_mb(const std::string& s)
{
    int next = get_mblen(s.data(), s.size());
    return check_mb(s.data(), next);
}
}
}

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_mbstring_tests, BasicTestingSetup)

/**
 * Many examples were adopted from the following sources:
 *
 * http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
 * https://github.com/rakudo-p5/roast5/blob/master/op/utf8decode.t
 * https://en.wikipedia.org/wiki/Mojibake
 */

BOOST_AUTO_TEST_CASE(len_unchecked)
{
    BOOST_CHECK_EQUAL(1, mbstring::get_mblen(0x7F));
    BOOST_CHECK_EQUAL(2, mbstring::get_mblen(0xC0));
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen(0xE0));
    BOOST_CHECK_EQUAL(4, mbstring::get_mblen(0xF0));
    BOOST_CHECK_EQUAL(5, mbstring::get_mblen(0xF8)); // more than 4 bytes are no valid UTF-8
    BOOST_CHECK_EQUAL(6, mbstring::get_mblen(0xFC)); // more than 4 bytes are no valid UTF-8
    BOOST_CHECK_EQUAL(7, mbstring::get_mblen(0xFE)); // more than 4 bytes are no valid UTF-8
}

BOOST_AUTO_TEST_CASE(length_min)
{
    BOOST_CHECK_EQUAL(0, mbstring::get_mblen("", 0));
    BOOST_CHECK_EQUAL(0, mbstring::get_mblen("\x00", 1));
    BOOST_CHECK_EQUAL(1, mbstring::get_mblen("\x01", 1));
    BOOST_CHECK_EQUAL(2, mbstring::get_mblen("\xc2\x80", 2));
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen("\xe0\xa0\x80", 3));
    BOOST_CHECK_EQUAL(4, mbstring::get_mblen("\xf0\x90\x80\x80", 4));
    BOOST_CHECK_EQUAL(5, mbstring::get_mblen("\xf8\x88\x80\x80\x80", 5));
    BOOST_CHECK_EQUAL(6, mbstring::get_mblen("\xfc\x84\x80\x80\x80\x80", 6));
}

BOOST_AUTO_TEST_CASE(length_max)
{
    BOOST_CHECK_EQUAL(1, mbstring::get_mblen("\x7f", 1));
    BOOST_CHECK_EQUAL(2, mbstring::get_mblen("\xdf\xbf", 2));
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen("\xef\xbf\xbf", 3));
    BOOST_CHECK_EQUAL(4, mbstring::get_mblen("\xf7\xbf\xbf\xbf", 4));
    BOOST_CHECK_EQUAL(5, mbstring::get_mblen("\xfb\xbf\xbf\xbf\xbf", 5));
    BOOST_CHECK_EQUAL(6, mbstring::get_mblen("\xfd\xbf\xbf\xbf\xbf\xbf", 6));
}

BOOST_AUTO_TEST_CASE(length_other_bounds)
{
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen("\xed\x9f\xbf", 3));
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen("\xee\x80\x80", 3));
    BOOST_CHECK_EQUAL(3, mbstring::get_mblen("\xef\xbf\xbd", 3));
    BOOST_CHECK_EQUAL(4, mbstring::get_mblen("\xf4\x8f\xbf\xbf", 4));
    BOOST_CHECK_EQUAL(4, mbstring::get_mblen("\xf4\x90\x80\x80", 4));
}

BOOST_AUTO_TEST_CASE(valid_ff)
{
    BOOST_CHECK(mbstring::check_mb("\x00"));
    BOOST_CHECK(mbstring::check_mb("\x01"));
    BOOST_CHECK(mbstring::check_mb("\xc2\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe0\xa0\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf0\x90\x80\x80"));
}

BOOST_AUTO_TEST_CASE(unexpected_continuation)
{
    BOOST_CHECK(!mbstring::check_mb("\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf\x80"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf\x80\xbf\x80"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf\x80\xbf\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\x80\xbf\x80\xbf\x80\xbf\x80"));
}

BOOST_AUTO_TEST_CASE(lonely_start_characters_2_byte)
{
    BOOST_CHECK(!mbstring::check_mb("\xc0\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc1\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc2\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc3\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc4\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc5\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc6\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc7\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc8\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc9\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xca\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xcb\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xcc\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xcd\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xce\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xcf\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd0\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd1\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd2\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd3\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd4\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd5\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd6\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd7\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd8\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xd9\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xda\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xdb\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xdc\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xdd\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xde\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xdf\x20"));
}

BOOST_AUTO_TEST_CASE(lonely_start_characters_3_byte)
{
    BOOST_CHECK(!mbstring::check_mb("\xe0\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe1\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe2\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe3\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe4\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe5\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe6\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe7\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe8\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xe9\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xea\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xeb\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xec\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xed\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xee\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xef\x20"));
}

BOOST_AUTO_TEST_CASE(lonely_start_characters_4_byte)
{
    BOOST_CHECK(!mbstring::check_mb("\xf0\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf2\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf3\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf5\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf6\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf7\x20"));
}

BOOST_AUTO_TEST_CASE(lonely_start_characters_5_byte)
{
    BOOST_CHECK(!mbstring::check_mb("\xf8\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf9\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xfa\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xfb\x20"));
}

BOOST_AUTO_TEST_CASE(lonely_start_characters_6_byte)
{
    BOOST_CHECK(!mbstring::check_mb("\xfc\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xfd\x20"));
}

BOOST_AUTO_TEST_CASE(continuations_2_byte)
{
    // Invalid
    BOOST_CHECK(!mbstring::check_mb("\xc0"));
    BOOST_CHECK(!mbstring::check_mb("\xc0\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xc0\x40"));
    BOOST_CHECK(!mbstring::check_mb("\xc0\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xc2"));
    BOOST_CHECK(!mbstring::check_mb("\xc2\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xc2\x40"));
    BOOST_CHECK(!mbstring::check_mb("\xc4\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xdf"));
    BOOST_CHECK(!mbstring::check_mb("\xdf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xdf\xff"));
    // Valid
    BOOST_CHECK(mbstring::check_mb("\xc6\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdf\xbf"));
}

BOOST_AUTO_TEST_CASE(continuations_3_byte)
{
    // Invalid
    BOOST_CHECK(!mbstring::check_mb("\xe2\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xe2\x80\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\xbf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf\xc0"));
    BOOST_CHECK(!mbstring::check_mb("\xef\xbf\xff"));
    // Valid
    BOOST_CHECK(mbstring::check_mb("\xe2\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xef\xbf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xef\xbf\xbf"));
}

BOOST_AUTO_TEST_CASE(continuations_4_byte)
{
    // Invalid
    BOOST_CHECK(!mbstring::check_mb("\xf1"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x80\x80\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x80\x80\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf1\x80\x80\xc0"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf\x00"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf\x20"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf\xc0"));
    BOOST_CHECK(!mbstring::check_mb("\xf4\x8f\xbf\xff"));
    // Valid
    BOOST_CHECK(mbstring::check_mb("\xf1\x80\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf4\x8f\xbf\xbf"));
}

BOOST_AUTO_TEST_CASE(continuations_other)
{
    // F5+ is invalid
    BOOST_CHECK(!mbstring::check_mb("\xf7\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf7\xbf\xbf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xfb\xbf\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xfb\xbf\xbf\xbf\x7f"));
    BOOST_CHECK(!mbstring::check_mb("\xfd\xbf\xbf\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xfd\xbf\xbf\xbf\xbf\x7f"));
}

BOOST_AUTO_TEST_CASE(impossible)
{
    BOOST_CHECK(!mbstring::check_mb("\xfe"));
    BOOST_CHECK(!mbstring::check_mb("\xfe\x80\x80\x80\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xff"));
    BOOST_CHECK(!mbstring::check_mb("\xff\xbf\xbf\xbf\xbf\xbf\xbf\xbf"));
}

BOOST_AUTO_TEST_CASE(range_1_byte)
{
    // 00-7F, valid
    BOOST_CHECK(mbstring::check_mb("\x00"));
    BOOST_CHECK(mbstring::check_mb("\x7f"));
    // 80–BF, invalid (only as continuation)
    BOOST_CHECK(!mbstring::check_mb("\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xbf"));
    // C0-C1, invalid
    BOOST_CHECK(!mbstring::check_mb("\xc0"));
    BOOST_CHECK(!mbstring::check_mb("\xc1"));
    BOOST_CHECK(!mbstring::check_mb("\xc0\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xc0\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xc1\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xc1\xbf"));
}

BOOST_AUTO_TEST_CASE(range_2_byte)
{
    // C2-DF, valid
    BOOST_CHECK(mbstring::check_mb("\xc2\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc3\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc4\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc5\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc6\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc7\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc8\x80"));
    BOOST_CHECK(mbstring::check_mb("\xc9\x80"));
    BOOST_CHECK(mbstring::check_mb("\xca\x80"));
    BOOST_CHECK(mbstring::check_mb("\xcb\x80"));
    BOOST_CHECK(mbstring::check_mb("\xcc\x80"));
    BOOST_CHECK(mbstring::check_mb("\xcd\x80"));
    BOOST_CHECK(mbstring::check_mb("\xce\x80"));
    BOOST_CHECK(mbstring::check_mb("\xcf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd0\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd1\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd2\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd3\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd4\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd5\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd6\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd7\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd8\x80"));
    BOOST_CHECK(mbstring::check_mb("\xd9\x80"));
    BOOST_CHECK(mbstring::check_mb("\xda\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdb\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdc\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdd\x80"));
    BOOST_CHECK(mbstring::check_mb("\xde\x80"));
    BOOST_CHECK(mbstring::check_mb("\xdf\x80"));
}

BOOST_AUTO_TEST_CASE(range_3_byte)
{
    // E0, invalid subrange
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\x9f"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\xa0"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x9f\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x9f\xbf"));
    // E0, valid subrange A0-BF
    BOOST_CHECK(mbstring::check_mb("\xe0\xa0\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe0\xa0\xbf"));
    BOOST_CHECK(mbstring::check_mb("\xe0\xbf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe0\xbf\xbf"));
    // E1–EC
    BOOST_CHECK(mbstring::check_mb("\xe1\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe2\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe3\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe4\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe5\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe6\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe7\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe8\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xe9\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xea\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xeb\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xec\x80\x80"));
    // ED, valid subrange 80-9F
    BOOST_CHECK(mbstring::check_mb("\xed\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xed\x80\xbf"));
    BOOST_CHECK(mbstring::check_mb("\xed\x9f\x80"));
    BOOST_CHECK(mbstring::check_mb("\xed\x9f\xbf"));
    // ED, invalid subrange
    BOOST_CHECK(!mbstring::check_mb("\xed\xa0\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xed\xa0\x9f"));
    BOOST_CHECK(!mbstring::check_mb("\xed\xbf\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xed\xbf\x9f"));
    // ED–EF
    BOOST_CHECK(mbstring::check_mb("\xed\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xee\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xef\x80\x80"));
}

BOOST_AUTO_TEST_CASE(range_4_byte)
{
    // F0, invalid subrange
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\xbf\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x8f\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x8f\x80\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x8f\xbf\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x8f\xbf\xbf"));
    // F0, valid subrange 90-BF
    BOOST_CHECK(mbstring::check_mb("\xf0\x90\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf0\x90\x80\xbf"));
    BOOST_CHECK(mbstring::check_mb("\xf0\x90\xbf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf0\x90\xbf\xbf"));
    BOOST_CHECK(mbstring::check_mb("\xf0\xbf\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf0\xbf\x80\xbf"));
    BOOST_CHECK(mbstring::check_mb("\xf0\xbf\xbf\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf0\xbf\xbf\xbf"));
    // F1-F4
    BOOST_CHECK(mbstring::check_mb("\xf1\x80\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf2\x80\x80\x80"));
    BOOST_CHECK(mbstring::check_mb("\xf3\x80\x80\x80"));
    // F5-F7, invalid
    BOOST_CHECK(!mbstring::check_mb("\xf5\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf6\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf7\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(range_5_byte)
{
    // F8-FB, invalid
    BOOST_CHECK(!mbstring::check_mb("\xf8\x80\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xfb\x80\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(range_6_byte)
{
    // FC-FD, invalid
    BOOST_CHECK(!mbstring::check_mb("\xfc\x80\x80\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xfd\x80\x80\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(range_7_byte)
{
    // FE, invalid
    BOOST_CHECK(!mbstring::check_mb("\xfe\x80\x80\x80\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(range_8_byte)
{
    // FF, invalid
    BOOST_CHECK(!mbstring::check_mb("\xff\x80\x80\x80\x80\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(overlong_ascii)
{
    // Examples of an overlong ASCII character
    BOOST_CHECK(!mbstring::check_mb("\xc0\xaf"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\xaf"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\x80\xaf"));
    BOOST_CHECK(!mbstring::check_mb("\xf8\x80\x80\x80\xaf"));
    BOOST_CHECK(!mbstring::check_mb("\xfc\x80\x80\x80\x80\xaf"));
    // Maximum overlong sequences
    BOOST_CHECK(!mbstring::check_mb("\xc1\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x9f\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x8f\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xf8\x87\xbf\xbf\xbf"));
    BOOST_CHECK(!mbstring::check_mb("\xfc\x83\xbf\xbf\xbf\xbf"));
    // Overlong representation of the NUL character
    BOOST_CHECK(!mbstring::check_mb("\xc0\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xe0\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf0\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xf8\x80\x80\x80\x80"));
    BOOST_CHECK(!mbstring::check_mb("\xfc\x80\x80\x80\x80\x80"));
}

BOOST_AUTO_TEST_CASE(mix_with_illegal_single_utf_16_surrogates)
{
    // Contains valid and invalid character sequences
    BOOST_CHECK_EQUAL("\xe5\x8d\x9a\xe6\x84\x9b\x3f\x3f\x3f\xe5\x88\x9d\xe6\x81\x8b",
            SanitizeInvalidUTF8("\xe5\x8d\x9a\xe6\x84\x9b\xed\xa0\x80\xe5\x88\x9d\xe6\x81\x8b"));
    BOOST_CHECK_EQUAL("\xe3\x81\x82\xe3\x81\x84\x3f\x3f\x3f\xe7\xa7\x91\xe5\xad\xa6",
            SanitizeInvalidUTF8("\xe3\x81\x82\xe3\x81\x84\xed\xad\xbf\xe7\xa7\x91\xe5\xad\xa6"));
    BOOST_CHECK_EQUAL("\xe9\xad\x94\xe6\xb3\x95\x3f\x3f\x3f\xe5\xb0\x91\xe5\xa5\xb3",
            SanitizeInvalidUTF8("\xe9\xad\x94\xe6\xb3\x95\xed\xae\x80\xe5\xb0\x91\xe5\xa5\xb3"));
    BOOST_CHECK_EQUAL("\xe3\x81\x84\xe3\x82\x82\xe3\x81\x86\xe3\x81\xa8\x3f\x3f\x3f",
            SanitizeInvalidUTF8("\xe3\x81\x84\xe3\x82\x82\xe3\x81\x86\xe3\x81\xa8\xed\xaf\xbf"));
    BOOST_CHECK_EQUAL("\xe7\x8a\xac\x3f\x3f\x3f\xe3\x81\x93\xe3\x81\x93\xe3\x82\x8d",
            SanitizeInvalidUTF8("\xe7\x8a\xac\xed\xb0\x80\xe3\x81\x93\xe3\x81\x93\xe3\x82\x8d"));
    BOOST_CHECK_EQUAL("\xe6\x9c\x88\x3f\x3f\x3f\xe4\xba\xba\xe9\xa1\x9e",
            SanitizeInvalidUTF8("\xe6\x9c\x88\xed\xbe\x80\xe4\xba\xba\xe9\xa1\x9e"));
    BOOST_CHECK_EQUAL("\xe5\xa4\x89\x3f\x3f\x3f\xe5\xa4\x9c\xe6\xb5\xb7",
            SanitizeInvalidUTF8("\xe5\xa4\x89\xed\xbf\xbf\xe5\xa4\x9c\xe6\xb5\xb7"));
}

BOOST_AUTO_TEST_CASE(mix_with_illegal_paired_utf_16_surrogates)
{
    // Contains valid and invalid character sequences
    BOOST_CHECK_EQUAL("\x61\x3f\x3f\x3f\x3f\x3f\x3f\x62",
            SanitizeInvalidUTF8("\x61\xed\xa0\x80\xed\xb0\x80\x62"));
    BOOST_CHECK_EQUAL("\x63\x3f\x3f\x3f\x3f\x3f\x3f\x64",
            SanitizeInvalidUTF8("\x63\xed\xa0\x80\xed\xbf\xbf\x64"));
    BOOST_CHECK_EQUAL("\x65\x3f\x3f\x3f\x3f\x3f\x3f\x66",
            SanitizeInvalidUTF8("\x65\xed\xad\xbf\xed\xb0\x80\x66"));
    BOOST_CHECK_EQUAL("\x67\x3f\x3f\x3f\x3f\x3f\x3f\x68",
            SanitizeInvalidUTF8("\x67\xed\xad\xbf\xed\xbf\xbf\x68"));
    BOOST_CHECK_EQUAL("\x69\x3f\x3f\x3f\x3f\x3f\x3f\x70",
            SanitizeInvalidUTF8("\x69\xed\xae\x80\xed\xb0\x80\x70"));
    BOOST_CHECK_EQUAL("\x71\x3f\x3f\x3f\x3f\x3f\x3f\x72\x73",
            SanitizeInvalidUTF8("\x71\xed\xae\x80\xed\xbf\xbf\x72\x73"));
    BOOST_CHECK_EQUAL("\x3f\x3f\x3f\x3f\x3f\x3f\x74\x75\x76\x77",
            SanitizeInvalidUTF8("\xed\xaf\xbf\xed\xb0\x80\x74\x75\x76\x77"));
    BOOST_CHECK_EQUAL("\x3f\x3f\x3f\x3f\x3f\x3f\x64\x65\x78\x78",
            SanitizeInvalidUTF8("\xed\xaf\xbf\xed\xbf\xbf\x64\x65\x78\x78"));
}

BOOST_AUTO_TEST_CASE(valid_ascii)
{
    BOOST_CHECK_EQUAL("",
            SanitizeInvalidUTF8(""));
    BOOST_CHECK_EQUAL("!\"#$%&'()*+,-./0123456789:;<=>?",
            SanitizeInvalidUTF8("!\"#$%&'()*+,-./0123456789:;<=>?"));
    BOOST_CHECK_EQUAL("@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_",
            SanitizeInvalidUTF8("@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"));
    BOOST_CHECK_EQUAL("`abcdefghijklmnopqrstuvwxyz{|}~",
            SanitizeInvalidUTF8("`abcdefghijklmnopqrstuvwxyz{|}~"));
}

BOOST_AUTO_TEST_CASE(omni_getproperty_2147483662)
{
    BOOST_CHECK_EQUAL(
            "{"
            "    \"propertyid\" : 2147483662,"
            "    \"name\" : \"Test?\","
            "    \"category\" : \"Test?\","
            "    \"subcategory\" : \"Test?\","
            "    \"data\" : \"n/a\","
            "    \"url\" : \"n/a\","
            "    \"divisible\" : false,"
            "    \"issuer\" : \"1EHdm4svRkVHf9vu7EvJ6aWjwhw7sHUchN\","
            "    \"creationtxid\" : \"9e8ffcbdc021ffef16f7f694220a3fb18e037e7ecc53ce6039f9841dfa410cbd\","
            "    \"fixedissuance\" : false,"
            "    \"totaltokens\" : \"63\""
            "}",
        SanitizeInvalidUTF8(
            "{"
            "    \"propertyid\" : 2147483662,"
            "    \"name\" : \"Test\x8b\","
            "    \"category\" : \"Test\x8b\","
            "    \"subcategory\" : \"Test\x8b\","
            "    \"data\" : \"n/a\","
            "    \"url\" : \"n/a\","
            "    \"divisible\" : false,"
            "    \"issuer\" : \"1EHdm4svRkVHf9vu7EvJ6aWjwhw7sHUchN\","
            "    \"creationtxid\" : \"9e8ffcbdc021ffef16f7f694220a3fb18e037e7ecc53ce6039f9841dfa410cbd\","
            "    \"fixedissuance\" : false,"
            "    \"totaltokens\" : \"63\""
            "}"
    ));
}

BOOST_AUTO_TEST_CASE(various_valid_strings)
{
    BOOST_CHECK_EQUAL("a€b€c€d@",
            SanitizeInvalidUTF8("a€b€c€d@"));
    BOOST_CHECK_EQUAL("𝄞s2345@äö€€",
            SanitizeInvalidUTF8("𝄞s2345@äö€€"));
    BOOST_CHECK_EQUAL("κόσμε",
            SanitizeInvalidUTF8("κόσμε"));
    BOOST_CHECK_EQUAL("zß水𝄋𝄞",
            SanitizeInvalidUTF8("zß水𝄋𝄞"));
    BOOST_CHECK_EQUAL("—–",
            SanitizeInvalidUTF8("—–"));
    BOOST_CHECK_EQUAL("“, ”",
            SanitizeInvalidUTF8("“, ”"));
    BOOST_CHECK_EQUAL("кракозя́бры, IPA:krɐkɐˈzʲæbrɪ̈",
            SanitizeInvalidUTF8("кракозя́бры, IPA:krɐkɐˈzʲæbrɪ̈"));
    BOOST_CHECK_EQUAL("маймуница",
            SanitizeInvalidUTF8("маймуница"));
    BOOST_CHECK_EQUAL("Кракозябры",
            SanitizeInvalidUTF8("Кракозябры"));
    BOOST_CHECK_EQUAL("ђубре",
            SanitizeInvalidUTF8("ђубре"));
    BOOST_CHECK_EQUAL("å, ä and ö in Finnish and Swedish",
            SanitizeInvalidUTF8("å, ä and ö in Finnish and Swedish"));
    BOOST_CHECK_EQUAL("à, ç, è, é, ï, í, ò, ó, ú, ü in Catalan",
            SanitizeInvalidUTF8("à, ç, è, é, ï, í, ò, ó, ú, ü in Catalan"));
    BOOST_CHECK_EQUAL("å, æ and ø in Norwegian and Danish",
            SanitizeInvalidUTF8("å, æ and ø in Norwegian and Danish"));
    BOOST_CHECK_EQUAL("á, é, ó, ý, è, ë, ï in Dutch",
            SanitizeInvalidUTF8("á, é, ó, ý, è, ë, ï in Dutch"));
    BOOST_CHECK_EQUAL("ä, ö, ü and ß in German",
            SanitizeInvalidUTF8("ä, ö, ü and ß in German"));
    BOOST_CHECK_EQUAL("á, ð, í, ó, ú, ý, æ and ø in Faroese",
            SanitizeInvalidUTF8("á, ð, í, ó, ú, ý, æ and ø in Faroese"));
    BOOST_CHECK_EQUAL("á, ð, é, í, ó, ú, ý, þ, æ and ö in Icelandic",
            SanitizeInvalidUTF8("á, ð, é, í, ó, ú, ý, þ, æ and ö in Icelandic"));
    BOOST_CHECK_EQUAL("à, â, ç, è, é, ë, ê, ï, î, ö, ô, ù, û, ÿ, æ, œ in French",
            SanitizeInvalidUTF8("à, â, ç, è, é, ë, ê, ï, î, ö, ô, ù, û, ÿ, æ, œ in French"));
    BOOST_CHECK_EQUAL("à, è, é, ì, ò, ù in Italian",
            SanitizeInvalidUTF8("à, è, é, ì, ò, ù in Italian"));
    BOOST_CHECK_EQUAL("á, é, í, ñ, ó, ú, ï, ü, ¡, ¿ in Spanish",
            SanitizeInvalidUTF8("á, é, í, ñ, ó, ú, ï, ü, ¡, ¿ in Spanish"));
    BOOST_CHECK_EQUAL("à, á, â, ã, ç, é, ê, í, ó, ô, õ, ú, ü in Portuguese",
            SanitizeInvalidUTF8("à, á, â, ã, ç, é, ê, í, ó, ô, õ, ú, ü in Portuguese"));
    BOOST_CHECK_EQUAL("£ in British English",
            SanitizeInvalidUTF8("£ in British English"));
    BOOST_CHECK_EQUAL("kärlekk舐lekkärlek",
            SanitizeInvalidUTF8("kärlekk舐lekkärlek"));
    BOOST_CHECK_EQUAL("å, ä or ö such as än",
            SanitizeInvalidUTF8("å, ä or ö such as än"));
    BOOST_CHECK_EQUAL("š, đ, č, ć, ž, Š, Đ, Č, Ć, Ž",
            SanitizeInvalidUTF8("š, đ, č, ć, ž, Š, Đ, Č, Ć, Ž"));
    BOOST_CHECK_EQUAL("šđčćž ŠĐČĆŽ",
            SanitizeInvalidUTF8("šđčćž ŠĐČĆŽ"));
    BOOST_CHECK_EQUAL("ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP",
            SanitizeInvalidUTF8("ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP"));
    BOOST_CHECK_EQUAL("الإعلان العالمى لحقوق الإنسان",
            SanitizeInvalidUTF8("الإعلان العالمى لحقوق الإنسان"));
    BOOST_CHECK_EQUAL("Smörgås",
            SanitizeInvalidUTF8("Smörgås"));
    BOOST_CHECK_EQUAL("化け",
            SanitizeInvalidUTF8("化け"));
    BOOST_CHECK_EQUAL("文字化け",
            SanitizeInvalidUTF8("文字化け"));
    BOOST_CHECK_EQUAL("乱码 亂碼 王建煊 游锡堃 游錫堃 朱镕基 陶喆",
            SanitizeInvalidUTF8("乱码 亂碼 王建煊 游锡堃 游錫堃 朱镕基 陶喆"));
    BOOST_CHECK_EQUAL("\xef\xbf\xbd",
            SanitizeInvalidUTF8("\xef\xbf\xbd"));
    BOOST_CHECK_EQUAL("\x68\xef\xbf\xbd\xef\xbf\xbd\x61",
            SanitizeInvalidUTF8("\x68\xef\xbf\xbd\xef\xbf\xbd\x61"));
}


BOOST_AUTO_TEST_SUITE_END()
