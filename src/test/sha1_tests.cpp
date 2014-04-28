// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sha1.h"
#include "util.h"

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(sha1_tests)

void SHA1TestVector(const std::string &in, const std::string &out) {
    std::vector<unsigned char> hash;
    hash.resize(20);
    CSHA1().Write((unsigned char*)&in[0], in.size()).Finalize(&hash[0]);
    BOOST_CHECK_EQUAL(HexStr(hash), out);
}

BOOST_AUTO_TEST_CASE(sha1_testvectors) {
    SHA1TestVector("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
    SHA1TestVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    SHA1TestVector(std::string(1000000, 'a'), "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

BOOST_AUTO_TEST_SUITE_END()
