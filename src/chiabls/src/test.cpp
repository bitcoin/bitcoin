
// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define CATCH_CONFIG_RUNNER
#include <thread>

#include "bls.hpp"
#include "catch.hpp"
extern "C" {
#include "relic.h"
}
#include "test-utils.hpp"
using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace bls;

void TestHKDF(string ikm_hex, string salt_hex, string info_hex, string prk_expected_hex, string okm_expected_hex, int L) {
    vector<uint8_t> ikm = Util::HexToBytes(ikm_hex);
    vector<uint8_t> salt = Util::HexToBytes(salt_hex);
    vector<uint8_t> info = Util::HexToBytes(info_hex);
    vector<uint8_t> prk_expected = Util::HexToBytes(prk_expected_hex);
    vector<uint8_t> okm_expected = Util::HexToBytes(okm_expected_hex);
    uint8_t prk[32];
    HKDF256::Extract(prk, salt.data(), salt.size(), ikm.data(), ikm.size());
    uint8_t okm[L];
    HKDF256::Expand(okm, L, prk, info.data(), info.size());

    REQUIRE(32 == prk_expected.size());
    REQUIRE(L == okm_expected.size());

    for (size_t i=0; i < 32; i++) {
        REQUIRE(prk[i] == prk_expected[i]);
    }
    for (size_t i=0; i < L; i++) {
        REQUIRE(okm[i] == okm_expected[i]);
    }
}

TEST_CASE("class PrivateKey") {
    uint8_t buffer[PrivateKey::PRIVATE_KEY_SIZE];
    memcmp(buffer, getRandomSeed().data(), PrivateKey::PRIVATE_KEY_SIZE);
    SECTION("Copy {constructor|assignment operator}") {
        PrivateKey pk1 = PrivateKey::FromByteVector(getRandomSeed(), true);
        PrivateKey pk2 = PrivateKey::FromByteVector(getRandomSeed(), true);
        PrivateKey pk3 = PrivateKey(pk2);
        REQUIRE(!pk1.IsZero());
        REQUIRE(!pk2.IsZero());
        REQUIRE(!pk3.IsZero());
        REQUIRE(pk1 != pk2);
        REQUIRE(pk3 == pk2);
        pk2 = pk1;
        REQUIRE(pk1 == pk2);
        REQUIRE(pk3 != pk2);
    }
    SECTION("Move {constructor|assignment operator}") {
        PrivateKey pk1 = PrivateKey::FromByteVector(getRandomSeed(), true);
        std::vector<uint8_t> vec1 = pk1.Serialize();
        PrivateKey pk2 = PrivateKey::FromByteVector(getRandomSeed(), true);
        std::vector<uint8_t> vec2 = pk2.Serialize();
        PrivateKey pk3 = PrivateKey(std::move(pk2));
        REQUIRE(!pk1.IsZero());
        REQUIRE_THROWS(pk2.IsZero());
        REQUIRE(!pk3.IsZero());
        REQUIRE(vec2 == pk3.Serialize());
        pk3 = std::move(pk1);
        REQUIRE_THROWS(pk1.IsZero());
        REQUIRE_THROWS(pk2.IsZero());
        REQUIRE(!pk3.IsZero());
        REQUIRE(vec1 == pk3.Serialize());
        pk3 = std::move(pk1);
        REQUIRE_THROWS(pk1.IsZero());
        REQUIRE_THROWS(pk2.IsZero());
        REQUIRE_THROWS(pk3.IsZero());
    }
    SECTION("Equality operators") {
        PrivateKey pk1 = PrivateKey::FromByteVector(getRandomSeed(), true);
        PrivateKey pk2 = PrivateKey::FromByteVector(getRandomSeed(), true);
        PrivateKey pk3 = pk2;
        REQUIRE(pk1 != pk2);
        REQUIRE(pk1 != pk3);
        REQUIRE(pk2 == pk3);
    }
    SECTION("(De)Serialization") {
        PrivateKey pk1 = PrivateKey::FromByteVector(getRandomSeed(), true);
        pk1.Serialize(buffer);
        REQUIRE(memcmp(buffer, pk1.Serialize().data(), PrivateKey::PRIVATE_KEY_SIZE) == 0);
        PrivateKey pk2 = PrivateKey:: FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), true);
        REQUIRE(pk1 == pk2);
        REQUIRE_THROWS(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE - 1), true));
        REQUIRE_THROWS(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE + 1), true));
        REQUIRE_NOTHROW(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), true));
        bn_t order;
        bn_new(order);
        g1_get_ord(order);
        bn_write_bin(buffer, PrivateKey::PRIVATE_KEY_SIZE, order);
        REQUIRE_NOTHROW(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), false));
        REQUIRE_NOTHROW(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), true));
        bn_add(order, order, order);
        bn_write_bin(buffer, PrivateKey::PRIVATE_KEY_SIZE, order);
        REQUIRE_THROWS(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), false));
        REQUIRE_NOTHROW(PrivateKey::FromBytes(Bytes(buffer, PrivateKey::PRIVATE_KEY_SIZE), true));
    }
    SECTION("keydata checks") {
        PrivateKey pk1 = PrivateKey::FromByteVector(getRandomSeed(), true);
        G1Element g1 = pk1.GetG1Element();
        G2Element g2 = pk1.GetG2Element();
        PrivateKey pk2 = std::move(pk1);
        REQUIRE_THROWS(PrivateKey(pk1));
        REQUIRE_THROWS(pk1 = pk2);
        REQUIRE_THROWS(pk1.GetG1Element());
        REQUIRE_THROWS(pk1.GetG2Element());
        REQUIRE_THROWS(g1 * pk1);
        REQUIRE_THROWS(pk1 * g1);
        REQUIRE_THROWS(g2 * pk1);
        REQUIRE_THROWS(pk1 * g2);
        REQUIRE_THROWS(pk1.GetG2Power(g2));
        REQUIRE_THROWS(PrivateKey::Aggregate({pk1, pk2}));
        REQUIRE_THROWS(pk1.IsZero());
        REQUIRE_THROWS(pk1 == pk2);
        REQUIRE_THROWS(pk1 != pk2);
        REQUIRE_THROWS(pk1.Serialize(buffer));
        REQUIRE_THROWS(pk1.Serialize());
        REQUIRE_THROWS(pk1.SignG2(buffer, sizeof(buffer), buffer, sizeof(buffer)));
    }
}

TEST_CASE("HKDF") {
    // https://tools.ietf.org/html/rfc5869 test vectors
    SECTION("Test case 2") {
        TestHKDF("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                 "000102030405060708090a0b0c",
                 "f0f1f2f3f4f5f6f7f8f9",
                 "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
                 "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
                 42
        );
    }
    SECTION("Test case 2") {
        TestHKDF("000102030405060708090a0b0c0d0e0f"
                 "101112131415161718191a1b1c1d1e1f"
                 "202122232425262728292a2b2c2d2e2f"
                 "303132333435363738393a3b3c3d3e3f"
                 "404142434445464748494a4b4c4d4e4f", // 80 octets
                 "0x606162636465666768696a6b6c6d6e6f"
                 "707172737475767778797a7b7c7d7e7f"
                 "808182838485868788898a8b8c8d8e8f"
                 "909192939495969798999a9b9c9d9e9f"
                 "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", // 80 octets
                 "0xb0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
                 "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
                 "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
                 "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
                 "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", // 80 octets
                 "0x06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244", // 32 octets
                 "0xb11e398dc80327a1c8e7f78c596a4934"
                 "4f012eda2d4efad8a050cc4c19afa97c"
                 "59045a99cac7827271cb41c65e590e09"
                 "da3275600c2f09b8367793a9aca3db71"
                 "cc30c58179ec3e87c14c01d5c1f3434f"
                 "1d87", // 82 octets
                 82
        );
    }
    SECTION("Test case 3") {
        TestHKDF("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                 "",
                 "",
                 "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04",
                 "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8",
                 42
        );
    }
    SECTION("Works with multiple of 32") {
        // This generates exactly 64 bytes. Uses a 32 byte key and 4 byte salt as in EIP2333.
        TestHKDF("8704f9ac024139fe62511375cf9bc534c0507dcf00c41603ac935cd5943ce0b4b88599390de14e743ca2f56a73a04eae13aa3f3b969b39d8701e0d69a6f8d42f",
                 "53d8e19b",
                 "",
                 "eb01c9cd916653df76ffa61b6ab8a74e254ebfd9bfc43e624cc12a72b0373dee",
                 "8faabea85fc0c64e7ca86217cdc6dcdc88551c3244d56719e630a3521063082c46455c2fd5483811f9520a748f0099c1dfcfa52c54e1c22b5cdf70efb0f3c676",
                 64
        );
    }
}

void TestEIP2333(string seedHex, string masterSkHex, string childSkHex, uint32_t childIndex) {
    auto masterSk = Util::HexToBytes(masterSkHex);
    auto childSk = Util::HexToBytes(childSkHex);

    PrivateKey master = BasicSchemeMPL().KeyGen(Util::HexToBytes(seedHex));
    PrivateKey child = HDKeys::DeriveChildSk(master, childIndex);

    uint8_t master_arr[32];
    master.Serialize(master_arr);
    auto calculatedMaster = master.Serialize();
    auto calculatedChild = child.Serialize();

    REQUIRE(calculatedMaster.size() == 32);
    REQUIRE(calculatedChild.size() == 32);
    for (int i=0; i<32; i++) {
        REQUIRE(calculatedMaster[i] == masterSk[i]);
    }
    for (int i=0; i<32; i++) {
        REQUIRE(calculatedChild[i] == childSk[i]);
    }
}

