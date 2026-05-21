// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/url.h>

#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(common_url_tests)

// These test vectors were ported from test/regress.c in the libevent library
// which used to be a dependency of the UrlDecode function.

BOOST_AUTO_TEST_CASE(encode_decode_test) {
    BOOST_CHECK_EQUAL(UrlDecode("Hello"), "Hello");
    BOOST_CHECK_EQUAL(UrlDecode("99"), "99");
    BOOST_CHECK_EQUAL(UrlDecode(""), "");
    BOOST_CHECK_EQUAL(UrlDecode("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_"),
                      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_");
    BOOST_CHECK_EQUAL(UrlDecode("%20"), " ");
    BOOST_CHECK_EQUAL(UrlDecode("%FF%F0%E0"), "\xff\xf0\xe0");
    BOOST_CHECK_EQUAL(UrlDecode("%01%19"), "\x01\x19");
    BOOST_CHECK_EQUAL(UrlDecode("http%3A%2F%2Fwww.ietf.org%2Frfc%2Frfc3986.txt"),
                      "http://www.ietf.org/rfc/rfc3986.txt");
    BOOST_CHECK_EQUAL(UrlDecode("1%2B2%3D3"), "1+2=3");
}

BOOST_AUTO_TEST_CASE(decode_malformed_test) {
    BOOST_CHECK_EQUAL(UrlDecode("%%xhello th+ere \xff"), "%%xhello th+ere \xff");

    BOOST_CHECK_EQUAL(UrlDecode("%"), "%");
    BOOST_CHECK_EQUAL(UrlDecode("%%"), "%%");
    BOOST_CHECK_EQUAL(UrlDecode("%%%"), "%%%");
    BOOST_CHECK_EQUAL(UrlDecode("%%%%"), "%%%%");

    BOOST_CHECK_EQUAL(UrlDecode("+"), "+");
    BOOST_CHECK_EQUAL(UrlDecode("++"), "++");

    BOOST_CHECK_EQUAL(UrlDecode("?"), "?");
    BOOST_CHECK_EQUAL(UrlDecode("??"), "??");

    BOOST_CHECK_EQUAL(UrlDecode("%G1"), "%G1");
    BOOST_CHECK_EQUAL(UrlDecode("%2"), "%2");
    BOOST_CHECK_EQUAL(UrlDecode("%ZX"), "%ZX");

    BOOST_CHECK_EQUAL(UrlDecode("valid%20string%G1"), "valid string%G1");
    BOOST_CHECK_EQUAL(UrlDecode("%20invalid%ZX"), " invalid%ZX");
    BOOST_CHECK_EQUAL(UrlDecode("%20%G1%ZX"), " %G1%ZX");

    BOOST_CHECK_EQUAL(UrlDecode("%1 "), "%1 ");
    BOOST_CHECK_EQUAL(UrlDecode("% 9"), "% 9");
    BOOST_CHECK_EQUAL(UrlDecode(" %Z "), " %Z ");
    BOOST_CHECK_EQUAL(UrlDecode(" % X"), " % X");

    BOOST_CHECK_EQUAL(UrlDecode("%%ffg"), "%\xffg");
    BOOST_CHECK_EQUAL(UrlDecode("%fg"), "%fg");

    BOOST_CHECK_EQUAL(UrlDecode("%-1"), "%-1");
    BOOST_CHECK_EQUAL(UrlDecode("%1-"), "%1-");
}

BOOST_AUTO_TEST_CASE(decode_lowercase_hex_test) {
    BOOST_CHECK_EQUAL(UrlDecode("%f0%a0%b0"), "\xf0\xa0\xb0");
}

BOOST_AUTO_TEST_CASE(decode_internal_nulls_test) {
    std::string result1{"\0\0x\0\0", 5};
    BOOST_CHECK_EQUAL(UrlDecode("%00%00x%00%00"), result1);
    std::string result2{"abc\0\0", 5};
    BOOST_CHECK_EQUAL(UrlDecode("abc%00%00"), result2);
}

BOOST_AUTO_TEST_SUITE_END()
