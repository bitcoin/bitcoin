// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/url.h>

#include <string>
#include <string_view>

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

BOOST_AUTO_TEST_CASE(url_encode) {
    BOOST_CHECK_EQUAL(UrlEncode(std::string_view{
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
        "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
        "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
        "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
        "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
        "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
        "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
        "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
        "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
        "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
        "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
        "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
        "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
        "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
        "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
        256}),
        "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F"
        "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F"
        "%20%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F"
        "0123456789%3A%3B%3C%3D%3E%3F"
        "%40ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ%5B%5C%5D%5E_"
        "%60abcdefghijklmno"
        "pqrstuvwxyz%7B%7C%7D~%7F"
        "%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F"
        "%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F"
        "%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF"
        "%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF"
        "%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF"
        "%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF"
        "%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF"
        "%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF");
}

BOOST_AUTO_TEST_SUITE_END()