TEST_CASE("EIP-2333 hardened HD keys") {
    // The comments in the test cases correspond to  integers that are converted to
    // bytes using python int.to_bytes(32, "big").hex(), since the EIP spec provides ints, but c++
    // does not support bigint by default
    SECTION("EIP-2333 Test case 1"){
        TestEIP2333("3141592653589793238462643383279502884197169399375105820974944592",
                    // 36167147331491996618072159372207345412841461318189449162487002442599770291484
                    "4ff5e145590ed7b71e577bb04032396d1619ff41cb4e350053ed2dce8d1efd1c",
                    // 41787458189896526028601807066547832426569899195138584349427756863968330588237
                    "5c62dcf9654481292aafa3348f1d1b0017bbfb44d6881d26d2b17836b38f204d",
                    3141592653
        );
    }
    SECTION("EIP-2333 Test case 2"){
        TestEIP2333("0x0099FF991111002299DD7744EE3355BBDD8844115566CC55663355668888CC00",
                    // 13904094584487173309420026178174172335998687531503061311232927109397516192843
                    "1ebd704b86732c3f05f30563dee6189838e73998ebc9c209ccff422adee10c4b",
                    // 12482522899285304316694838079579801944734479969002030150864436005368716366140
                    "1b98db8b24296038eae3f64c25d693a269ef1e4d7ae0f691c572a46cf3c0913c",
                    4294967295
        );
    }
    SECTION("EIP-2333 Test case 3"){
        TestEIP2333("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
                    // 44010626067374404458092393860968061149521094673473131545188652121635313364506
                    "614d21b10c0e4996ac0608e0e7452d5720d95d20fe03c59a3321000a42432e1a",
                    // 4011524214304750350566588165922015929937602165683407445189263506512578573606
                    "08de7136e4afc56ae3ec03b20517d9c1232705a747f588fd17832f36ae337526",
                    42
        );
    }
    SECTION("EIP-2333 Test vector with intermediate values"){
        TestEIP2333("c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04",
                    // 5399117110774477986698372024995405256382522670366369834617409486544348441851
                    "0x0befcabff4a664461cc8f190cdd51c05621eb2837c71a1362df5b465a674ecfb",
                    // 11812940737387919040225825939013910852517748782307378293770044673328955938106
                    "1a1de3346883401f1e3b2281be5774080edb8e5ebe6f776b0f7af9fea942553a",
                    0
        );
    }
}

TEST_CASE("Unhardened HD keys") {
    SECTION("Should match derivation through private and public keys"){
        const vector<uint8_t> seed = {1, 50, 6, 244, 24, 199, 1, 25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};

        PrivateKey sk = BasicSchemeMPL().KeyGen(seed);
        G1Element pk = sk.GetG1Element();

        PrivateKey childSk = BasicSchemeMPL().DeriveChildSkUnhardened(sk, 42);
        G1Element childPk = BasicSchemeMPL().DeriveChildPkUnhardened(pk, 42);

        REQUIRE(childSk.GetG1Element() == childPk);

        PrivateKey grandchildSk = BasicSchemeMPL().DeriveChildSkUnhardened(childSk, 12142);
        G1Element grandcihldPk = BasicSchemeMPL().DeriveChildPkUnhardened(childPk, 12142);

        REQUIRE(grandchildSk.GetG1Element() == grandcihldPk);
    }

    SECTION("Should derive public child from parent") {
        const vector<uint8_t> seed = {2, 50, 6, 244, 24, 199, 1, 25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};

        PrivateKey sk = BasicSchemeMPL().KeyGen(seed);
        G1Element pk = sk.GetG1Element();

        PrivateKey childSk = BasicSchemeMPL().DeriveChildSkUnhardened(sk, 42);
        G1Element childPk = BasicSchemeMPL().DeriveChildPkUnhardened(pk, 42);

        PrivateKey childSkHardened = BasicSchemeMPL().DeriveChildSk(sk, 42);
        REQUIRE(childSk.GetG1Element() == childPk);
        REQUIRE(childSkHardened != childSk);
        REQUIRE(childSkHardened.GetG1Element() != childPk);
    }
}

TEST_CASE("IETF test vectors") {
    SECTION ("Pyecc vector") {
        string sig1BasicHex = "96ba34fac33c7f129d602a0bc8a3d43f9abc014eceaab7359146b4b150e57b808645738f35671e9e10e0d862a30cab70074eb5831d13e6a5b162d01eebe687d0164adbd0a864370a7c222a2768d7704da254f1bf1823665bc2361f9dd8c00e99";
        string sk = "0x0101010101010101010101010101010101010101010101010101010101010101";
        vector<uint8_t> msg = {3, 1, 4, 1, 5, 9};
        auto skobj = PrivateKey::FromBytes(Bytes(Util::HexToBytes(sk)));
        G2Element sig = BasicSchemeMPL().Sign(skobj, msg);
        vector<uint8_t> sig1;
        for (const uint8_t byte : Util::HexToBytes(sig1BasicHex)) {
            sig1.push_back(byte);
        }
        REQUIRE(sig == G2Element::FromByteVector(sig1));
    }
}


TEST_CASE("Chia test vectors") {
    SECTION("Chia test vectors 1 (Basic)") {
        vector<uint8_t> seed1(32, 0x00);  // All 0s
        vector<uint8_t> seed2(32, 0x01);  // All 1s
        vector<uint8_t> message1 = {7, 8, 9};
        vector<uint8_t> message2 = {10, 11, 12};

        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed1);
        G1Element pk1 = sk1.GetG1Element();
        G2Element sig1 = BasicSchemeMPL().Sign(sk1, message1);


        PrivateKey sk2 = BasicSchemeMPL().KeyGen(seed2);
        G1Element pk2 = sk2.GetG1Element();
        G2Element sig2 = BasicSchemeMPL().Sign(sk2, message2);

        REQUIRE(pk1.GetFingerprint() == 0xb40dd58a);
        REQUIRE(pk2.GetFingerprint() == 0xb839add1);

        REQUIRE(
            Util::HexStr(sig1.Serialize()) ==
            "b8faa6d6a3881c9fdbad803b170d70ca5cbf1e6ba5a586262df368c75acd1d1f"
            "fa3ab6ee21c71f844494659878f5eb230c958dd576b08b8564aad2ee0992e85a"
            "1e565f299cd53a285de729937f70dc176a1f01432129bb2b94d3d5031f8065a1");
        REQUIRE(
            Util::HexStr(sk1.Serialize()) ==
            "4a353be3dac091a0a7e640620372f5e1e2e4401717c1e79cac6ffba8f6905604");
        REQUIRE(
            Util::HexStr(pk1.Serialize()) ==
            "85695fcbc06cc4c4c9451f4dce21cbf8de3e5a13bf48f44cdbb18e2038ba7b8bb1632d7911e"
            "f1e2e08749bddbf165352");

        REQUIRE(
            Util::HexStr(sig2.Serialize()) ==
            "a9c4d3e689b82c7ec7e838dac2380cb014f9a08f6cd6ba044c263746e39a8f7a60ffee4afb7"
            "8f146c2e421360784d58f0029491e3bd8ab84f0011d258471ba4e87059de295d9aba845c044e"
            "e83f6cf2411efd379ef38bf4cf41d5f3c0ae1205d");

        G2Element aggSig1 = BasicSchemeMPL().Aggregate({sig1, sig2});

        REQUIRE(
            Util::HexStr(aggSig1.Serialize()) ==
            "aee003c8cdaf3531b6b0ca354031b0819f7586b5846796615aee8108fec75ef838d181f9d24"
            "4a94d195d7b0231d4afcf06f27f0cc4d3c72162545c240de7d5034a7ef3a2a03c0159de982fb"
            "c2e7790aeb455e27beae91d64e077c70b5506dea3");

        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message1, message2}, aggSig1));
        REQUIRE(!BasicSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message1, message2}, sig1));
        REQUIRE(!BasicSchemeMPL().Verify(pk1, message1, sig2));
        REQUIRE(!BasicSchemeMPL().Verify(pk1, message2, sig1));

        vector<uint8_t> message3 = {1, 2, 3};
        vector<uint8_t> message4 = {1, 2, 3, 4};
        vector<uint8_t> message5 = {1, 2};

        G2Element sig3 = BasicSchemeMPL().Sign(sk1, message3);
        G2Element sig4 = BasicSchemeMPL().Sign(sk1, message4);
        G2Element sig5 = BasicSchemeMPL().Sign(sk2, message5);

        G2Element aggSig2 = BasicSchemeMPL().Aggregate({sig3, sig4, sig5});

        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk1, pk2}, vector<vector<uint8_t>>{message3, message4, message5}, aggSig2));
        REQUIRE(
            Util::HexStr(aggSig2.Serialize()) ==
            "a0b1378d518bea4d1100adbc7bdbc4ff64f2c219ed6395cd36fe5d2aa44a4b8e710b607afd9"
            "65e505a5ac3283291b75413d09478ab4b5cfbafbeea366de2d0c0bcf61deddaa521f6020460f"
            "d547ab37659ae207968b545727beba0a3c5572b9c");
    }

    SECTION("Chia test vector 2 (Augmented, aggregate of aggregates)") {
        vector<uint8_t> message1 = {1, 2, 3, 40};
        vector<uint8_t> message2 = {5, 6, 70, 201};
        vector<uint8_t> message3 = {9, 10, 11, 12, 13};
        vector<uint8_t> message4 = {15, 63, 244, 92, 0, 1};

        vector<uint8_t> seed1(32, 0x02);  // All 2s
        vector<uint8_t> seed2(32, 0x03);  // All 3s

        PrivateKey sk1 = AugSchemeMPL().KeyGen(seed1);
        PrivateKey sk2 = AugSchemeMPL().KeyGen(seed2);

        G1Element pk1 = sk1.GetG1Element();
        G1Element pk2 = sk2.GetG1Element();

        G2Element sig1 = AugSchemeMPL().Sign(sk1, message1);
        G2Element sig2 = AugSchemeMPL().Sign(sk2, message2);
        G2Element sig3 = AugSchemeMPL().Sign(sk2, message1);
        G2Element sig4 = AugSchemeMPL().Sign(sk1, message3);
        G2Element sig5 = AugSchemeMPL().Sign(sk1, message1);
        G2Element sig6 = AugSchemeMPL().Sign(sk1, message4);


        G2Element aggSigL = AugSchemeMPL().Aggregate({sig1, sig2});
        G2Element aggSigR = AugSchemeMPL().Aggregate({sig3, sig4, sig5});
        G2Element aggSig = AugSchemeMPL().Aggregate({aggSigL, aggSigR, sig6});

        REQUIRE(AugSchemeMPL().AggregateVerify({pk1, pk2, pk2, pk1, pk1, pk1}, vector<vector<uint8_t>>{message1, message2, message1, message3, message1, message4}, aggSig));

        REQUIRE(
            Util::HexStr(aggSig.Serialize()) ==
            "a1d5360dcb418d33b29b90b912b4accde535cf0e52caf467a005dc632d9f7af44b6c4e9acd4"
            "6eac218b28cdb07a3e3bc087df1cd1e3213aa4e11322a3ff3847bbba0b2fd19ddc25ca964871"
            "997b9bceeab37a4c2565876da19382ea32a962200");
    }
    SECTION("Chia test vector 3 (PoP)") {
        vector<uint8_t> message1 = {1, 2, 3, 40, 50};

        vector<uint8_t> seed1(32, 0x04);  // All 4s

        PrivateKey sk1 = PopSchemeMPL().KeyGen(seed1);

        G2Element pop = PopSchemeMPL().PopProve(sk1);
        REQUIRE(PopSchemeMPL().PopVerify(sk1.GetG1Element(), pop));

        REQUIRE(Util::HexStr(pop.Serialize()) == "84f709159435f0dc73b3e8bf6c78d85282d19231555a8ee3b6e2573aaf66872d9203fefa1ef"
                                                 "700e34e7c3f3fb28210100558c6871c53f1ef6055b9f06b0d1abe22ad584ad3b957f3018a8f5"
                                                 "8227c6c716b1e15791459850f2289168fa0cf9115");
    }
}


