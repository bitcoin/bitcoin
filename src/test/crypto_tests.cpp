// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto/ripemd160.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/sha512.h"
#include "crypto/hmac_sha256.h"
#include "crypto/hmac_sha512.h"
#include "random.h"
#include "utilstrencodings.h"
#include "test/test_dash.h"

#include <vector>

#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

#include <openssl/evp.h>

BOOST_FIXTURE_TEST_SUITE(crypto_tests, BasicTestingSetup)

template<typename Hasher, typename In, typename Out>
void TestVector(const Hasher &h, const In &in, const Out &out) {
    Out hash;
    BOOST_CHECK(out.size() == h.OUTPUT_SIZE);
    hash.resize(out.size());
    {
        // Test that writing the whole input string at once works.
        Hasher(h).Write((unsigned char*)&in[0], in.size()).Finalize(&hash[0]);
        BOOST_CHECK(hash == out);
    }
    for (int i=0; i<32; i++) {
        // Test that writing the string broken up in random pieces works.
        Hasher hasher(h);
        size_t pos = 0;
        while (pos < in.size()) {
            size_t len = insecure_rand() % ((in.size() - pos + 1) / 2 + 1);
            hasher.Write((unsigned char*)&in[pos], len);
            pos += len;
            if (pos > 0 && pos + 2 * out.size() > in.size() && pos < in.size()) {
                // Test that writing the rest at once to a copy of a hasher works.
                Hasher(hasher).Write((unsigned char*)&in[pos], in.size() - pos).Finalize(&hash[0]);
                BOOST_CHECK(hash == out);
            }
        }
        hasher.Finalize(&hash[0]);
        BOOST_CHECK(hash == out);
    }
}

void TestSHA1(const std::string &in, const std::string &hexout) { TestVector(CSHA1(), in, ParseHex(hexout));}
void TestSHA256(const std::string &in, const std::string &hexout) { TestVector(CSHA256(), in, ParseHex(hexout));}
void TestSHA512(const std::string &in, const std::string &hexout) { TestVector(CSHA512(), in, ParseHex(hexout));}
void TestRIPEMD160(const std::string &in, const std::string &hexout) { TestVector(CRIPEMD160(), in, ParseHex(hexout));}

void TestHMACSHA256(const std::string &hexkey, const std::string &hexin, const std::string &hexout) {
    std::vector<unsigned char> key = ParseHex(hexkey);
    TestVector(CHMAC_SHA256(&key[0], key.size()), ParseHex(hexin), ParseHex(hexout));
}

void TestHMACSHA512(const std::string &hexkey, const std::string &hexin, const std::string &hexout) {
    std::vector<unsigned char> key = ParseHex(hexkey);
    TestVector(CHMAC_SHA512(&key[0], key.size()), ParseHex(hexin), ParseHex(hexout));
}

std::string LongTestString(void) {
    std::string ret;
    for (int i=0; i<200000; i++) {
        ret += (unsigned char)(i);
        ret += (unsigned char)(i >> 4);
        ret += (unsigned char)(i >> 8);
        ret += (unsigned char)(i >> 12);
        ret += (unsigned char)(i >> 16);
    }
    return ret;
}

const std::string test1 = LongTestString();

