// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sha2.h"
#include "util.h"

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(sha2_tests)

void SHA256TestVector(const std::string &in, const std::string &out) {
    std::vector<unsigned char> hash;
    hash.resize(32);
    CSHA256().Write((unsigned char*)&in[0], in.size()).Finalize(&hash[0]);
    BOOST_CHECK_EQUAL(HexStr(hash), out);
}

void SHA512TestVector(const std::string &in, const std::string &out) {
    std::vector<unsigned char> hash;
    hash.resize(64);
    CSHA512().Write((unsigned char*)&in[0], in.size()).Finalize(&hash[0]);
    BOOST_CHECK_EQUAL(HexStr(hash), out);
}

BOOST_AUTO_TEST_CASE(sha256_testvectors) {
    SHA256TestVector("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    SHA256TestVector("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    SHA256TestVector("message digest", "f7846f55cf23e14eebeab5b4e1550cad5b509e3348fbc4efa3a1413d393cb650");
    SHA256TestVector("secure hash algorithm", "f30ceb2bb2829e79e4ca9753d35a8ecc00262d164cc077080295381cbd643f0d");
    SHA256TestVector("SHA256 is considered to be safe", "6819d915c73f4d1e77e4e1b52d1fa0f9cf9beaead3939f15874bd988e2a23630");
    SHA256TestVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    SHA256TestVector("For this sample, this 63-byte string will be used as input data", "f08a78cbbaee082b052ae0708f32fa1e50c5c421aa772ba5dbb406a2ea6be342");
    SHA256TestVector("This is exactly 64 bytes long, not counting the terminating byte", "ab64eff7e88e2e46165e29f2bce41826bd4c7b3552f6b382a9e7d3af47c245f8");
    SHA256TestVector("As Bitcoin relies on 80 byte header hashes, we want to have an example for that.", "7406e8de7d6e4fffc573daef05aefb8806e7790f55eab5576f31349743cca743");
}

BOOST_AUTO_TEST_CASE(sha512_testvectors) {
    SHA512TestVector("abc", "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    SHA512TestVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445");
    SHA512TestVector("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909");
    SHA512TestVector(std::string(1000000, 'a'), "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b");
    SHA512TestVector("", "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
}

typedef struct {
    const char *pszKey;
    const char *pszData;
    const char *pszMAC;
} testvec_t;

// test cases 1, 2, 3, 4, 6 and 7 of RFC 4231
static const testvec_t vtest[] = {
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"
        "0b0b0b0b",
        "4869205468657265",
        "87aa7cdea5ef619d4ff0b4241a1d6cb0"
        "2379f4e2ce4ec2787ad0b30545e17cde"
        "daa833b7d6b8a702038b274eaea3f4e4"
        "be9d914eeb61f1702e696c203a126854"
    },
    {
        "4a656665",
        "7768617420646f2079612077616e7420"
        "666f72206e6f7468696e673f",
        "164b7a7bfcf819e2e395fbe73b56e0a3"
        "87bd64222e831fd610270cd7ea250554"
        "9758bf75c05a994a6d034f65f8f0e6fd"
        "caeab1a34d4a6b4b636e070a38bce737"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaa",
        "dddddddddddddddddddddddddddddddd"
        "dddddddddddddddddddddddddddddddd"
        "dddddddddddddddddddddddddddddddd"
        "dddd",
        "fa73b0089d56a284efb0f0756c890be9"
        "b1b5dbdd8ee81a3655f83e33b2279d39"
        "bf3e848279a722c806b485a47e67c807"
        "b946a337bee8942674278859e13292fb"
    },
    {
        "0102030405060708090a0b0c0d0e0f10"
        "111213141516171819",
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcd",
        "b0ba465637458c6990e5a8c5f61d4af7"
        "e576d97ff94b872de76f8050361ee3db"
        "a91ca5c11aa25eb4d679275cc5788063"
        "a5f19741120c4f2de2adebeb10a298dd"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaa",
        "54657374205573696e67204c61726765"
        "72205468616e20426c6f636b2d53697a"
        "65204b6579202d2048617368204b6579"
        "204669727374",
        "80b24263c7c1a3ebb71493c1dd7be8b4"
        "9b46d1f41b4aeec1121b013783f8f352"
        "6b56d037e05f2598bd0fd2215d6a1e52"
        "95e64f73f63f0aec8b915a985d786598"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaa",
        "54686973206973206120746573742075"
        "73696e672061206c6172676572207468"
        "616e20626c6f636b2d73697a65206b65"
        "7920616e642061206c61726765722074"
        "68616e20626c6f636b2d73697a652064"
        "6174612e20546865206b6579206e6565"
        "647320746f2062652068617368656420"
        "6265666f7265206265696e6720757365"
        "642062792074686520484d414320616c"
        "676f726974686d2e",
        "e37b6a775dc87dbaa4dfa9f96e5e3ffd"
        "debd71f8867289865df5a32d20cdc944"
        "b6022cac3c4982b10d5eeb55c3e4de15"
        "134676fb6de0446065c97440fa8c6a58"
    }
};

BOOST_AUTO_TEST_CASE(hmacsha512_testvectors)
{
    for (unsigned int n=0; n<sizeof(vtest)/sizeof(vtest[0]); n++)
    {
        std::vector<unsigned char> vchKey  = ParseHex(vtest[n].pszKey);
        std::vector<unsigned char> vchData = ParseHex(vtest[n].pszData);
        std::vector<unsigned char> vchMAC  = ParseHex(vtest[n].pszMAC);
        unsigned char vchTemp[64];
        CHMAC_SHA512(&vchKey[0], vchKey.size()).Write(&vchData[0], vchData.size()).Finalize(&vchTemp[0]);
        BOOST_CHECK(memcmp(&vchTemp[0], &vchMAC[0], 64) == 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