TEST_CASE("Key generation")
{
    SECTION("Should generate a keypair from a seed")
    {
        vector<uint8_t> seed1(31, 0x08);
        vector<uint8_t> seed2(32, 0x08);

        REQUIRE_THROWS(BasicSchemeMPL().KeyGen(seed1));
        PrivateKey sk = BasicSchemeMPL().KeyGen(seed2);
        G1Element pk = sk.GetG1Element();
        REQUIRE(core_get()->code == RLC_OK);
        REQUIRE(pk.GetFingerprint() == 0x8ee7ba56);
    }
}


TEST_CASE("Error handling")
{
    SECTION("Should throw on a bad private key")
    {
        vector<uint8_t> seed(32, 0x10);
        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        uint8_t* skData = Util::SecAlloc<uint8_t>(G2Element::SIZE);
        sk1.Serialize(skData);
        skData[0] = 255;
        REQUIRE_THROWS(PrivateKey::FromBytes(Bytes(skData, PrivateKey::PRIVATE_KEY_SIZE)));
        Util::SecFree(skData);
    }

    SECTION("Should throw on a bad public key")
    {
        vector<uint8_t> buf(G1Element::SIZE, 0);

        for (int i = 0; i < 10; i++) {
            buf[0] = (uint8_t)i;
            REQUIRE_THROWS(G1Element::FromByteVector(buf));
        }
    }

    SECTION("Should throw on a bad G2Element")
    {
        vector<uint8_t> buf(G2Element::SIZE, 0);

        for (int i = 0; i < 10; i++) {
            buf[0] = (uint8_t)i;
            REQUIRE_THROWS(G2Element::FromByteVector(buf));
        }
    }

    SECTION("Error handling should be thread safe")
    {
        core_get()->code = 10;
        REQUIRE(core_get()->code == 10);

        ctx_t* ctx1 = core_get();

        // spawn a thread and make sure it uses a different/same context depending on relic's multithreading setup
        std::thread([&]() {
#if MULTI != RELIC_NONE
            REQUIRE(ctx1 != core_get());
            REQUIRE(core_get()->code == RLC_OK);
#else
            REQUIRE(ctx1 == core_get());
            REQUIRE(core_get()->code != RLC_OK);
#endif
            core_get()->code = 1;
        }).join();

#if MULTI != RELIC_NONE
        REQUIRE(core_get()->code == 10);
#else
        REQUIRE(core_get()->code == 1);
#endif
        // reset so that future test cases don't fail
        core_get()->code = RLC_OK;
    }
}


TEST_CASE("Util tests")
{
    SECTION("Should convert an int to four bytes")
    {
        uint32_t x = 1024;
        uint8_t expected[4] = {0x00, 0x00, 0x04, 0x00};
        uint8_t result[4];
        Util::IntToFourBytes(result, x);
        REQUIRE(result[0] == expected[0]);
        REQUIRE(result[1] == expected[1]);
        REQUIRE(result[2] == expected[2]);
        REQUIRE(result[3] == expected[3]);
        uint32_t again = Util::FourBytesToInt(result);
        REQUIRE(again == x);
    }
}