BOOST_AUTO_TEST_CASE(ripemd160_testvectors) {
    TestRIPEMD160("", "9c1185a5c5e9fc54612808977ee8f548b2258d31");
    TestRIPEMD160("abc", "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
    TestRIPEMD160("message digest", "5d0689ef49d2fae572b881b123a85ffa21595f36");
    TestRIPEMD160("secure hash algorithm", "20397528223b6a5f4cbc2808aba0464e645544f9");
    TestRIPEMD160("RIPEMD160 is considered to be safe", "a7d78608c7af8a8e728778e81576870734122b66");
    TestRIPEMD160("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                  "12a053384a9c0c88e405a06c27dcf49ada62eb2b");
    TestRIPEMD160("For this sample, this 63-byte string will be used as input data",
                  "de90dbfee14b63fb5abf27c2ad4a82aaa5f27a11");
    TestRIPEMD160("This is exactly 64 bytes long, not counting the terminating byte",
                  "eda31d51d3a623b81e19eb02e24ff65d27d67b37");
    TestRIPEMD160(std::string(1000000, 'a'), "52783243c1697bdbe16d37f97f68f08325dc1528");
    TestRIPEMD160(test1, "464243587bd146ea835cdf57bdae582f25ec45f1");
}

BOOST_AUTO_TEST_CASE(sha1_testvectors) {
    TestSHA1("", "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    TestSHA1("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
    TestSHA1("message digest", "c12252ceda8be8994d5fa0290a47231c1d16aae3");
    TestSHA1("secure hash algorithm", "d4d6d2f0ebe317513bbd8d967d89bac5819c2f60");
    TestSHA1("SHA1 is considered to be safe", "f2b6650569ad3a8720348dd6ea6c497dee3a842a");
    TestSHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
             "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    TestSHA1("For this sample, this 63-byte string will be used as input data",
             "4f0ea5cd0585a23d028abdc1a6684e5a8094dc49");
    TestSHA1("This is exactly 64 bytes long, not counting the terminating byte",
             "fb679f23e7d1ce053313e66e127ab1b444397057");
    TestSHA1(std::string(1000000, 'a'), "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
    TestSHA1(test1, "b7755760681cbfd971451668f32af5774f4656b5");
}

BOOST_AUTO_TEST_CASE(sha256_testvectors) {
    TestSHA256("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    TestSHA256("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    TestSHA256("message digest",
               "f7846f55cf23e14eebeab5b4e1550cad5b509e3348fbc4efa3a1413d393cb650");
    TestSHA256("secure hash algorithm",
               "f30ceb2bb2829e79e4ca9753d35a8ecc00262d164cc077080295381cbd643f0d");
    TestSHA256("SHA256 is considered to be safe",
               "6819d915c73f4d1e77e4e1b52d1fa0f9cf9beaead3939f15874bd988e2a23630");
    TestSHA256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
               "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    TestSHA256("For this sample, this 63-byte string will be used as input data",
               "f08a78cbbaee082b052ae0708f32fa1e50c5c421aa772ba5dbb406a2ea6be342");
    TestSHA256("This is exactly 64 bytes long, not counting the terminating byte",
               "ab64eff7e88e2e46165e29f2bce41826bd4c7b3552f6b382a9e7d3af47c245f8");
    TestSHA256("As Bitcoin relies on 80 byte header hashes, we want to have an example for that.",
               "7406e8de7d6e4fffc573daef05aefb8806e7790f55eab5576f31349743cca743");
    TestSHA256(std::string(1000000, 'a'),
               "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    TestSHA256(test1, "a316d55510b49662420f49d145d42fb83f31ef8dc016aa4e32df049991a91e26");
}

BOOST_AUTO_TEST_CASE(sha512_testvectors) {
    TestSHA512("",
               "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
               "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    TestSHA512("abc",
               "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
               "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    TestSHA512("message digest",
               "107dbf389d9e9f71a3a95f6c055b9251bc5268c2be16d6c13492ea45b0199f33"
               "09e16455ab1e96118e8a905d5597b72038ddb372a89826046de66687bb420e7c");
    TestSHA512("secure hash algorithm",
               "7746d91f3de30c68cec0dd693120a7e8b04d8073cb699bdce1a3f64127bca7a3"
               "d5db502e814bb63c063a7a5043b2df87c61133395f4ad1edca7fcf4b30c3236e");
    TestSHA512("SHA512 is considered to be safe",
               "099e6468d889e1c79092a89ae925a9499b5408e01b66cb5b0a3bd0dfa51a9964"
               "6b4a3901caab1318189f74cd8cf2e941829012f2449df52067d3dd5b978456c2");
    TestSHA512("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
               "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c335"
               "96fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445");
    TestSHA512("For this sample, this 63-byte string will be used as input data",
               "b3de4afbc516d2478fe9b518d063bda6c8dd65fc38402dd81d1eb7364e72fb6e"
               "6663cf6d2771c8f5a6da09601712fb3d2a36c6ffea3e28b0818b05b0a8660766");
    TestSHA512("This is exactly 64 bytes long, not counting the terminating byte",
               "70aefeaa0e7ac4f8fe17532d7185a289bee3b428d950c14fa8b713ca09814a38"
               "7d245870e007a80ad97c369d193e41701aa07f3221d15f0e65a1ff970cedf030");
    TestSHA512("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
               "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
               "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
               "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909");
    TestSHA512(std::string(1000000, 'a'),
               "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973eb"
               "de0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b");
    TestSHA512(test1,
               "40cac46c147e6131c5193dd5f34e9d8bb4951395f27b08c558c65ff4ba2de594"
               "37de8c3ef5459d76a52cedc02dc499a3c9ed9dedbfb3281afd9653b8a112fafc");
}

BOOST_AUTO_TEST_CASE(hmac_sha256_testvectors) {
    // test cases 1, 2, 3, 4, 6 and 7 of RFC 4231
    TestHMACSHA256("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                   "4869205468657265",
                   "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    TestHMACSHA256("4a656665",
                   "7768617420646f2079612077616e7420666f72206e6f7468696e673f",
                   "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    TestHMACSHA256("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                   "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
                   "dddddddddddddddddddddddddddddddddddd",
                   "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
    TestHMACSHA256("0102030405060708090a0b0c0d0e0f10111213141516171819",
                   "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
                   "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd",
                   "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
    TestHMACSHA256("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaa",
                   "54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a"
                   "65204b6579202d2048617368204b6579204669727374",
                   "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    TestHMACSHA256("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaa",
                   "5468697320697320612074657374207573696e672061206c6172676572207468"
                   "616e20626c6f636b2d73697a65206b657920616e642061206c61726765722074"
                   "68616e20626c6f636b2d73697a6520646174612e20546865206b6579206e6565"
                   "647320746f20626520686173686564206265666f7265206265696e6720757365"
                   "642062792074686520484d414320616c676f726974686d2e",
                   "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
}

BOOST_AUTO_TEST_CASE(hmac_sha512_testvectors) {
    // test cases 1, 2, 3, 4, 6 and 7 of RFC 4231
    TestHMACSHA512("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                   "4869205468657265",
                   "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
                   "daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854");
    TestHMACSHA512("4a656665",
                   "7768617420646f2079612077616e7420666f72206e6f7468696e673f",
                   "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
                   "9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737");
    TestHMACSHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                   "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
                   "dddddddddddddddddddddddddddddddddddd",
                   "fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
                   "bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb");
    TestHMACSHA512("0102030405060708090a0b0c0d0e0f10111213141516171819",
                   "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
                   "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd",
                   "b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3db"
                   "a91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd");
    TestHMACSHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaa",
                   "54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a"
                   "65204b6579202d2048617368204b6579204669727374",
                   "80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f352"
                   "6b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598");
    TestHMACSHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "aaaaaa",
                   "5468697320697320612074657374207573696e672061206c6172676572207468"
                   "616e20626c6f636b2d73697a65206b657920616e642061206c61726765722074"
                   "68616e20626c6f636b2d73697a6520646174612e20546865206b6579206e6565"
                   "647320746f20626520686173686564206265666f7265206265696e6720757365"
                   "642062792074686520484d414320616c676f726974686d2e",
                   "e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944"
                   "b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58");
}

BOOST_AUTO_TEST_CASE(pbkdf2_hmac_sha512_test) {
    // test vectors from
    // https://github.com/trezor/trezor-crypto/blob/87c920a7e747f7ed40b6ae841327868ab914435b/tests.c#L1936-L1957
    // https://stackoverflow.com/questions/15593184/pbkdf2-hmac-sha-512-test-vectors
    uint8_t k[64], s[40];

    strcpy((char *)s, "salt");
    PKCS5_PBKDF2_HMAC("password", 8, s, 4, 1, EVP_sha512(), 64, k);
    BOOST_CHECK(HexStr(k, k + 64) == "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce");

    strcpy((char *)s, "salt");
    PKCS5_PBKDF2_HMAC("password", 8, s, 4, 2, EVP_sha512(), 64, k);
    BOOST_CHECK(HexStr(k, k + 64) == "e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53cf76cab2868a39b9f7840edce4fef5a82be67335c77a6068e04112754f27ccf4e");

    strcpy((char *)s, "salt");
    PKCS5_PBKDF2_HMAC("password", 8, s, 4, 4096, EVP_sha512(), 64, k);
    BOOST_CHECK(HexStr(k, k + 64) == "d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f87f6902e072f457b5143f30602641b3d55cd335988cb36b84376060ecd532e039b742a239434af2d5");

    strcpy((char *)s, "saltSALTsaltSALTsaltSALTsaltSALTsalt");
    PKCS5_PBKDF2_HMAC("passwordPASSWORDpassword", 3*8, s, 9*4, 4096, EVP_sha512(), 64, k);
    BOOST_CHECK(HexStr(k, k + 64) == "8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b868c005174dc4ee71115b59f9e60cd9532fa33e0f75aefe30225c583a186cd82bd4daea9724a3d3b8");
}

BOOST_AUTO_TEST_SUITE_END()
