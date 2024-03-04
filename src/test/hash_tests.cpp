// Copyright (c) 2013-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(hash_tests)

BOOST_AUTO_TEST_CASE(murmurhash3)
{

#define T(expected, seed, data) BOOST_CHECK_EQUAL(MurmurHash3(seed, ParseHex(data)), expected)

    // Test MurmurHash3 with various inputs. Of course this is retested in the
    // bloom filter tests - they would fail if MurmurHash3() had any problems -
    // but is useful for those trying to implement Bitcoin libraries as a
    // source of test data for their MurmurHash3() primitive during
    // development.
    //
    // The magic number 0xFBA4C795 comes from CBloomFilter::Hash()

    T(0x00000000U, 0x00000000, "");
    T(0x6a396f08U, 0xFBA4C795, "");
    T(0x81f16f39U, 0xffffffff, "");

    T(0x514e28b7U, 0x00000000, "00");
    T(0xea3f0b17U, 0xFBA4C795, "00");
    T(0xfd6cf10dU, 0x00000000, "ff");

    T(0x16c6b7abU, 0x00000000, "0011");
    T(0x8eb51c3dU, 0x00000000, "001122");
    T(0xb4471bf8U, 0x00000000, "00112233");
    T(0xe2301fa8U, 0x00000000, "0011223344");
    T(0xfc2e4a15U, 0x00000000, "001122334455");
    T(0xb074502cU, 0x00000000, "00112233445566");
    T(0x8034d2a0U, 0x00000000, "0011223344556677");
    T(0xb4698defU, 0x00000000, "001122334455667788");

#undef T
}

/*
   SipHash-2-4 output with
   k = 00 01 02 ...
   and
   in = (empty string)
   in = 00 (1 byte)
   in = 00 01 (2 bytes)
   in = 00 01 02 (3 bytes)
   ...
   in = 00 01 02 ... 3e (63 bytes)

   from: https://131002.net/siphash/siphash24.c
*/
uint64_t siphash_4_2_testvec[] = {
    0x726fdb47dd0e0e31, 0x74f839c593dc67fd, 0x0d6c8009d9a94f5a, 0x85676696d7fb7e2d,
    0xcf2794e0277187b7, 0x18765564cd99a68d, 0xcbc9466e58fee3ce, 0xab0200f58b01d137,
    0x93f5f5799a932462, 0x9e0082df0ba9e4b0, 0x7a5dbbc594ddb9f3, 0xf4b32f46226bada7,
    0x751e8fbc860ee5fb, 0x14ea5627c0843d90, 0xf723ca908e7af2ee, 0xa129ca6149be45e5,
    0x3f2acc7f57c29bdb, 0x699ae9f52cbe4794, 0x4bc1b3f0968dd39c, 0xbb6dc91da77961bd,
    0xbed65cf21aa2ee98, 0xd0f2cbb02e3b67c7, 0x93536795e3a33e88, 0xa80c038ccd5ccec8,
    0xb8ad50c6f649af94, 0xbce192de8a85b8ea, 0x17d835b85bbb15f3, 0x2f2e6163076bcfad,
    0xde4daaaca71dc9a5, 0xa6a2506687956571, 0xad87a3535c49ef28, 0x32d892fad841c342,
    0x7127512f72f27cce, 0xa7f32346f95978e3, 0x12e0b01abb051238, 0x15e034d40fa197ae,
    0x314dffbe0815a3b4, 0x027990f029623981, 0xcadcd4e59ef40c4d, 0x9abfd8766a33735c,
    0x0e3ea96b5304a7d0, 0xad0c42d6fc585992, 0x187306c89bc215a9, 0xd4a60abcf3792b95,
    0xf935451de4f21df2, 0xa9538f0419755787, 0xdb9acddff56ca510, 0xd06c98cd5c0975eb,
    0xe612a3cb9ecba951, 0xc766e62cfcadaf96, 0xee64435a9752fe72, 0xa192d576b245165a,
    0x0a8787bf8ecb74b2, 0x81b3e73d20b49b6f, 0x7fa8220ba3b2ecea, 0x245731c13ca42499,
    0xb78dbfaf3a8d83bd, 0xea1ad565322a1a0b, 0x60e61c23a3795013, 0x6606d7e446282b93,
    0x6ca4ecb15c5f91e1, 0x9f626da15c9625f3, 0xe51b38608ef25f57, 0x958a324ceb064572
};