TEST_CASE("Signature tests")
{
    SECTION("Should use copy constructor")
    {
        vector<uint8_t> message1 = {1, 65, 254, 88, 90, 45, 22};

        vector<uint8_t> seed(32, 0x30);
        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        G1Element pk1 = sk1.GetG1Element();
        PrivateKey sk2 = PrivateKey(sk1);

        uint8_t skBytes[PrivateKey::PRIVATE_KEY_SIZE];
        sk2.Serialize(skBytes);
        PrivateKey sk4 = PrivateKey::FromBytes(Bytes(skBytes, PrivateKey::PRIVATE_KEY_SIZE));

        G1Element pk2 = G1Element(pk1);
        G2Element sig1 = BasicSchemeMPL().Sign(sk4, message1);
        G2Element sig2 = G2Element(sig1);

        REQUIRE(BasicSchemeMPL().Verify(pk2, message1, sig2));
    }

    SECTION("Should sign with the zero key") {
        vector<uint8_t> sk0(32, 0);
        PrivateKey sk = PrivateKey::FromByteVector(sk0);
        REQUIRE(sk.GetG1Element() == G1Element());  // Infinity
        REQUIRE(sk.GetG2Element() == G2Element());  // Infinity
        REQUIRE(BasicSchemeMPL().Sign(sk, {1, 2, 3}) == G2Element());
        REQUIRE(AugSchemeMPL().Sign(sk, {1, 2, 3}) == G2Element());
        REQUIRE(PopSchemeMPL().Sign(sk, {1, 2, 3}) == G2Element());
    }

    SECTION("Should use equality operators")
    {
        vector<uint8_t> message1 = {1, 65, 254, 88, 90, 45, 22};
        vector<uint8_t> seed(32, 0x40);
        vector<uint8_t> seed3(32, 0x50);

        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        PrivateKey sk2 = PrivateKey(sk1);
        PrivateKey sk3 = BasicSchemeMPL().KeyGen(seed3);
        G1Element pk1 = sk1.GetG1Element();
        G1Element pk2 = sk2.GetG1Element();
        G1Element pk3 = G1Element(pk2);
        G1Element pk4 = sk3.GetG1Element();
        G2Element sig1 = BasicSchemeMPL().Sign(sk1, message1);
        G2Element sig2 = BasicSchemeMPL().Sign(sk1, message1);
        G2Element sig3 = BasicSchemeMPL().Sign(sk2, message1);
        G2Element sig4 = BasicSchemeMPL().Sign(sk3, message1);

        REQUIRE(sk1 == sk2);
        REQUIRE(sk1 != sk3);
        REQUIRE(pk1 == pk2);
        REQUIRE(pk2 == pk3);
        REQUIRE(pk1 != pk4);
        REQUIRE(sig1 == sig2);
        REQUIRE(sig2 == sig3);
        REQUIRE(sig3 != sig4);

        REQUIRE(pk1.Serialize() == pk2.Serialize());
        REQUIRE(sig1.Serialize() == sig2.Serialize());
    }

    SECTION("Should serialize and deserialize")
    {
        vector<uint8_t> message1 = {1, 65, 254, 88, 90, 45, 22};

        vector<uint8_t> seed(32, 0x40);
        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        G1Element pk1 = sk1.GetG1Element();

        uint8_t* skData = Util::SecAlloc<uint8_t>(G2Element::SIZE);
        sk1.Serialize(skData);
        PrivateKey sk2 = PrivateKey::FromBytes(Bytes(skData, PrivateKey::PRIVATE_KEY_SIZE));
        REQUIRE(sk1 == sk2);

        auto pkData = pk1.Serialize();

        G1Element pk2 = G1Element::FromBytes(Bytes(pkData));
        REQUIRE(pk1 == pk2);

        G2Element sig1 = BasicSchemeMPL().Sign(sk1, message1);

        auto sigData = sig1.Serialize();

        G2Element sig2 = G2Element::FromBytes(Bytes(sigData));
        REQUIRE(sig1 == sig2);

        REQUIRE(BasicSchemeMPL().Verify(pk2, message1, sig2));
        Util::SecFree(skData);
    }

    SECTION("Should not verify aggregate with same message under BasicScheme")
    {
        vector<uint8_t> message = {100, 2, 254, 88, 90, 45, 23};
        uint8_t hash[BLS::MESSAGE_HASH_LEN];

        vector<uint8_t> seed(32, 0x50);
        vector<uint8_t> seed2(32, 0x70);

        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        PrivateKey sk2 = BasicSchemeMPL().KeyGen(seed2);

        G1Element pk1 = sk1.GetG1Element();
        G1Element pk2 = sk2.GetG1Element();

        G2Element sig1 = BasicSchemeMPL().Sign(sk1, message);
        G2Element sig2 = BasicSchemeMPL().Sign(sk2, message);

        G2Element aggSig = BasicSchemeMPL().Aggregate({sig1, sig2});
        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message}, aggSig) == false);
    }

    SECTION("Should verify aggregate with same message under AugScheme/PopScheme")
    {
        vector<uint8_t> message = {100, 2, 254, 88, 90, 45, 23};
        uint8_t hash[BLS::MESSAGE_HASH_LEN];

        vector<uint8_t> seed(32, 0x50);
        vector<uint8_t> seed2(32, 0x70);

        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        PrivateKey sk2 = BasicSchemeMPL().KeyGen(seed2);

        G1Element pk1 = sk1.GetG1Element();
        G1Element pk2 = sk2.GetG1Element();

        G2Element sig1Aug = AugSchemeMPL().Sign(sk1, message);
        G2Element sig2Aug = AugSchemeMPL().Sign(sk2, message);
        G2Element aggSigAug = AugSchemeMPL().Aggregate({sig1Aug, sig2Aug});
        REQUIRE(AugSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message}, aggSigAug));

        G2Element sig1Pop = PopSchemeMPL().Sign(sk1, message);
        G2Element sig2Pop = PopSchemeMPL().Sign(sk2, message);
        G2Element aggSigPop = PopSchemeMPL().Aggregate({sig1Pop, sig2Pop});
        REQUIRE(PopSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message}, aggSigPop));
    }

    SECTION("Should Aug aggregate many G2Elements, diff message")
    {
        vector<G1Element> pks;
        vector<G2Element> sigs;
        vector<vector<uint8_t> > ms;

        for (uint8_t i = 0; i < 80; i++) {
            vector<uint8_t> message = {0, 100, 2, 45, 64, 12, 12, 63, i};
            PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
            pks.push_back(sk.GetG1Element());
            auto sig = AugSchemeMPL().Sign(sk, message);
            sigs.push_back(sig);
            ms.push_back(message);
        }

        G2Element aggSig = AugSchemeMPL().Aggregate(sigs);

        REQUIRE(AugSchemeMPL().AggregateVerify(pks, ms, aggSig));
    }

    SECTION("Aggregate Verification of zero items with infinity should pass")
    {
        vector<G1Element> pks_as_g1;
        vector<vector<uint8_t> > pks_as_bytes;
        vector<vector<uint8_t> > msgs;
        vector<G2Element> sigs;

        sigs.push_back(G2Element());
        G2Element aggSig = AugSchemeMPL().Aggregate(sigs);

        REQUIRE(aggSig.Serialize().size() != 0);
        REQUIRE(aggSig == G2Element());

        REQUIRE(AugSchemeMPL().AggregateVerify(pks_as_g1, msgs, aggSig));
        REQUIRE(AugSchemeMPL().AggregateVerify(pks_as_bytes, msgs, aggSig.Serialize()));

        REQUIRE(BasicSchemeMPL().AggregateVerify(pks_as_g1, msgs, aggSig));
        REQUIRE(BasicSchemeMPL().AggregateVerify(pks_as_bytes, msgs, aggSig.Serialize()));

	// FastAggregateVerify takes one message, and requires at least one key
        vector<uint8_t> msg;
        REQUIRE(pks_as_g1.size() == 0);
        REQUIRE(PopSchemeMPL().FastAggregateVerify(pks_as_g1, msg, aggSig) == false);
        REQUIRE(pks_as_bytes.size() == 0);
        REQUIRE(PopSchemeMPL().FastAggregateVerify(pks_as_bytes, msg, aggSig.Serialize()) == false);

    }
}

TEST_CASE("Agg sks") {
    SECTION("Should create aggregates with agg sk (basic scheme)")
    {
        const vector<uint8_t> message = {100, 2, 254, 88, 90, 45, 23};
        const vector<uint8_t> seed(32, 0x07);
        const vector<uint8_t> seed2(32, 0x08);

        const PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed);
        const G1Element pk1 = sk1.GetG1Element();

        const PrivateKey sk2 = BasicSchemeMPL().KeyGen(seed2);
        const G1Element pk2 = sk2.GetG1Element();

        const PrivateKey aggSk = PrivateKey::Aggregate({sk1, sk2});
        const PrivateKey aggSkAlt = PrivateKey::Aggregate({sk2, sk1});
        REQUIRE(aggSk == aggSkAlt);

        const G1Element aggPubKey = pk1 + pk2;
        REQUIRE(aggPubKey == aggSk.GetG1Element());

        const G2Element sig1 = BasicSchemeMPL().Sign(sk1, message);
        const G2Element sig2 = BasicSchemeMPL().Sign(sk2, message);

        const G2Element aggSig2 = BasicSchemeMPL().Sign(aggSk, message);


        const G2Element aggSig = BasicSchemeMPL().Aggregate({sig1, sig2});
        REQUIRE(aggSig == aggSig2);

        // Verify as a single G2Element
        REQUIRE(BasicSchemeMPL().Verify(aggPubKey, message, aggSig));
        REQUIRE(BasicSchemeMPL().Verify(aggPubKey, message, aggSig2));

        // Verify aggregate with both keys (Fails since not distinct)
        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message}, aggSig) == false);
        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message}, aggSig2) == false);

        // Try the same with distinct message, and same sk
        vector<uint8_t> message2 = {200, 29, 54, 8, 9, 29, 155, 55};
        G2Element sig3 = BasicSchemeMPL().Sign(sk2, message2);
        G2Element aggSigFinal = BasicSchemeMPL().Aggregate({aggSig, sig3});
        G2Element aggSigAlt = BasicSchemeMPL().Aggregate({sig1, sig2, sig3});
        G2Element aggSigAlt2 = BasicSchemeMPL().Aggregate({sig1, sig3, sig2});
        REQUIRE(aggSigFinal == aggSigAlt);
        REQUIRE(aggSigFinal == aggSigAlt2);

        PrivateKey skFinal = PrivateKey::Aggregate({aggSk, sk2});
        PrivateKey skFinalAlt = PrivateKey::Aggregate({sk2, sk1, sk2});
        REQUIRE(skFinal == skFinalAlt);
        REQUIRE(skFinal != aggSk);

        G1Element pkFinal = aggPubKey + pk2;
        G1Element pkFinalAlt = pk2 + pk1 + pk2;
        REQUIRE(pkFinal == pkFinalAlt);
        REQUIRE(pkFinal != aggPubKey);

        // Cannot verify with aggPubKey (since we have multiple messages)
        REQUIRE(BasicSchemeMPL().AggregateVerify({aggPubKey, pk2}, vector<vector<uint8_t>>{message, message2}, aggSigFinal));
    }
}

