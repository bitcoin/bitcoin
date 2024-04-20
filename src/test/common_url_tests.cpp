// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/url.h>

#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(common_url_tests)

// These test vectors were ported from test/regress.c in the libevent library
// which used to be a dependency of the urlDecode function.

BOOST_AUTO_TEST_CASE(encode_decode_test) {
    BOOST_CHECK_EQUAL(urlDecode("Hello"), "Hello");
    BOOST_CHECK_EQUAL(urlDecode("99"), "99");
    BOOST_CHECK_EQUAL(urlDecode(""), "");
    BOOST_CHECK_EQUAL(urlDecode("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_"),
                      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_");
    BOOST_CHECK_EQUAL(urlDecode("%20"), " ");
    BOOST_CHECK_EQUAL(urlDecode("%FF%F0%E0"), "\xff\xf0\xe0");
    BOOST_CHECK_EQUAL(urlDecode("%01%19"), "\x01\x19");
    BOOST_CHECK_EQUAL(urlDecode("http%3A%2F%2Fwww.ietf.org%2Frfc%2Frfc3986.txt"),
                      "http://www.ietf.org/rfc/rfc3986.txt");
    BOOST_CHECK_EQUAL(urlDecode("1%2B2%3D3"), "1+2=3");
}

BOOST_AUTO_TEST_CASE(decode_malformed_test) {
    BOOST_CHECK_EQUAL(urlDecode("%%xhello th+ere \xff"), "%%xhello th+ere \xff");

    BOOST_CHECK_EQUAL(urlDecode("%"), "%");
    BOOST_CHECK_EQUAL(urlDecode("%%"), "%%");
    BOOST_CHECK_EQUAL(urlDecode("%%%"), "%%%");
    BOOST_CHECK_EQUAL(urlDecode("%%%%"), "%%%%");

    BOOST_CHECK_EQUAL(urlDecode("+"), "+");
    BOOST_CHECK_EQUAL(urlDecode("++"), "++");

    BOOST_CHECK_EQUAL(urlDecode("?"), "?");
    BOOST_CHECK_EQUAL(urlDecode("??"), "??");

    BOOST_CHECK_EQUAL(urlDecode("%G1"), "%G1");
    BOOST_CHECK_EQUAL(urlDecode("%2"), "%2");
    BOOST_CHECK_EQUAL(urlDecode("%ZX"), "%ZX");

    BOOST_CHECK_EQUAL(urlDecode("valid%20string%G1"), "valid string%G1");
    BOOST_CHECK_EQUAL(urlDecode("%20invalid%ZX"), " invalid%ZX");
    BOOST_CHECK_EQUAL(urlDecode("%20%G1%ZX"), " %G1%ZX");

    BOOST_CHECK_EQUAL(urlDecode("%1 "), "%1 ");
    BOOST_CHECK_EQUAL(urlDecode("% 9"), "% 9");
    BOOST_CHECK_EQUAL(urlDecode(" %Z "), " %Z ");
    BOOST_CHECK_EQUAL(urlDecode(" % X"), " % X");

    BOOST_CHECK_EQUAL(urlDecode("%-1"), "%-1");
    BOOST_CHECK_EQUAL(urlDecode("%1-"), "%1-");
}

BOOST_AUTO_TEST_CASE(decode_lowercase_hex_test) {
    BOOST_CHECK_EQUAL(urlDecode("%f0%a0%b0"), "\xf0\xa0\xb0");
}

BOOST_AUTO_TEST_CASE(decode_internal_nulls_test) {
    BOOST_CHECK_EQUAL(urlDecode("%00%00x%00%00"), "");
    BOOST_CHECK_EQUAL(urlDecode("abc%00%00"), "abc");
}

BOOST_AUTO_TEST_SUITE_END()