BOOST_AUTO_TEST_CASE(siphash)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x726fdb47dd0e0e31ull);
    static const unsigned char t0[1] = {0};
    hasher.Write(t0, 1);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x74f839c593dc67fdull);
    static const unsigned char t1[7] = {1,2,3,4,5,6,7};
    hasher.Write(t1, 7);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x93f5f5799a932462ull);
    hasher.Write(0x0F0E0D0C0B0A0908ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x3f2acc7f57c29bdbull);
    static const unsigned char t2[2] = {16,17};
    hasher.Write(t2, 2);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x4bc1b3f0968dd39cull);
    static const unsigned char t3[9] = {18,19,20,21,22,23,24,25,26};
    hasher.Write(t3, 9);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x2f2e6163076bcfadull);
    static const unsigned char t4[5] = {27,28,29,30,31};
    hasher.Write(t4, 5);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x7127512f72f27cceull);
    hasher.Write(0x2726252423222120ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x0e3ea96b5304a7d0ull);
    hasher.Write(0x2F2E2D2C2B2A2928ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0xe612a3cb9ecba951ull);

    BOOST_CHECK_EQUAL(SipHashUint256(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL, uint256S("1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100")), 0x7127512f72f27cceull);

    // Check test vectors from spec, one byte at a time
    CSipHasher hasher2(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    for (uint8_t x=0; x<std::size(siphash_4_2_testvec); ++x)
    {
        BOOST_CHECK_EQUAL(hasher2.Finalize(), siphash_4_2_testvec[x]);
        hasher2.Write(&x, 1);
    }
    // Check test vectors from spec, eight bytes at a time
    CSipHasher hasher3(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    for (uint8_t x=0; x<std::size(siphash_4_2_testvec); x+=8)
    {
        BOOST_CHECK_EQUAL(hasher3.Finalize(), siphash_4_2_testvec[x]);
        hasher3.Write(uint64_t(x)|(uint64_t(x+1)<<8)|(uint64_t(x+2)<<16)|(uint64_t(x+3)<<24)|
                     (uint64_t(x+4)<<32)|(uint64_t(x+5)<<40)|(uint64_t(x+6)<<48)|(uint64_t(x+7)<<56));
    }

    CHashWriter ss(SER_DISK, CLIENT_VERSION);
    CMutableTransaction tx;
    // Note these tests were originally written with tx.nVersion=1
    // and the test would be affected by default tx version bumps if not fixed.
    tx.nVersion = 1;
    ss << tx;

    // Check consistency between CSipHasher and SipHashUint256[Extra].
    FastRandomContext ctx;
    for (int i = 0; i < 16; ++i) {
        uint64_t k1 = ctx.rand64();
        uint64_t k2 = ctx.rand64();
        uint256 x = InsecureRand256();
        uint32_t n = ctx.rand32();
        uint8_t nb[4];
        WriteLE32(nb, n);
        CSipHasher sip256(k1, k2);
        sip256.Write(x.begin(), 32);
        CSipHasher sip288 = sip256;
        sip288.Write(nb, 4);
        BOOST_CHECK_EQUAL(SipHashUint256(k1, k2, x), sip256.Finalize());
        BOOST_CHECK_EQUAL(SipHashUint256Extra(k1, k2, x, n), sip288.Finalize());
    }

    BOOST_CHECK_EQUAL(SipHashUint256(1, 2, ss.GetHash()), 0x79751e980c2a0a35ULL);
}

BOOST_AUTO_TEST_SUITE_END()