TEST_CASE("Advanced") {
    SECTION("Should aggregate with multiple levels, degenerate") {
        vector<uint8_t> message1 = {100, 2, 254, 88, 90, 45, 23};
        PrivateKey sk1 = AugSchemeMPL().KeyGen(getRandomSeed());
        G1Element pk1 = sk1.GetG1Element();
        G2Element aggSig = AugSchemeMPL().Sign(sk1, message1);
        vector<G1Element> pks = {pk1};
        vector<vector<uint8_t>> ms = {message1};

        for (size_t i = 0; i < 10; i++) {
            PrivateKey sk = AugSchemeMPL().KeyGen(getRandomSeed());
            G1Element pk = sk.GetG1Element();
            pks.push_back(pk);
            ms.push_back(message1);
            G2Element sig = AugSchemeMPL().Sign(sk, message1);
            aggSig = AugSchemeMPL().Aggregate({aggSig, sig});
        }
        REQUIRE(AugSchemeMPL().AggregateVerify(pks, ms, aggSig));
    }

    SECTION("Should aggregate with multiple levels, different messages") {
        vector<uint8_t> message1 = {100, 2, 254, 88, 90, 45, 23};
        vector<uint8_t> message2 = {192, 29, 2, 0, 0, 45, 23};
        vector<uint8_t> message3 = {52, 29, 2, 0, 0, 45, 102};
        vector<uint8_t> message4 = {99, 29, 2, 0, 0, 45, 222};

        PrivateKey sk1 = AugSchemeMPL().KeyGen(getRandomSeed());
        PrivateKey sk2 = AugSchemeMPL().KeyGen(getRandomSeed());

        G1Element pk1 = sk1.GetG1Element();
        G1Element pk2 = sk2.GetG1Element();

        G2Element sig1 = AugSchemeMPL().Sign(sk1, message1);
        G2Element sig2 = AugSchemeMPL().Sign(sk2, message2);
        G2Element sig3 = AugSchemeMPL().Sign(sk2, message3);
        G2Element sig4 = AugSchemeMPL().Sign(sk1, message4);

        vector<G2Element> const sigsL = {sig1, sig2};
        vector<G1Element> const pksL = {pk1, pk2};
        vector<vector<uint8_t>> messagesL = {message1, message2};
        const G2Element aggSigL = AugSchemeMPL().Aggregate(sigsL);

        vector<G2Element> const sigsR = {sig3, sig4};
        vector<G1Element> const pksR = {pk2, pk1};
        const G2Element aggSigR = AugSchemeMPL().Aggregate(sigsR);

        vector<G2Element> sigs = {aggSigL, aggSigR};
        const G2Element aggSig = AugSchemeMPL().Aggregate(sigs);

        vector<G1Element> allPks = {pk1, pk2, pk2, pk1};
        vector<vector<uint8_t>> allMessages = {
            message1, message2, message3, message4};
        REQUIRE(AugSchemeMPL().AggregateVerify(allPks, allMessages, aggSig));
    }

    SECTION("README")
    {
        // Example seed, used to generate private key. Always use
        // a secure RNG with sufficient entropy to generate a seed (at least 32 bytes).
        vector<uint8_t> seed = {0,  50, 6,  244, 24,  199, 1,  25,  52,  88,  192,
                                19, 18, 12, 89,  6,   220, 18, 102, 58,  209, 82,
                                12, 62, 89, 110, 182, 9,   44, 20,  254, 22};

        PrivateKey sk = AugSchemeMPL().KeyGen(seed);
        G1Element pk = sk.GetG1Element();

        vector<uint8_t> message = {1, 2, 3, 4, 5};  // Message is passed in as a byte vector
        G2Element signature = AugSchemeMPL().Sign(sk, message);

        vector<uint8_t> skBytes = sk.Serialize();
        vector<uint8_t> pkBytes = pk.Serialize();
        vector<uint8_t> signatureBytes = signature.Serialize();

        cout << Util::HexStr(skBytes) << endl;    // 32 bytes
        cout << Util::HexStr(pkBytes) << endl;    // 48 bytes
        cout << Util::HexStr(signatureBytes) << endl;  // 96 bytes

        // Takes array of 32 bytes
        PrivateKey skc = PrivateKey::FromByteVector(skBytes);

        // Takes array of 48 bytes
        pk = G1Element::FromByteVector(pkBytes);

        // Takes array of 96 bytes
        signature = G2Element::FromByteVector(signatureBytes);

        REQUIRE(AugSchemeMPL().Verify(pk, message, signature));

        // Generate some more private keys
        seed[0] = 1;
        PrivateKey sk1 = AugSchemeMPL().KeyGen(seed);
        seed[0] = 2;
        PrivateKey sk2 = AugSchemeMPL().KeyGen(seed);
        vector<uint8_t> message2 = {1, 2, 3, 4, 5, 6, 7};

        // Generate first sig
        G1Element pk1 = sk1.GetG1Element();
        G2Element sig1 = AugSchemeMPL().Sign(sk1, message);

        // Generate second sig
        G1Element pk2 = sk2.GetG1Element();
        G2Element sig2 = AugSchemeMPL().Sign(sk2, message2);

        // Signatures can be noninteractively combined by anyone
        G2Element aggSig = AugSchemeMPL().Aggregate({sig1, sig2});

        REQUIRE(AugSchemeMPL().AggregateVerify({pk1, pk2}, vector<vector<uint8_t>>{message, message2}, aggSig));

        seed[0] = 3;
        PrivateKey sk3 = AugSchemeMPL().KeyGen(seed);
        G1Element pk3 = sk3.GetG1Element();
        vector<uint8_t> message3 = {100, 2, 254, 88, 90, 45, 23};
        G2Element sig3 = AugSchemeMPL().Sign(sk3, message3);


        // Arbitrary trees of aggregates
        G2Element aggSigFinal = AugSchemeMPL().Aggregate({aggSig, sig3});

        REQUIRE(AugSchemeMPL().AggregateVerify({pk1, pk2, pk3}, vector<vector<uint8_t>>{message, message2, message3}, aggSigFinal));

        // If the same message is signed, you can use Proof of Posession (PopScheme) for efficiency
        // A proof of possession MUST be passed around with the PK to ensure security.

        G2Element popSig1 = PopSchemeMPL().Sign(sk1, message);
        G2Element popSig2 = PopSchemeMPL().Sign(sk2, message);
        G2Element popSig3 = PopSchemeMPL().Sign(sk3, message);
        G2Element pop1 = PopSchemeMPL().PopProve(sk1);
        G2Element pop2 = PopSchemeMPL().PopProve(sk2);
        G2Element pop3 = PopSchemeMPL().PopProve(sk3);

        REQUIRE(PopSchemeMPL().PopVerify(pk1, pop1));
        REQUIRE(PopSchemeMPL().PopVerify(pk2, pop2));
        REQUIRE(PopSchemeMPL().PopVerify(pk3, pop3));
        G2Element popSigAgg = PopSchemeMPL().Aggregate({popSig1, popSig2, popSig3});

        REQUIRE(PopSchemeMPL().FastAggregateVerify({pk1, pk2, pk3}, message, popSigAgg));

        // Aggregate public key, indistinguishable from a single public key
        G1Element popAggPk = pk1 + pk2 + pk3;
        REQUIRE(PopSchemeMPL().Verify(popAggPk, message, popSigAgg));

        // Aggregate private keys
        PrivateKey aggSk = PrivateKey::Aggregate({sk1, sk2, sk3});
        REQUIRE(PopSchemeMPL().Sign(aggSk, message) == popSigAgg);


        PrivateKey masterSk = AugSchemeMPL().KeyGen(seed);
        PrivateKey child = AugSchemeMPL().DeriveChildSk(masterSk, 152);
        PrivateKey grandchild = AugSchemeMPL().DeriveChildSk(child, 952);

        G1Element masterPk = masterSk.GetG1Element();
        PrivateKey childU = AugSchemeMPL().DeriveChildSkUnhardened(masterSk, 22);
        PrivateKey grandchildU = AugSchemeMPL().DeriveChildSkUnhardened(childU, 0);

        G1Element childUPk = AugSchemeMPL().DeriveChildPkUnhardened(masterPk, 22);
        G1Element grandchildUPk = AugSchemeMPL().DeriveChildPkUnhardened(childUPk, 0);

        REQUIRE(grandchildUPk == grandchildU.GetG1Element());
    }

    SECTION("Bytes overloads")
    {
        std::vector<uint8_t> vecHash = getRandomSeed();
        PrivateKey pk1 = BasicSchemeMPL().KeyGen(Bytes(getRandomSeed()));
        PrivateKey pk2 = AugSchemeMPL().KeyGen(Bytes(getRandomSeed()));
        PrivateKey pk3 = PopSchemeMPL().KeyGen(Bytes(getRandomSeed()));

        std::vector<uint8_t> vecG1Element = pk1.GetG1Element().Serialize();
        G1Element g1Vector = G1Element::FromByteVector(vecG1Element);
        G1Element g1Bytes = G1Element::FromBytes(Bytes(vecG1Element));
        REQUIRE(g1Vector == g1Bytes);

        std::vector<uint8_t> vecG2Element = pk1.GetG2Element().Serialize();
        G2Element g2Vector = G2Element::FromByteVector(vecG2Element);
        G2Element g2Bytes = G2Element::FromBytes(Bytes(vecG2Element));
        REQUIRE(g2Vector == g2Bytes);

        G1Element g1MessageVector = G1Element::FromMessage(vecHash, vecHash.data(), vecHash.size());
        G1Element g1MessageBytes = G1Element::FromMessage(Bytes(vecHash), vecHash.data(), vecHash.size());
        REQUIRE(g1MessageVector == g1MessageBytes);

        G2Element g2MessageVector = G2Element::FromMessage(vecHash, vecHash.data(), vecHash.size());
        G2Element g2MessageBytes = G2Element::FromMessage(Bytes(vecHash), vecHash.data(), vecHash.size());
        REQUIRE(g2MessageVector == g2MessageBytes);

        G1Element g1_1 = pk1.GetG1Element();
        G1Element g1_2 = pk2.GetG1Element();
        G1Element g1_3 = pk3.GetG1Element();

        G2Element g2BasicSignVector1 = BasicSchemeMPL().Sign(pk1, vecHash);
        G2Element g2BasicSignBytes1 = BasicSchemeMPL().Sign(pk1, Bytes(vecHash));
        REQUIRE(g2BasicSignVector1 == g2BasicSignBytes1);
        G2Element g2BasicSign2 = BasicSchemeMPL().Sign(pk2, Bytes(vecHash));
        G2Element g2BasicSign3 = BasicSchemeMPL().Sign(pk3, Bytes(vecG2Element));
        REQUIRE(g2BasicSignVector1 != g2BasicSign2);

        REQUIRE(BasicSchemeMPL().Verify(Bytes(g1_1.Serialize()), Bytes(vecHash), Bytes(g2BasicSignVector1.Serialize())));
        REQUIRE(BasicSchemeMPL().Verify(g1_1, Bytes(vecHash), g2BasicSignVector1));

        vector<vector<uint8_t>> vecG1Vector = {g1_1.Serialize(), g1_3.Serialize()};
        vector<vector<uint8_t>> vecG2Vector = {g2BasicSignVector1.Serialize(), g2BasicSign3.Serialize()};
        vector<vector<uint8_t>> vecHashes = {vecHash, vecG2Element};

        vector<uint8_t> aggVector = BasicSchemeMPL().Aggregate(vecG2Vector);
        vector<uint8_t> aggBytes = BasicSchemeMPL().Aggregate(vector<Bytes>{vecG2Vector.begin(), vecG2Vector.end()});
        REQUIRE(aggVector == aggBytes);

        REQUIRE(BasicSchemeMPL().AggregateVerify(vector<Bytes>{vecG1Vector.begin(), vecG1Vector.end()},
                                                 vector<Bytes>{vecHashes.begin(), vecHashes.end()},
                                                 Bytes(aggVector)));
        REQUIRE(BasicSchemeMPL().AggregateVerify({g1_1, g1_3},
                                                 vector<Bytes>{vecHashes.begin(), vecHashes.end()},
                                                 G2Element::FromByteVector(aggVector)));

        G2Element g2AugSignVector1 = AugSchemeMPL().Sign(pk1, vecHash);
        G2Element g2AugSignBytes1 = AugSchemeMPL().Sign(pk1, Bytes(vecHash));
        G2Element g2AugSign2 = AugSchemeMPL().Sign(pk2, vecG2Element);
        REQUIRE(g2AugSignVector1 == g2AugSignBytes1);
        REQUIRE(g2AugSignVector1 != g2AugSign2);

        REQUIRE(AugSchemeMPL().Verify(Bytes(g1_1.Serialize()), Bytes(vecHash), Bytes(g2AugSignVector1.Serialize())));
        REQUIRE(AugSchemeMPL().Verify(g1_1, Bytes(vecHash), g2AugSignVector1));

        vector<vector<uint8_t>> vecG1AugVector = {g1_1.Serialize(), g1_2.Serialize()};
        vector<vector<uint8_t>> vecG2AugVector = {g2AugSignVector1.Serialize(), g2AugSign2.Serialize()};

        vector<uint8_t> aggAugVector = AugSchemeMPL().Aggregate(vecG2AugVector);
        vector<uint8_t> aggAugBytes = AugSchemeMPL().Aggregate(vector<Bytes>{vecG2AugVector.begin(), vecG2AugVector.end()});
        REQUIRE(aggAugVector == aggAugBytes);

        REQUIRE(AugSchemeMPL().AggregateVerify(vector<Bytes>{vecG1AugVector.begin(), vecG1AugVector.end()},
                                                 vector<Bytes>{vecHashes.begin(), vecHashes.end()},
                                                 Bytes(aggAugVector)));
        REQUIRE(AugSchemeMPL().AggregateVerify({g1_1, g1_2},
                                                 vector<Bytes>{vecHashes.begin(), vecHashes.end()},
                                                 G2Element::FromByteVector(aggAugVector)));

        G2Element proof = PopSchemeMPL().PopProve(pk1);
        REQUIRE(PopSchemeMPL().PopVerify(g1_1, proof));
        REQUIRE(PopSchemeMPL().PopVerify(Bytes(g1_1.Serialize()), Bytes(proof.Serialize())));

        G2Element g2Pop1 = PopSchemeMPL().Sign(pk1, vecHash);
        G2Element g2Pop2 = PopSchemeMPL().Sign(pk2, vecHash);
        G2Element g2PopAgg = PopSchemeMPL().Aggregate({g2Pop1, g2Pop2});
        vecG1Vector = {g1_1.Serialize(), g1_2.Serialize()};
        REQUIRE(PopSchemeMPL().FastAggregateVerify({g1_1, g1_2}, Bytes(vecHash), g2PopAgg));
        REQUIRE(PopSchemeMPL().FastAggregateVerify(vector<Bytes>{vecG1Vector.begin(), vecG1Vector.end()},
                                                   Bytes(vecHash), Bytes(g2PopAgg.Serialize())));
    }
}


TEST_CASE("Schemes") {
    SECTION("Basic Scheme") {
        vector<uint8_t> seed1(32, 0x04);
        vector<uint8_t> seed2(32, 0x05);
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = BasicSchemeMPL().KeyGen(seed1);
        G1Element pk1 = BasicSchemeMPL().SkToG1(sk1);
        vector<uint8_t> pk1v = BasicSchemeMPL().SkToPk(sk1);
        G2Element sig1 = BasicSchemeMPL().Sign(sk1, msg1);
        vector<uint8_t> sig1v = BasicSchemeMPL().Sign(sk1, msg1).Serialize();


        REQUIRE(BasicSchemeMPL().Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = BasicSchemeMPL().KeyGen(seed2);
        G1Element pk2 = BasicSchemeMPL().SkToG1(sk2);
        vector<uint8_t> pk2v = BasicSchemeMPL().SkToPk(sk2);
        G2Element sig2 = BasicSchemeMPL().Sign(sk2, msg2);
        vector<uint8_t> sig2v = BasicSchemeMPL().Sign(sk2, msg2).Serialize();

        // Wrong G2Element
        REQUIRE(BasicSchemeMPL().Verify(pk1, msg1, sig2) == false);
        REQUIRE(BasicSchemeMPL().Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(BasicSchemeMPL().Verify(pk1, msg2, sig1) == false);
        REQUIRE(BasicSchemeMPL().Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(BasicSchemeMPL().Verify(pk2, msg1, sig1) == false);
        REQUIRE(BasicSchemeMPL().Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = BasicSchemeMPL().Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = BasicSchemeMPL().Aggregate(vector<vector<uint8_t>>{sig1v, sig2v});
        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(BasicSchemeMPL().AggregateVerify({pk1v, pk2v}, msgs, aggsigv));
    }

    SECTION("Aug Scheme")
    {
        vector<uint8_t> seed1(32, 0x04);
        vector<uint8_t> seed2(32, 0x05);
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = AugSchemeMPL().KeyGen(seed1);
        G1Element pk1 = AugSchemeMPL().SkToG1(sk1);
        vector<uint8_t> pk1v = AugSchemeMPL().SkToPk(sk1);
        G2Element sig1 = AugSchemeMPL().Sign(sk1, msg1);
        vector<uint8_t> sig1v = AugSchemeMPL().Sign(sk1, msg1).Serialize();

        REQUIRE(AugSchemeMPL().Verify(pk1, msg1, sig1));
        REQUIRE(AugSchemeMPL().Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = AugSchemeMPL().KeyGen(seed2);
        G1Element pk2 = AugSchemeMPL().SkToG1(sk2);
        vector<uint8_t> pk2v = AugSchemeMPL().SkToPk(sk2);
        G2Element sig2 = AugSchemeMPL().Sign(sk2, msg2);
        vector<uint8_t> sig2v = AugSchemeMPL().Sign(sk2, msg2).Serialize();

        // Wrong G2Element
        REQUIRE(AugSchemeMPL().Verify(pk1, msg1, sig2) == false);
        REQUIRE(AugSchemeMPL().Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(AugSchemeMPL().Verify(pk1, msg2, sig1) == false);
        REQUIRE(AugSchemeMPL().Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(AugSchemeMPL().Verify(pk2, msg1, sig1) == false);
        REQUIRE(AugSchemeMPL().Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = AugSchemeMPL().Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = AugSchemeMPL().Aggregate(vector<vector<uint8_t>>{sig1v, sig2v});
        REQUIRE(AugSchemeMPL().AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(AugSchemeMPL().AggregateVerify({pk1v, pk2v}, msgs, aggsigv));
    }

    SECTION("Pop Scheme")
    {
        vector<uint8_t> seed1(32, 0x06);
        vector<uint8_t> seed2(32, 0x07);
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = PopSchemeMPL().KeyGen(seed1);
        G1Element pk1 = PopSchemeMPL().SkToG1(sk1);
        vector<uint8_t> pk1v = PopSchemeMPL().SkToPk(sk1);
        G2Element sig1 = PopSchemeMPL().Sign(sk1, msg1);
        vector<uint8_t> sig1v = PopSchemeMPL().Sign(sk1, msg1).Serialize();

        REQUIRE(PopSchemeMPL().Verify(pk1, msg1, sig1));
        REQUIRE(PopSchemeMPL().Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = PopSchemeMPL().KeyGen(seed2);
        G1Element pk2 = PopSchemeMPL().SkToG1(sk2);
        vector<uint8_t> pk2v = PopSchemeMPL().SkToPk(sk2);
        G2Element sig2 = PopSchemeMPL().Sign(sk2, msg2);
        vector<uint8_t> sig2v = PopSchemeMPL().Sign(sk2, msg2).Serialize();

        // Wrong G2Element
        REQUIRE(PopSchemeMPL().Verify(pk1, msg1, sig2) == false);
        REQUIRE(PopSchemeMPL().Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(PopSchemeMPL().Verify(pk1, msg2, sig1) == false);
        REQUIRE(PopSchemeMPL().Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(PopSchemeMPL().Verify(pk2, msg1, sig1) == false);
        REQUIRE(PopSchemeMPL().Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = PopSchemeMPL().Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = PopSchemeMPL().Aggregate(vector<vector<uint8_t>>{sig1v, sig2v});
        REQUIRE(PopSchemeMPL().AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(PopSchemeMPL().AggregateVerify({pk1v, pk2v}, msgs, aggsigv));

        // PopVerify
        G2Element proof1 = PopSchemeMPL().PopProve(sk1);
        vector<uint8_t> proof1v = PopSchemeMPL().PopProve(sk1).Serialize();
        REQUIRE(PopSchemeMPL().PopVerify(pk1, proof1));
        REQUIRE(PopSchemeMPL().PopVerify(pk1v, proof1v));

        // FastAggregateVerify
        // We want sk2 to sign the same message
        G2Element sig2_same = PopSchemeMPL().Sign(sk2, msg1);
        vector<uint8_t> sig2v_same = PopSchemeMPL().Sign(sk2, msg1).Serialize();
        G2Element aggsig_same = PopSchemeMPL().Aggregate({sig1, sig2_same});
        vector<uint8_t> aggsigv_same =
            PopSchemeMPL().Aggregate(vector<vector<uint8_t>>{sig1v, sig2v_same});
        REQUIRE(
            PopSchemeMPL().FastAggregateVerify({pk1, pk2}, msg1, aggsig_same));
        REQUIRE(PopSchemeMPL().FastAggregateVerify(
            {pk1v, pk2v}, msg1, aggsigv_same));
    }

    SECTION("Legacy scheme") {
        // Test legacy example data defined in https://gist.github.com/xdustinface/318c2c08c36ab12a2b1963caf1f7815c
        std::string strSignHash{"b6d8ee31bbd375dfd55d5fb4b02cfccc68709e64f4c5ffcd3895ceb46540311d"};
        std::string strThresholdPublicKey{"97a12b918eac73718d55b7fca60435ec0b442d7e24b45cbd80f5cf2ea2e14c4164333fffdb00d27e309abbafacaa9c34"};
        std::string strThresholdSignature{"031c536960c9c44efefa4a3334d2d1b808f46abe121cd14a1d0b6ee6b6dca85fd3087d86fe5b1327e6792be53f4ed4df19c3af3aac79c0bb9dc36fc2a30ba566de6a82cd3e4af2d6654cbe02bb52769a06c1644a4c63b3c3a6fd16e5e68e5c0b"};
        std::vector<std::string> vecSecretKeySharesHex{
            "43dd263542a8d10bc9f843b46f15c86cd87e00c8f45fe5339b30c08e3233d8e3",
            "5e7247ef1a0e671b8349e7be3f50ec88f1eafde90345f34520010e4aa9f18731",
            "34bcc40dea17bb03ec085ac46a0ea9614b3ffc4bae8b0b292f3d7c54662b00a6",
            "139f967b6f4af5dfe2bebf8085b6332efe31c2dc348d02e6b4745a0e7e56a469",
            "08442e959054d87b5de3553ef8cfd9da923241664c35c6548b5e3271a86b4a18",
            "2698613947a156639b423ad1a9fbe45863d58540d8ebd08612504bf9cf4743ea",
            "1871c9270c8344028946eee64e79d09e4915dd3b717ffc1c9aa86faff88c0475",
            "68409938427df3567e8948c1fff8610b5cf94872eb959c90a714b7ff0f339e88",
            "70d3e3ad7d22bd30e4e2ca108a3fa47f4873bda28f3b000a218339b09db6f58a",
            "39ea630e894b71d9f28fefb551611824f16d4b16d29fdea8bb3dbd857a6bc317",
        };
        std::vector<std::string> vecSigSharesHex{
                "0888879c99852460912fd28c7a9138926c1e87fd6609fd2d3d307764e49feb85702fd8f9b3b836bc11f7ce151b769dc70b760879d26f8c33a29e24f69297f45ef028f0794e63ddb0610db7de1a608b6d6a2129ada62b845004a408f651fd44a5",
                "16efc39fa5aa979245a82334856a97ebf3027bc6be7d35df117267a3c9b1618e32477fe1b8f4a23bdba346bf2b2b35ad0b395227de76827fd32eb9952e0d97b7dba275040101131a7fc38ea381a3099c2b3205177866ee4ab3119593bb58d092",
                "8407afd2776ab9d3f9293f1519ace1a9ce8aaf07d0a6a9785ec7ae4ae47e42102c09cfb3c8655dba014d53933af6a0b41244df006575e85e333271c90fcad80410cab4962bf4bba1570770775b1282f142b526d521a38fbc14336f22dc44a683",
                "027061a8c2d631e40882ce6919d3e5f45c4c74ac32a3bce94e5661d06cdecf2d492dfab99e9a9dd8a29a90fe8f816be30178bf9277a3751882e49bb9f08437f5f2cd9dbc12c2fdcccaf7578838e87273fd2ba87f20cf00ca5fec56822f845769",
                "178ece91967145b1dfc02de703dbd8049b05d626f18a71303ea0c23ee3b60a52bd61cc30fea3e4a562b20c20e0439a2f108b4e6b8a646f647afb64e3b355eba382380ef2c634f3a56de066b7a764249aba1c42c49d76d65e094e890cbbf005a3",
                "193da45d31d728acc92165173253fd8689301b448c81039350ae6916a72f157b00da469a7ab6d2b5db2dac216f47073d089afdcdffce25b6aac991f4745c803f164d7426b8da7d19bf699f5e5489f715ac32e539db90610d7df47121556c1a20",
                "938ce6cc265fa15fe67314ac4773f18ff0b49c01eac626814abc2f836814068aeba8582d619a3e0c08dc4bafecda84a818b6a7abd350637e72a47356e5919e3a72be66316417c598338e4ab85f8d25535bd6c4a5fb16767ac470902e0cf19df0",
                "985039d55a92f6fb3b324b0b9f1c7ddcd5f443d6d1ce72549f043b967ded7d56dc4320dc8569a1c41c6cfb4c150d8c61095d3b325e3308a321ceb43369fe56807fba67b6b313f072ab2cdbaa872b52a9a2e75bf396f1b2007152067f756946b7",
                "922717d8c170862662d08a4c29943cc26bb05daff0f07b0b7c352651ec64ee5a1d032bad24dfb42243e9afe044ed25680694b183b657948a91533e9a73b6bf359ff907d5088503137edc8e271ac3d2003a4daf8d36f3f847cc87afc6f9007c72",
                "02bc8e3ea8409418949fb4106e00893b49983495035b47026a1747eb6f89b05d4c1b8e357e89ddfa7c9f8145b78e0c480177842f20b98b1f7ca2ed26cfb9895380e4d86aeb60c2326bc43753a0633167a7c4ae7a526ce927ade1388a6cdc11d1",
        };
        std::vector<std::string> vecIdsHex{
                "4393e2a243c3db776dcdbe2535493440d29cb65006a1e6f0f8d3f1e1488cbf1a",
                "8b2d776ac75cfca1559b5616ade4eb376a6478556135276e4b3210fe170b9f59",
                "f2015bdbc0daa70c41a25d2450b96f433ac7d568126505e9997794bb357cf3af",
                "5818a68f2f34e5ff7d1d43beca5ff103739dd918efda4bac7fd4ede6c53dc6af",
                "965437bdf51278f716078477a2eae595a9d2b0aa3fe69a387b30936c13c7d68e",
                "fd14695a48a35fe6a1f9894accfa83349508e350b3f743d494074fe204b17166",
                "7e3c28e59ff54bf097b2f3ada4a30f5613227951116675127fc97c98405f2067",
                "5e427dba092e3d81d057c0277a9a465e036c8336c59a18f27d7c21bc51910904",
                "90976dbe492de3eda7623c7ad6523ed9f63f83c3200c383fccd9f22408e18e1b",
                "cd474447afd5df18a0c10c9e2d5216eace9c624119974280236a043cb4b7f8ae",
        };
        size_t nSize = 10;
        REQUIRE(vecSecretKeySharesHex.size() == nSize);
        REQUIRE(vecSigSharesHex.size() == nSize);
        REQUIRE(vecIdsHex.size() == nSize);

        std::vector<uint8_t> vecSignHash = Util::HexToBytes(strSignHash);
        std::reverse(vecSignHash.begin(), vecSignHash.end());
        G1Element thresholdPublicKey = G1Element::FromByteVector(Util::HexToBytes(strThresholdPublicKey), true);
        G2Element thresholdSignatureExpected = G2Element::FromByteVector(Util::HexToBytes(strThresholdSignature), true);

        std::vector<G2Element> vecSignatureShares;
        std::vector<std::vector<uint8_t>> vecIds;

        for (size_t i = 0; i < nSize; ++i) {
            vecIds.push_back(Util::HexToBytes(vecIdsHex[i]));
            std::reverse(vecIds.back().begin(), vecIds.back().end());

            PrivateKey skShare = PrivateKey::FromByteVector(Util::HexToBytes(vecSecretKeySharesHex[i]));
            std::vector<uint8_t> vecSigShareBytes = Util::HexToBytes(vecSigSharesHex[i]);
            vecSignatureShares.push_back(G2Element::FromByteVector(vecSigShareBytes, true));
            G2Element sigShare = LegacySchemeMPL().Sign(skShare, Bytes(vecSignHash));
            REQUIRE(sigShare == vecSignatureShares.back());
            REQUIRE(sigShare.Serialize(true) == vecSigShareBytes);
        }

        G2Element thresholdSignature = Threshold::SignatureRecover(vecSignatureShares, {vecIds.begin(), vecIds.end()});
        REQUIRE(thresholdSignature == thresholdSignatureExpected);
        REQUIRE(LegacySchemeMPL().Verify(thresholdPublicKey, Bytes{vecSignHash}, thresholdSignature));
    }
}

TEST_CASE("Legacy HD keys") {
    SECTION("Should create an extended private key from seed") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));

        ExtendedPrivateKey esk77 = esk.PrivateChild(77 + (1 << 31));
        ExtendedPrivateKey esk77copy = esk.PrivateChild(77 + (1 << 31));

        REQUIRE(esk77 == esk77copy);

        ExtendedPrivateKey esk77nh = esk.PrivateChild(77);

        auto eskLong = esk.PrivateChild((1 << 31) + 5)
                .PrivateChild(0)
                .PrivateChild(0)
                .PrivateChild((1 << 31) + 56)
                .PrivateChild(70)
                .PrivateChild(4);
        uint8_t chainCode[32];
        eskLong.GetChainCode().Serialize(chainCode);
    }


    SECTION("Should match derivation through private and public keys") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        G1Element pk1 = esk.PrivateChild(238757).GetPublicKey();
        G1Element pk2 = epk.PublicChild(238757).GetPublicKey();

        REQUIRE(pk1 == pk2);

        PrivateKey sk3 = esk.PrivateChild(0)
                .PrivateChild(3)
                .PrivateChild(8)
                .PrivateChild(1)
                .GetPrivateKey();

        G1Element pk4 = epk.PublicChild(0)
                .PublicChild(3)
                .PublicChild(8)
                .PublicChild(1)
                .GetPublicKey();
        REQUIRE(sk3.GetG1Element() == pk4);

        G2Element sig = LegacySchemeMPL().Sign(sk3, Bytes(seed));

        REQUIRE(LegacySchemeMPL().Verify(sk3.GetG1Element(), Bytes(seed), sig));
    }

    SECTION("Should prevent hardened pk derivation") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        ExtendedPrivateKey sk = esk.PrivateChild((1 << 31) + 3);
        REQUIRE_THROWS(epk.PublicChild((1 << 31) + 3));
    }

    SECTION("Should derive public child from parent") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 24, 199, 1, 0, 0, 0};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        ExtendedPublicKey pk1 = esk.PublicChild(13);
        ExtendedPublicKey pk2 = epk.PublicChild(13);

        REQUIRE(pk1 == pk2);
    }

    SECTION("Should cout structures") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 24, 199, 1, 0, 0, 0};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        cout << epk << endl;
        cout << epk.GetPublicKey() << endl;
        cout << epk.GetChainCode() << endl;

        G2Element sig1 = LegacySchemeMPL().Sign(esk.GetPrivateKey(), Bytes(seed));
        cout << sig1 << endl;
    }

    SECTION("Should serialize extended keys") {
        std::vector<uint8_t> seed{1, 50, 6, 244, 25, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(Bytes(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        G1Element pk1 = esk.PrivateChild(238757).GetPublicKey();
        G1Element pk2 = epk.PublicChild(238757).GetPublicKey();

        REQUIRE(pk1 == pk2);

        ExtendedPrivateKey sk3 = esk.PrivateChild(0)
                .PrivateChild(3)
                .PrivateChild(8)
                .PrivateChild(1);

        ExtendedPublicKey pk4 = epk.PublicChild(0)
                .PublicChild(3)
                .PublicChild(8)
                .PublicChild(1);
        uint8_t buffer1[ExtendedPrivateKey::SIZE];
        uint8_t buffer2[ExtendedPublicKey::SIZE];
        uint8_t buffer3[ExtendedPublicKey::SIZE];

        sk3.Serialize(buffer1);
        sk3.GetExtendedPublicKey().Serialize(buffer2);
        pk4.Serialize(buffer3);
        REQUIRE(std::memcmp(buffer2, buffer3, ExtendedPublicKey::SIZE) == 0);
    }
}

TEST_CASE("Threshold Signatures") {
    SECTION("Secret Key Shares") {
        size_t m = 3;
        size_t n = 5;

        std::vector<PrivateKey> sks;
        std::vector<G1Element> pks;
        std::vector<G2Element> sigs;
        std::vector<std::vector<uint8_t>> idHashes(n);
        std::vector<Bytes> ids;
        std::vector<PrivateKey> skShares;
        std::vector<G1Element> pkShares;
        std::vector<G2Element> sigShares;

        std::vector<uint8_t> vecHash = getRandomSeed();

        for (size_t i = 0; i < n; i++) {
            idHashes[i] = getRandomSeed();
        }
        ids = std::vector<Bytes>(idHashes.begin(), idHashes.end());

        for (size_t i = 0; i < m; i++) {
            std::vector<uint8_t> buf = getRandomSeed();

            PrivateKey sk = PrivateKey::FromByteVector(buf, true);
            sks.push_back(sk);
            pks.push_back(sk.GetG1Element());
            sigs.push_back(bls::Threshold::Sign(sk, Bytes(vecHash)));
            ASSERT(bls::Threshold::Verify(sk.GetG1Element(), Bytes(vecHash), sigs.back()));
        }

        G2Element sig = bls::Threshold::Sign(sks[0], Bytes(vecHash));

        REQUIRE(bls::Threshold::Verify({pks[0]}, Bytes{vecHash}, {sig}));

        for (size_t i = 0; i < n; i++) {
            PrivateKey skShare = bls::Threshold::PrivateKeyShare(sks, ids[i]);
            G1Element pkShare = bls::Threshold::PublicKeyShare(pks, ids[i]);
            G2Element sigShare1 = bls::Threshold::SignatureShare(sigs, ids[i]);
            REQUIRE(skShare.GetG1Element() == pkShare);

            G2Element sigShare2 = bls::Threshold::Sign(skShare, Bytes(vecHash));
            REQUIRE(sigShare1 == sigShare2);
            REQUIRE(bls::Threshold::Verify({pkShare}, Bytes(vecHash), {sigShare1}));

            skShares.push_back(skShare);
            pkShares.push_back(pkShare);
            sigShares.push_back(sigShare1);
        }

        std::vector<PrivateKey> rsks;
        std::vector<G1Element> rpks;
        std::vector<G2Element> rsigs;
        std::vector<Bytes> rids;
        for (size_t i = 0; i < 2; i++) {
            rsks.push_back(skShares[i]);
            rpks.push_back(pkShares[i]);
            rsigs.push_back(sigShares[i]);
            rids.push_back(ids[i]);
        }
        PrivateKey recSk = bls::Threshold::PrivateKeyRecover(rsks, rids);
        G1Element recPk = bls::Threshold::PublicKeyRecover(rpks, rids);
        G2Element recSig = bls::Threshold::SignatureRecover(rsigs, rids);
        REQUIRE(recSk != sks[0]);
        REQUIRE(recPk != pks[0]);
        REQUIRE(recSig != sig);

        rsks.push_back(skShares[2]);
        rpks.push_back(pkShares[2]);
        rsigs.push_back(sigShares[2]);
        rids.push_back(ids[2]);
        recSk = bls::Threshold::PrivateKeyRecover(rsks, rids);
        recPk = bls::Threshold::PublicKeyRecover(rpks, rids);
        recSig = bls::Threshold::SignatureRecover(rsigs, rids);
        REQUIRE(recSk == sks[0]);
        REQUIRE(recPk == pks[0]);
        REQUIRE(recSig == sig);
    }
}

int main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
