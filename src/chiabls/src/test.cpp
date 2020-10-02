// Copyright 2018 Chia Network Inc

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
#include "catch.hpp"
#include "bls.hpp"
#include "test-utils.hpp"

#include <array>
#include <thread>
using std::string;
using std::vector;
using std::cout;
using std::endl;

using namespace bls;

TEST_CASE("Test vectors") {
    SECTION("Test vectors 1") {
        uint8_t seed1[5] = {1, 2, 3, 4, 5};
        uint8_t seed2[6] = {1, 2, 3, 4, 5, 6};
        uint8_t message1[3] = {7, 8, 9};

        PrivateKey sk1 = PrivateKey::FromSeed(seed1, sizeof(seed1));
        PublicKey pk1 = sk1.GetPublicKey();
        Signature sig1 = sk1.Sign(message1, sizeof(message1));

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, sizeof(seed2));
        PublicKey pk2 = sk2.GetPublicKey();
        Signature sig2 = sk2.Sign(message1, sizeof(message1));

        uint8_t buf[Signature::SIGNATURE_SIZE];
        uint8_t buf2[PrivateKey::PRIVATE_KEY_SIZE];

        REQUIRE(pk1.GetFingerprint() == 0x26d53247);
        REQUIRE(pk2.GetFingerprint() == 0x289bb56e);


        sig1.Serialize(buf);
        sk1.Serialize(buf2);

        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
             == "93eb2e1cb5efcfb31f2c08b235e8203a67265bc6a13d9f0ab77727293b74a357ff0459ac210dc851fcb8a60cb7d393a419915cfcf83908ddbeac32039aaa3e8fea82efcb3ba4f740f20c76df5e97109b57370ae32d9b70d256a98942e5806065");
        REQUIRE(Util::HexStr(buf2, PrivateKey::PRIVATE_KEY_SIZE)
             == "022fb42c08c12de3a6af053880199806532e79515f94e83461612101f9412f9e");

        sig2.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
             == "975b5daa64b915be19b5ac6d47bc1c2fc832d2fb8ca3e95c4805d8216f95cf2bdbb36cc23645f52040e381550727db420b523b57d494959e0e8c0c6060c46cf173872897f14d43b2ac2aec52fc7b46c02c5699ff7a10beba24d3ced4e89c821e");

        vector<Signature> sigs = {sig1, sig2};
        Signature aggSig1 = Signature::AggregateSigs(sigs);

        aggSig1.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
             == "0a638495c1403b25be391ed44c0ab013390026b5892c796a85ede46310ff7d0e0671f86ebe0e8f56bee80f28eb6d999c0a418c5fc52debac8fc338784cd32b76338d629dc2b4045a5833a357809795ef55ee3e9bee532edfc1d9c443bf5bc658");
        REQUIRE(aggSig1.Verify());

        uint8_t message2[3] = {1, 2, 3};
        uint8_t message3[4] = {1, 2, 3, 4};
        uint8_t message4[2] = {1, 2};
        Signature sig3 = sk1.Sign(message2, sizeof(message2));
        Signature sig4 = sk1.Sign(message3, sizeof(message3));
        Signature sig5 = sk2.Sign(message4, sizeof(message4));
        vector<Signature> sigs2 = {sig3, sig4, sig5};
        Signature aggSig2 = Signature::AggregateSigs(sigs2);
        REQUIRE(aggSig2.Verify());
        aggSig2.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
            == "8b11daf73cd05f2fe27809b74a7b4c65b1bb79cc1066bdf839d96b97e073c1a635d2ec048e0801b4a208118fdbbb63a516bab8755cc8d850862eeaa099540cd83621ff9db97b4ada857ef54c50715486217bd2ecb4517e05ab49380c041e159b");
    }

    SECTION("Test vector 2") {
        uint8_t message1[4] = {1, 2, 3, 40};
        uint8_t message2[4] = {5, 6, 70, 201};
        uint8_t message3[5] = {9, 10, 11, 12, 13};
        uint8_t message4[6] = {15, 63, 244, 92, 0, 1};

        uint8_t seed1[5] = {1, 2, 3, 4, 5};
        uint8_t seed2[6] = {1, 2, 3, 4, 5, 6};

        PrivateKey sk1 = PrivateKey::FromSeed(seed1, sizeof(seed1));
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, sizeof(seed2));

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message2, sizeof(message2));
        Signature sig3 = sk2.Sign(message1, sizeof(message1));
        Signature sig4 = sk1.Sign(message3, sizeof(message3));
        Signature sig5 = sk1.Sign(message1, sizeof(message1));
        Signature sig6 = sk1.Sign(message4, sizeof(message4));

        std::vector<Signature> const sigsL = {sig1, sig2};
        const Signature aggSigL = Signature::AggregateSigs(sigsL);

        std::vector<Signature> const sigsR = {sig3, sig4, sig5};
        const Signature aggSigR = Signature::AggregateSigs(sigsR);

        std::vector<Signature> sigs = {aggSigL, aggSigR, sig6};

        Signature aggSig = Signature::AggregateSigs(sigs);

        REQUIRE(aggSig.Verify());

        uint8_t buf[Signature::SIGNATURE_SIZE];
        aggSig.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
            == "07969958fbf82e65bd13ba0749990764cac81cf10d923af9fdd2723f1e3910c3fdb874a67f9d511bb7e4920f8c01232b12e2fb5e64a7c2d177a475dab5c3729ca1f580301ccdef809c57a8846890265d195b694fa414a2a3aa55c32837fddd80");
        vector<Signature> signatures_to_divide = {sig2, sig5, sig6};
        Signature quotient = aggSig.DivideBy(signatures_to_divide);
        aggSig.DivideBy(signatures_to_divide);

        quotient.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
            == "8ebc8a73a2291e689ce51769ff87e517be6089fd0627b2ce3cd2f0ee1ce134b39c4da40928954175014e9bbe623d845d0bdba8bfd2a85af9507ddf145579480132b676f027381314d983a63842fcc7bf5c8c088461e3ebb04dcf86b431d6238f");

        REQUIRE(quotient.Verify());
        REQUIRE(quotient.DivideBy(vector<Signature>()) == quotient);
        signatures_to_divide = {sig6};
        REQUIRE_THROWS(quotient.DivideBy(signatures_to_divide));

        // Should not throw
        signatures_to_divide = {sig1};
        aggSig.DivideBy(signatures_to_divide);

        // Should throw due to not unique
        signatures_to_divide = {aggSigL};
        REQUIRE_THROWS(aggSig.DivideBy(signatures_to_divide));

        Signature sig7 = sk2.Sign(message3, sizeof(message3));
        Signature sig8 = sk2.Sign(message4, sizeof(message4));

        // Divide by aggregate
        std::vector<Signature> sigsR2 = {sig7, sig8};
        Signature aggSigR2 = Signature::AggregateSigs(sigsR2);
        std::vector<Signature> sigsFinal2 = {aggSig, aggSigR2};
        Signature aggSig2 = Signature::AggregateSigs(sigsFinal2);
        std::vector<Signature> divisorFinal2 = {aggSigR2};
        Signature quotient2 = aggSig2.DivideBy(divisorFinal2);

        REQUIRE(quotient2.Verify());
        quotient2.Serialize(buf);
        REQUIRE(Util::HexStr(buf, Signature::SIGNATURE_SIZE)
            == "06af6930bd06838f2e4b00b62911fb290245cce503ccf5bfc2901459897731dd08fc4c56dbde75a11677ccfbfa61ab8b14735fddc66a02b7aeebb54ab9a41488f89f641d83d4515c4dd20dfcf28cbbccb1472c327f0780be3a90c005c58a47d3");
    }

    SECTION("Test vector 3") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        REQUIRE(esk.GetPublicKey().GetFingerprint() == 0xa4700b27);
        uint8_t chainCode[32];
        esk.GetChainCode().Serialize(chainCode);
        REQUIRE(Util::HexStr(chainCode, 32) == "d8b12555b4cc5578951e4a7c80031e22019cc0dce168b3ed88115311b8feb1e3");

        ExtendedPrivateKey esk77 = esk.PrivateChild(77 + (1 << 31));
        esk77.GetChainCode().Serialize(chainCode);
        REQUIRE(Util::HexStr(chainCode, 32) == "f2c8e4269bb3e54f8179a5c6976d92ca14c3260dd729981e9d15f53049fd698b");
        REQUIRE(esk77.GetPrivateKey().GetPublicKey().GetFingerprint() == 0xa8063dcf);

        REQUIRE(esk.PrivateChild(3)
                   .PrivateChild(17)
                   .GetPublicKey()
                   .GetFingerprint() == 0xff26a31f);
        REQUIRE(esk.GetExtendedPublicKey()
                   .PublicChild(3)
                   .PublicChild(17)
                   .GetPublicKey()
                   .GetFingerprint() == 0xff26a31f);
    }
}

TEST_CASE("Key generation") {
    SECTION("Should generate a keypair from a seed") {
        uint8_t seed[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};


        PrivateKey sk = PrivateKey::FromSeed(seed, sizeof(seed));
        PublicKey pk = sk.GetPublicKey();
        REQUIRE(core_get()->code == STS_OK);
        REQUIRE(pk.GetFingerprint() == 0xddad59bb);
    }
}

TEST_CASE("Error handling") {
    SECTION("Should throw on a bad private key") {
        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        uint8_t* skData = Util::SecAlloc<uint8_t>(
                Signature::SIGNATURE_SIZE);
        sk1.Serialize(skData);
        skData[0] = 255;
        REQUIRE_THROWS(PrivateKey::FromBytes(skData));

        Util::SecFree(skData);
    }

    SECTION("Should throw on a bad public key") {
        uint8_t buf[PublicKey::PUBLIC_KEY_SIZE] = {0};
        std::set<int> invalid = {1, 2, 3, 4};

        for (int i = 0; i < 10; i++) {
            buf[0] = (uint8_t)i;
            try {
                PublicKey::FromBytes(buf);
                REQUIRE(invalid.count(i) == 0);
            } catch (std::string& s) {
                REQUIRE(invalid.count(i) != 0);
            }
        }
    }

    SECTION("Should throw on a bad signature") {
        uint8_t buf[Signature::SIGNATURE_SIZE] = {0};
        std::set<int> invalid = {0, 1, 2, 3, 5, 6, 7, 8};

        for (int i = 0; i < 10; i++) {
            buf[0] = (uint8_t)i;
            try {
                Signature::FromBytes(buf);
                REQUIRE(invalid.count(i) == 0);
            } catch (std::string& s) {
                REQUIRE(invalid.count(i) != 0);
            }
        }
    }

    SECTION("Error handling should be thread safe") {
        core_get()->code = 10;
        REQUIRE(core_get()->code == 10);

        ctx_t* ctx1 = core_get();
        bool ctxError = false;

        // spawn a thread and make sure it uses a different context
        std::thread([&]() {
            if (ctx1 == core_get()) {
                ctxError = true;
            }
            if (core_get()->code != STS_OK) {
                ctxError = true;
            }
            // this should not modify the code of the main thread
            core_get()->code = 1;
        }).join();

        REQUIRE(!ctxError);

        // other thread should not modify code
        REQUIRE(core_get()->code == 10);

        // reset so that future test cases don't fail
        core_get()->code = STS_OK;
    }
}

TEST_CASE("Util tests") {
    SECTION("Should convert an int to four bytes") {
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

    SECTION("Should calculate public key fingerprints") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        uint32_t fingerprint = esk.GetPublicKey().GetFingerprint();
        REQUIRE(fingerprint == 0xa4700b27);
    }
}

TEST_CASE("Signatures") {
    SECTION("Should sign and verify") {
        uint8_t message1[7] = {1, 65, 254, 88, 90, 45, 22};

        uint8_t seed[6] = {28, 20, 102, 229, 1, 157};
        PrivateKey sk1 = PrivateKey::FromSeed(seed, sizeof(seed));
        PublicKey pk1 = sk1.GetPublicKey();
        Signature sig1 = sk1.Sign(message1, sizeof(message1));

        sig1.SetAggregationInfo(
                AggregationInfo::FromMsg(pk1, message1, sizeof(message1)));
        REQUIRE(sig1.Verify());

        uint8_t hash[32];
        Util::Hash256(hash, message1, 7);
        Signature sig2 = sk1.SignPrehashed(hash);
        sig2.SetAggregationInfo(
                AggregationInfo::FromMsg(pk1, message1, sizeof(message1)));
        REQUIRE(sig1 == sig2);
        REQUIRE(sig2.Verify());

        // Hashing to g1
        uint8_t mapMsg[0] = {};
        g1_t result;
        uint8_t buf[49];
        ep_map(result, mapMsg, 0);
        g1_write_bin(buf, 49, result, 1);
        REQUIRE(Util::HexStr(buf + 1, 48) == "12fc5ad5a2fbe9d4b6eb0bc16d530e5f263b6d59cbaf26c3f2831962924aa588ab84d46cc80d3a433ce064adb307f256");
    }

    SECTION("Should use copy constructor") {
        uint8_t message1[7] = {1, 65, 254, 88, 90, 45, 22};

        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();
        PrivateKey sk2 = PrivateKey(sk1);

        uint8_t skBytes[PrivateKey::PRIVATE_KEY_SIZE];
        sk2.Serialize(skBytes);
        PrivateKey sk4 = PrivateKey::FromBytes(skBytes);

        PublicKey pk2 = PublicKey(pk1);
        Signature sig1 = sk4.Sign(message1, sizeof(message1));
        Signature sig2 = Signature(sig1);

        REQUIRE(sig2.Verify());
    }

    SECTION("Should use operators") {
        uint8_t message1[7] = {1, 65, 254, 88, 90, 45, 22};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed3[32];
        getRandomSeed(seed3);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey(sk1);
        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();
        PublicKey pk3 = PublicKey(pk2);
        PublicKey pk4 = sk3.GetPublicKey();
        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk1.Sign(message1, sizeof(message1));
        Signature sig3 = sk2.Sign(message1, sizeof(message1));
        Signature sig4 = sk3.Sign(message1, sizeof(message1));

        REQUIRE(sk1 == sk2);
        REQUIRE(sk1 != sk3);
        REQUIRE(pk1 == pk2);
        REQUIRE(pk2 == pk3);
        REQUIRE(pk1 != pk4);
        REQUIRE(sig1 == sig2);
        REQUIRE(sig2 == sig3);
        REQUIRE(sig3 != sig4);

        REQUIRE(pk1.Serialize() == pk2.Serialize());
        REQUIRE(sig1.Serialize() ==  sig2.Serialize());
    }

    SECTION("Should serialize and deserialize") {
        uint8_t message1[7] = {1, 65, 254, 88, 90, 45, 22};

        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        uint8_t* skData = Util::SecAlloc<uint8_t>(
                Signature::SIGNATURE_SIZE);
        sk1.Serialize(skData);
        PrivateKey sk2 = PrivateKey::FromBytes(skData);
        REQUIRE(sk1 == sk2);

        uint8_t pkData[PublicKey::PUBLIC_KEY_SIZE];
        pk1.Serialize(pkData);

        PublicKey pk2 = PublicKey::FromBytes(pkData);
        REQUIRE(pk1 == pk2);

        Signature sig1 = sk1.Sign(message1, sizeof(message1));

        uint8_t sigData[Signature::SIGNATURE_SIZE];
        sig1.Serialize(sigData);

        Signature sig2 = Signature::FromBytes(sigData);
        REQUIRE(sig1 == sig2);
        sig2.SetAggregationInfo(AggregationInfo::FromMsg(
                pk2, message1, sizeof(message1)));

        REQUIRE(sig2.Verify());
        Util::SecFree(skData);

        InsecureSignature sig3 = InsecureSignature::FromBytes(sigData);
        REQUIRE(Signature::FromInsecureSig(sig3) == sig2);
    }

    SECTION("Should not validate a bad sig") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 22};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();

        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        sig2.SetAggregationInfo(AggregationInfo::FromMsg(
                pk1, message1, sizeof(message1)));

        REQUIRE(sig2.Verify() == false);
    }

    SECTION("Should insecurely aggregate and verify aggregate same message") {
        uint8_t message[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t hash[BLS::MESSAGE_HASH_LEN];

        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        Util::Hash256(hash, message, sizeof(message));

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        InsecureSignature sig1 = sk1.SignInsecure(message, sizeof(message));
        InsecureSignature sig2 = sk2.SignInsecure(message, sizeof(message));
        REQUIRE(sig1 != sig2);
        REQUIRE(sig1.Verify({hash}, {sk1.GetPublicKey()}));
        REQUIRE(sig2.Verify({hash}, {sk2.GetPublicKey()}));

        std::vector<InsecureSignature> const sigs = {sig1, sig2};
        std::vector<PublicKey> const pks = {sk1.GetPublicKey(), sk2.GetPublicKey()};
        InsecureSignature aggSig = InsecureSignature::Aggregate(sigs);
        PublicKey aggPk = PublicKey::AggregateInsecure(pks);
        REQUIRE(aggSig.Verify({hash}, {aggPk}));
    }

    SECTION("Should insecurely aggregate and verify aggregate diff messages") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[8] = {100, 2, 254, 88, 90, 45, 24, 1};

        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        uint8_t hash1[BLS::MESSAGE_HASH_LEN];
        uint8_t hash2[BLS::MESSAGE_HASH_LEN];
        Util::Hash256(hash1, message1, sizeof(message1));
        Util::Hash256(hash2, message2, sizeof(message2));

        InsecureSignature sig1 = sk1.SignInsecurePrehashed(hash1);
        InsecureSignature sig2 = sk2.SignInsecurePrehashed(hash2);
        REQUIRE(sig1 != sig2);
        REQUIRE(sig1.Verify({hash1}, {sk1.GetPublicKey()}));
        REQUIRE(sig2.Verify({hash2}, {sk2.GetPublicKey()}));

        std::vector<InsecureSignature> const sigs = {sig1, sig2};
        std::vector<PublicKey> const pks = {sk1.GetPublicKey(), sk2.GetPublicKey()};
        InsecureSignature aggSig = InsecureSignature::Aggregate(sigs);

        // same message verification should fail
        PublicKey aggPk = PublicKey::AggregateInsecure(pks);
        REQUIRE(!aggSig.Verify({hash1}, {aggPk}));
        REQUIRE(!aggSig.Verify({hash2}, {aggPk}));

        // diff message verification should succeed
        std::vector<const uint8_t*> hashes = {hash1, hash2};
        REQUIRE(aggSig.Verify(hashes, pks));
    }

    SECTION("Should securely aggregate and verify aggregate") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[7] = {192, 29, 2, 0, 0, 45, 23};

        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message2, sizeof(message2));

        std::vector<Signature> const sigs = {sig1, sig2};
        Signature aggSig = Signature::AggregateSigs(sigs);

        Signature sig3 = sk1.Sign(message1, sizeof(message1));
        Signature sig4 = sk2.Sign(message2, sizeof(message2));

        std::vector<Signature> const sigs2 = {sig3, sig4};
        Signature aggSig2 = Signature::AggregateSigs(sigs2);
        REQUIRE(sig1 == sig3);
        REQUIRE(sig2 == sig4);
        REQUIRE(aggSig == aggSig2);
        REQUIRE(sig1 != sig2);

        REQUIRE(aggSig.Verify());
    }

    SECTION("Should securely aggregate many signatures, diff message") {
        std::vector<PrivateKey> sks;
        std::vector<Signature> sigs;

        for (int i = 0; i < 80; i++) {
            uint8_t* message = new uint8_t[8];
            message[0] = 0;
            message[1] = 100;
            message[2] = 2;
            message[3] = 59;
            message[4] = 255;
            message[5] = 92;
            message[6] = 5;
            message[7] = i;
            uint8_t seed[32];
            getRandomSeed(seed);
            const PrivateKey sk = PrivateKey::FromSeed(seed, 32);
            const PublicKey pk = sk.GetPublicKey();
            sks.push_back(sk);
            sigs.push_back(sk.Sign(message, sizeof(message)));
            delete[] message;
        }

        Signature aggSig = Signature::AggregateSigs(sigs);

        REQUIRE(aggSig.Verify());
    }

    SECTION("Should insecurely aggregate many signatures, diff message") {
        std::vector<PrivateKey> sks;
        std::vector<PublicKey> pks;
        std::vector<InsecureSignature> sigs;
        std::vector<const uint8_t*> hashes;

        for (int i = 0; i < 80; i++) {
            uint8_t* message = new uint8_t[8];
            uint8_t* hash = new uint8_t[BLS::MESSAGE_HASH_LEN];
            message[0] = 0;
            message[1] = 100;
            message[2] = 2;
            message[3] = 59;
            message[4] = 255;
            message[5] = 92;
            message[6] = 5;
            message[7] = i;
            Util::Hash256(hash, message, 8);
            hashes.push_back(hash);
            uint8_t seed[32];
            getRandomSeed(seed);
            const PrivateKey sk = PrivateKey::FromSeed(seed, 32);
            const PublicKey pk = sk.GetPublicKey();
            sks.push_back(sk);
            pks.push_back(pk);
            sigs.push_back(sk.SignInsecurePrehashed(hash));
            delete[] message;
        }

        InsecureSignature aggSig = InsecureSignature::Aggregate(sigs);

        REQUIRE(aggSig.Verify(hashes, pks));
        std::swap(pks[0], pks[1]);
        REQUIRE(!aggSig.Verify(hashes, pks));
        std::swap(hashes[0], hashes[1]);
        REQUIRE(aggSig.Verify(hashes, pks));

        for (auto& p : hashes) {
            delete[] p;
        }
    }

    SECTION("Should securely aggregate same message") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        uint8_t seed3[32];
        getRandomSeed(seed3);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk3 = sk3.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        Signature sig3 = sk3.Sign(message1, sizeof(message1));

        std::vector<Signature> const sigs = {sig1, sig2, sig3};
        std::vector<PublicKey> const pubKeys = {pk1, pk2, pk3};
        Signature aggSig = Signature::AggregateSigs(sigs);

        const PublicKey aggPubKey = PublicKey::Aggregate(pubKeys);
        aggSig.SetAggregationInfo(AggregationInfo::FromMsg(
                aggPubKey, message1, sizeof(message1)));
        REQUIRE(aggSig.Verify());
    }

    SECTION("Should securely divide signatures") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        uint8_t seed3[32];
        getRandomSeed(seed3);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk3 = sk3.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        Signature sig3 = sk3.Sign(message1, sizeof(message1));

        std::vector<Signature> sigs = {sig1, sig2, sig3};
        Signature aggSig = Signature::AggregateSigs(sigs);

        REQUIRE(sig2.Verify());
        REQUIRE(sig3.Verify());
        std::vector<Signature> divisorSigs = {sig2, sig3};

        REQUIRE(aggSig.Verify());

        REQUIRE(aggSig.GetAggregationInfo()->GetPubKeys().size() == 3);
        const Signature aggSig2 = aggSig.DivideBy(divisorSigs);
        REQUIRE(aggSig.GetAggregationInfo()->GetPubKeys().size() == 3);
        REQUIRE(aggSig2.GetAggregationInfo()->GetPubKeys().size() == 1);

        REQUIRE(aggSig.Verify());
        REQUIRE(aggSig2.Verify());
    }

    SECTION("Should securely divide aggregate signatures") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[7] = {92, 20, 5, 89, 91, 15, 105};
        uint8_t message3[7] = {200, 10, 10, 159, 4, 15, 24};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        uint8_t seed3[32];
        getRandomSeed(seed3);
        uint8_t seed4[32];
        getRandomSeed(seed4);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk3 = sk3.GetPublicKey();

        PrivateKey sk4 = PrivateKey::FromSeed(seed4, 32);
        PublicKey pk4 = sk4.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        Signature sig3 = sk3.Sign(message1, sizeof(message1));
        Signature sig4 = sk4.Sign(message2, sizeof(message2));
        Signature sig5 = sk4.Sign(message1, sizeof(message1));
        Signature sig6 = sk2.Sign(message3, sizeof(message3));

        std::vector<Signature> sigsL = {sig1, sig2};
        std::vector<Signature> sigsC = {sig3, sig4};
        std::vector<Signature> sigsR = {sig5, sig6};
        Signature aggSigL = Signature::AggregateSigs(sigsL);
        Signature aggSigC = Signature::AggregateSigs(sigsC);
        Signature aggSigR = Signature::AggregateSigs(sigsR);

        std::vector<Signature> sigsL2 = {aggSigL, aggSigC};
        Signature aggSigL2 = Signature::AggregateSigs(sigsL2);

        std::vector<Signature> sigsFinal = {aggSigL2, aggSigR};
        Signature aggSigFinal = Signature::AggregateSigs(sigsFinal);

        REQUIRE(aggSigFinal.Verify());
        REQUIRE(aggSigFinal.GetAggregationInfo()->GetPubKeys().size() == 6);
        std::vector<Signature> divisorSigs = {aggSigL, sig6};
        aggSigFinal = aggSigFinal.DivideBy(divisorSigs);
        REQUIRE(aggSigFinal.GetAggregationInfo()->GetPubKeys().size() == 3);
        REQUIRE(aggSigFinal.Verify());

        // Throws when the m/pk pair is not unique within the aggregate (sig1
        // is in both aggSigL2 and sig1.
        std::vector<Signature> sigsFinal2 = {aggSigL2, aggSigR, sig1};
        Signature aggSigFinal2 = Signature::AggregateSigs(sigsFinal2);
        std::vector<Signature> divisorSigs2 = {aggSigL};
        std::vector<Signature> divisorSigs3 = {sig6};
        aggSigFinal2 = aggSigFinal2.DivideBy(divisorSigs3);
        REQUIRE_THROWS(aggSigFinal2.DivideBy(divisorSigs));
    }

    SECTION("Should insecurely aggregate many sigs, same message") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t hash1[BLS::MESSAGE_HASH_LEN];

        std::vector<PrivateKey> sks;
        std::vector<PublicKey> pks;
        std::vector<InsecureSignature> sigs;

        Util::Hash256(hash1, message1, sizeof(message1));

        for (int i = 0; i < 70; i++) {
            uint8_t seed[32];
            getRandomSeed(seed);
            PrivateKey sk = PrivateKey::FromSeed(seed, 32);
            const PublicKey pk = sk.GetPublicKey();
            sks.push_back(sk);
            pks.push_back(pk);
            sigs.push_back(sk.SignInsecure(message1, sizeof(message1)));
        }

        InsecureSignature aggSig = InsecureSignature::Aggregate(sigs);
        const PublicKey aggPubKey = PublicKey::AggregateInsecure(pks);
        REQUIRE(aggSig.Verify({hash1}, {aggPubKey}));
    }

    SECTION("Should securely aggregate many sigs, same message") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};

        std::vector<PrivateKey> sks;
        std::vector<PublicKey> pks;
        std::vector<Signature> sigs;

        for (int i = 0; i < 70; i++) {
            uint8_t seed[32];
            getRandomSeed(seed);
            PrivateKey sk = PrivateKey::FromSeed(seed, 32);
            const PublicKey pk = sk.GetPublicKey();
            sks.push_back(sk);
            pks.push_back(pk);
            sigs.push_back(sk.Sign(message1, sizeof(message1)));
        }

        Signature aggSig = Signature::AggregateSigs(sigs);
        const PublicKey aggPubKey = PublicKey::Aggregate(pks);
        aggSig.SetAggregationInfo(AggregationInfo::FromMsg(
                aggPubKey, message1, sizeof(message1)));
        REQUIRE(aggSig.Verify());
    }

    SECTION("Should have at least one sig, with AggregationInfo") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));

        std::vector<Signature> const sigs = {};
        REQUIRE_THROWS(Signature::AggregateSigs(sigs));

        sig1.SetAggregationInfo(AggregationInfo());
        std::vector<Signature> const sigs2 = {sig1};
        REQUIRE_THROWS(Signature::AggregateSigs(sigs2));
    }

    SECTION("Should perform batch verification") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[8] = {10, 28, 254, 88, 90, 45, 29, 38};
        uint8_t message3[9] = {10, 28, 254, 88, 90, 45, 29, 38, 177};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        uint8_t seed3[32];
        getRandomSeed(seed3);
        uint8_t seed4[32];
        getRandomSeed(seed4);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk3 = sk3.GetPublicKey();

        PrivateKey sk4 = PrivateKey::FromSeed(seed4, 32);
        PublicKey pk4 = sk4.GetPublicKey();


        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        Signature sig3 = sk3.Sign(message2, sizeof(message2));
        Signature sig4 = sk4.Sign(message3, sizeof(message3));
        Signature sig5 = sk3.Sign(message1, sizeof(message1));
        Signature sig6 = sk2.Sign(message1, sizeof(message1));
        Signature sig7 = sk4.Sign(message2, sizeof(message2));

        std::vector<Signature> const sigs =
                {sig1, sig2, sig3, sig4, sig5, sig6, sig7};
        std::vector<PublicKey> const pubKeys =
                {pk1, pk2, pk3, pk4, pk3, pk2, pk4};
        std::vector<uint8_t*> const messages =
                {message1, message1, message2, message3, message1,
                 message1, message2};
        std::vector<size_t> const messageLens =
                {sizeof(message1), sizeof(message1), sizeof(message2),
                 sizeof(message3), sizeof(message1), sizeof(message1),
                 sizeof(message2)};

        // Verifier generates a batch signature for efficiency
        Signature aggSig = Signature::AggregateSigs(sigs);
        REQUIRE(aggSig.Verify());
    }

    SECTION("Should perform batch verification with cache optimization") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[8] = {10, 28, 254, 88, 90, 45, 29, 38};
        uint8_t message3[9] = {10, 28, 254, 88, 90, 45, 29, 38, 177};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);
        uint8_t seed3[32];
        getRandomSeed(seed3);
        uint8_t seed4[32];
        getRandomSeed(seed4);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        PrivateKey sk3 = PrivateKey::FromSeed(seed3, 32);
        PublicKey pk3 = sk3.GetPublicKey();

        PrivateKey sk4 = PrivateKey::FromSeed(seed4, 32);
        PublicKey pk4 = sk4.GetPublicKey();


        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));
        Signature sig3 = sk3.Sign(message2, sizeof(message2));
        Signature sig4 = sk4.Sign(message3, sizeof(message3));
        Signature sig5 = sk3.Sign(message1, sizeof(message1));
        Signature sig6 = sk2.Sign(message1, sizeof(message1));
        Signature sig7 = sk4.Sign(message2, sizeof(message2));

        std::vector<Signature> const sigs =
                {sig1, sig2, sig3, sig4, sig5, sig6, sig7};

        REQUIRE(sig1.Verify());
        REQUIRE(sig3.Verify());
        REQUIRE(sig4.Verify());
        REQUIRE(sig7.Verify());
        std::vector<Signature> cache = {sig1, sig3, sig4, sig7};

        // Verifier generates a batch signature for efficiency
        Signature aggSig = Signature::AggregateSigs(sigs);

        const Signature aggSig2 = aggSig.DivideBy(cache);
        REQUIRE(aggSig.Verify());
        REQUIRE(aggSig2.Verify());
    }

    SECTION("Should aggregate same message with agg sk") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);
        PublicKey pk2 = sk2.GetPublicKey();

        std::vector<PrivateKey> const privateKeys = {sk1, sk2};
        std::vector<PublicKey> const pubKeys = {pk1, pk2};
        const PrivateKey aggSk = PrivateKey::Aggregate(
                privateKeys, pubKeys);

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message1, sizeof(message1));

        Signature aggSig2 = aggSk.Sign(message1, sizeof(message1));

        std::vector<Signature> const sigs = {sig1, sig2};
        std::vector<uint8_t*> const messages = {message1, message1};
        std::vector<size_t> const messageLens = {sizeof(message1), sizeof(message1)};
        Signature aggSig = Signature::AggregateSigs(sigs);
        ASSERT(aggSig == aggSig2);

        const PublicKey aggPubKey = PublicKey::Aggregate(pubKeys);
        REQUIRE(aggSig.Verify());
        REQUIRE(aggSig2.Verify());
    }
}

TEST_CASE("HD keys") {
    SECTION("Should create an extended private key from seed") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));

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
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        PublicKey pk1 = esk.PrivateChild(238757).GetPublicKey();
        PublicKey pk2 = epk.PublicChild(238757).GetPublicKey();

        REQUIRE(pk1 == pk2);

        PrivateKey sk3 = esk.PrivateChild(0)
                              .PrivateChild(3)
                              .PrivateChild(8)
                              .PrivateChild(1)
                              .GetPrivateKey();

        PublicKey pk4 = epk.PublicChild(0)
                              .PublicChild(3)
                              .PublicChild(8)
                              .PublicChild(1)
                              .GetPublicKey();
        REQUIRE(sk3.GetPublicKey() == pk4);

        Signature sig = sk3.Sign(seed, sizeof(seed));

        REQUIRE(sig.Verify());
    }

    SECTION("Should prevent hardened pk derivation") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        ExtendedPrivateKey sk = esk.PrivateChild((1 << 31) + 3);
        REQUIRE_THROWS(epk.PublicChild((1 << 31) + 3));
    }

    SECTION("Should derive public child from parent") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 0, 0, 0};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        ExtendedPublicKey pk1 = esk.PublicChild(13);
        ExtendedPublicKey pk2 = epk.PublicChild(13);

        REQUIRE(pk1 == pk2);
    }

    SECTION("Should cout structures") {
        uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 0, 0, 0};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        cout << epk << endl;
        cout << epk.GetPublicKey() << endl;
        cout << epk.GetChainCode() << endl;

        Signature sig1 = esk.GetPrivateKey()
                               .Sign(seed, sizeof(seed));
        cout << sig1 << endl;
    }

    SECTION("Should serialize extended keys") {
        uint8_t seed[] = {1, 50, 6, 244, 25, 199, 1, 25};
        ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(
                seed, sizeof(seed));
        ExtendedPublicKey epk = esk.GetExtendedPublicKey();

        PublicKey pk1 = esk.PrivateChild(238757).GetPublicKey();
        PublicKey pk2 = epk.PublicChild(238757).GetPublicKey();

        REQUIRE(pk1 == pk2);

        ExtendedPrivateKey sk3 = esk.PrivateChild(0)
                              .PrivateChild(3)
                              .PrivateChild(8)
                              .PrivateChild(1);

        ExtendedPublicKey pk4 = epk.PublicChild(0)
                              .PublicChild(3)
                              .PublicChild(8)
                              .PublicChild(1);
        uint8_t buffer1[ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE];
        uint8_t buffer2[ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE];
        uint8_t buffer3[ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE];

        sk3.Serialize(buffer1);
        sk3.GetExtendedPublicKey().Serialize(buffer2);
        pk4.Serialize(buffer3);
        REQUIRE(std::memcmp(buffer2, buffer3,
                ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE) == 0);
    }
}

TEST_CASE("AggregationInfo") {
    SECTION("Should create object") {
        uint8_t message1[7] = {1, 65, 254, 88, 90, 45, 22};
        uint8_t message2[8] = {1, 65, 254, 88, 90, 45, 22, 12};
        uint8_t message3[8] = {2, 65, 254, 88, 90, 45, 22, 12};
        uint8_t message4[8] = {3, 65, 254, 88, 90, 45, 22, 12};
        uint8_t message5[8] = {4, 65, 254, 88, 90, 45, 22, 12};
        uint8_t message6[8] = {5, 65, 254, 88, 90, 45, 22, 12};
        uint8_t messageHash1[32];
        uint8_t messageHash2[32];
        uint8_t messageHash3[32];
        uint8_t messageHash4[32];
        uint8_t messageHash5[32];
        uint8_t messageHash6[32];
        Util::Hash256(messageHash1, message1, 7);
        Util::Hash256(messageHash2, message2, 8);
        Util::Hash256(messageHash3, message3, 8);
        Util::Hash256(messageHash4, message4, 8);
        Util::Hash256(messageHash5, message5, 8);
        Util::Hash256(messageHash6, message6, 8);

        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        getRandomSeed(seed);
        PrivateKey sk2 = PrivateKey::FromSeed(seed, 32);
        getRandomSeed(seed);
        PrivateKey sk3 = PrivateKey::FromSeed(seed, 32);
        getRandomSeed(seed);
        PrivateKey sk4 = PrivateKey::FromSeed(seed, 32);
        getRandomSeed(seed);
        PrivateKey sk5 = PrivateKey::FromSeed(seed, 32);
        getRandomSeed(seed);
        PrivateKey sk6 = PrivateKey::FromSeed(seed, 32);

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();
        PublicKey pk3 = sk3.GetPublicKey();
        PublicKey pk4 = sk4.GetPublicKey();
        PublicKey pk5 = sk5.GetPublicKey();
        PublicKey pk6 = sk6.GetPublicKey();

        AggregationInfo a1 = AggregationInfo::FromMsgHash(pk1, messageHash1);
        AggregationInfo a2 = AggregationInfo::FromMsgHash(pk2, messageHash2);
        std::vector<AggregationInfo> infosA = {a1, a2};
        std::vector<AggregationInfo> infosAcopy = {a2, a1};

        AggregationInfo a3 = AggregationInfo::FromMsgHash(pk3, messageHash1);
        AggregationInfo a4 = AggregationInfo::FromMsgHash(pk4, messageHash1);
        std::vector<AggregationInfo> infosB = {a3, a4};
        std::vector<AggregationInfo> infosBcopy = {a4, a3};
        std::vector<AggregationInfo> infosC = {a1, a2, a3, a4};

        AggregationInfo a5 = AggregationInfo::MergeInfos(infosA);
        AggregationInfo a5b = AggregationInfo::MergeInfos(infosAcopy);
        AggregationInfo a6 = AggregationInfo::MergeInfos(infosB);
        AggregationInfo a6b = AggregationInfo::MergeInfos(infosBcopy);
        std::vector<AggregationInfo> infosD = {a5, a6};

        AggregationInfo a7 = AggregationInfo::MergeInfos(infosC);
        AggregationInfo a8 = AggregationInfo::MergeInfos(infosD);

        REQUIRE(a5 == a5b);
        REQUIRE(a5 != a6);
        REQUIRE(a6 == a6b);

        std::vector<AggregationInfo> infosE = {a1, a3, a4};
        AggregationInfo a9 = AggregationInfo::MergeInfos(infosE);
        std::vector<AggregationInfo> infosF = {a2, a9};
        AggregationInfo a10 = AggregationInfo::MergeInfos(infosF);

        REQUIRE(a10 == a7);

        AggregationInfo a11 = AggregationInfo::FromMsgHash(pk1, messageHash1);
        AggregationInfo a12 = AggregationInfo::FromMsgHash(pk2, messageHash2);
        AggregationInfo a13 = AggregationInfo::FromMsgHash(pk3, messageHash3);
        AggregationInfo a14 = AggregationInfo::FromMsgHash(pk4, messageHash4);
        AggregationInfo a15 = AggregationInfo::FromMsgHash(pk5, messageHash5);
        AggregationInfo a16 = AggregationInfo::FromMsgHash(pk6, messageHash6);
        AggregationInfo a17 = AggregationInfo::FromMsgHash(pk6, messageHash5);
        AggregationInfo a18 = AggregationInfo::FromMsgHash(pk5, messageHash6);

        // Tree L
        std::vector<AggregationInfo> L1 = {a15, a17};
        std::vector<AggregationInfo> L2 = {a11, a13};
        std::vector<AggregationInfo> L3 = {a18, a14};

        AggregationInfo a19 = AggregationInfo::MergeInfos(L1);
        AggregationInfo a20 = AggregationInfo::MergeInfos(L2);
        AggregationInfo a21 = AggregationInfo::MergeInfos(L3);

        std::vector<AggregationInfo> L4 = {a21, a16};
        std::vector<AggregationInfo> L5 = {a19, a20};
        AggregationInfo a22 = AggregationInfo::MergeInfos(L4);
        AggregationInfo a23 = AggregationInfo::MergeInfos(L5);

        std::vector<AggregationInfo> L6 = {a22, a12};
        AggregationInfo a24 = AggregationInfo::MergeInfos(L6);
        std::vector<AggregationInfo> L7 = {a23, a24};
        AggregationInfo LFinal = AggregationInfo::MergeInfos(L7);

        // Tree R
        std::vector<AggregationInfo> R1 = {a17, a12, a11, a15};
        std::vector<AggregationInfo> R2 = {a14, a18};

        AggregationInfo a25 = AggregationInfo::MergeInfos(R1);
        AggregationInfo a26 = AggregationInfo::MergeInfos(R2);

        std::vector<AggregationInfo> R3 = {a26, a16};

        AggregationInfo a27 = AggregationInfo::MergeInfos(R3);

        std::vector<AggregationInfo> R4 = {a27, a13};
        AggregationInfo a28 = AggregationInfo::MergeInfos(R4);
        std::vector<AggregationInfo> R5 = {a25, a28};

        AggregationInfo RFinal = AggregationInfo::MergeInfos(R5);

        REQUIRE(LFinal == RFinal);
    }

    SECTION("Should aggregate with multiple levels.") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[8] = {192, 29, 2, 0, 0, 45, 23, 192};
        uint8_t message3[7] = {52, 29, 2, 0, 0, 45, 102};
        uint8_t message4[7] = {99, 29, 2, 0, 0, 45, 222};

        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message2, sizeof(message2));
        Signature sig3 = sk2.Sign(message1, sizeof(message1));
        Signature sig4 = sk1.Sign(message3, sizeof(message3));
        Signature sig5 = sk1.Sign(message4, sizeof(message4));
        Signature sig6 = sk1.Sign(message1, sizeof(message1));

        std::vector<Signature> const sigsL = {sig1, sig2};
        std::vector<PublicKey> const pksL = {pk1, pk2};
        const Signature aggSigL = Signature::AggregateSigs(sigsL);

        std::vector<Signature> const sigsR = {sig3, sig4, sig6};
        const Signature aggSigR = Signature::AggregateSigs(sigsR);

        std::vector<PublicKey> pk1Vec = {pk1};

        std::vector<Signature> sigs = {aggSigL, aggSigR, sig5};

        const Signature aggSig = Signature::AggregateSigs(sigs);

        REQUIRE(aggSig.Verify());
    }

    SECTION("Should aggregate with multiple levels, degenerate") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t seed[32];
        getRandomSeed(seed);
        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PublicKey pk1 = sk1.GetPublicKey();
        Signature aggSig = sk1.Sign(message1, sizeof(message1));

        for (size_t i = 0; i < 10; i++) {
            getRandomSeed(seed);
            PrivateKey sk = PrivateKey::FromSeed(seed, 32);
            PublicKey pk = sk.GetPublicKey();
            Signature sig = sk.Sign(message1, sizeof(message1));
            std::vector<Signature> sigs = {aggSig, sig};
            aggSig = Signature::AggregateSigs(sigs);
        }
        REQUIRE(aggSig.Verify());
        uint8_t sigSerialized[Signature::SIGNATURE_SIZE];
        aggSig.Serialize(sigSerialized);

        const Signature aggSig2 = Signature::FromBytes(sigSerialized);
        REQUIRE(aggSig2.Verify() == false);
    }

    SECTION("Should aggregate with multiple levels, different messages") {
        uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
        uint8_t message2[7] = {192, 29, 2, 0, 0, 45, 23};
        uint8_t message3[7] = {52, 29, 2, 0, 0, 45, 102};
        uint8_t message4[7] = {99, 29, 2, 0, 0, 45, 222};

        uint8_t seed[32];
        getRandomSeed(seed);
        uint8_t seed2[32];
        getRandomSeed(seed2);

        PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
        PrivateKey sk2 = PrivateKey::FromSeed(seed2, 32);

        PublicKey pk1 = sk1.GetPublicKey();
        PublicKey pk2 = sk2.GetPublicKey();

        Signature sig1 = sk1.Sign(message1, sizeof(message1));
        Signature sig2 = sk2.Sign(message2, sizeof(message2));
        Signature sig3 = sk2.Sign(message3, sizeof(message4));
        Signature sig4 = sk1.Sign(message4, sizeof(message4));

        std::vector<Signature> const sigsL = {sig1, sig2};
        std::vector<PublicKey> const pksL = {pk1, pk2};
        std::vector<uint8_t*> const messagesL = {message1, message2};
        std::vector<size_t> const messageLensL = {sizeof(message1),
                                             sizeof(message2)};
        const Signature aggSigL = Signature::AggregateSigs(sigsL);

        std::vector<Signature> const sigsR = {sig3, sig4};
        std::vector<PublicKey> const pksR = {pk2, pk1};
        std::vector<uint8_t*> const messagesR = {message3, message4};
        std::vector<size_t> const messageLensR = {sizeof(message3),
                                             sizeof(message4)};
        const Signature aggSigR = Signature::AggregateSigs(sigsR);

        std::vector<Signature> sigs = {aggSigL, aggSigR};
        std::vector<std::vector<PublicKey> > pks = {pksL, pksR};
        std::vector<std::vector<uint8_t*> > messages = {messagesL, messagesR};
        std::vector<std::vector<size_t> > messageLens = {messageLensL, messageLensR};

        const Signature aggSig = Signature::AggregateSigs(sigs);

        std::vector<PublicKey> allPks = {pk1, pk2, pk2, pk1};
        std::vector<uint8_t*> allMessages = {message1, message2,
                                              message3, message4};
        std::vector<size_t> allMessageLens = {sizeof(message1), sizeof(message2),
                                         sizeof(message3), sizeof(message4)};

        REQUIRE(aggSig.Verify());
    }
    SECTION("README") {
        // Example seed, used to generate private key. Always use
        // a secure RNG with sufficient entropy to generate a seed.
        uint8_t seed[] = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                        19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                        82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};

        PrivateKey sk = PrivateKey::FromSeed(seed, sizeof(seed));
        PublicKey pk = sk.GetPublicKey();

        uint8_t msg[] = {100, 2, 254, 88, 90, 45, 23};

        Signature sig = sk.Sign(msg, sizeof(msg));

        uint8_t skBytes[PrivateKey::PRIVATE_KEY_SIZE];  // 32 byte array
        uint8_t pkBytes[PublicKey::PUBLIC_KEY_SIZE];    // 48 byte array
        uint8_t sigBytes[Signature::SIGNATURE_SIZE];    // 96 byte array

        sk.Serialize(skBytes);   // 32 bytes
        pk.Serialize(pkBytes);   // 48 bytes
        sig.Serialize(sigBytes); // 96 bytes
        // Takes array of 32 bytes
        sk = PrivateKey::FromBytes(skBytes);

        // Takes array of 48 bytes
        pk = PublicKey::FromBytes(pkBytes);

        // Takes array of 96 bytes
        sig = Signature::FromBytes(sigBytes);
        // Add information required for verification, to sig object
        sig.SetAggregationInfo(AggregationInfo::FromMsg(pk, msg, sizeof(msg)));

        bool ok = sig.Verify();
        // Generate some more private keys
        seed[0] = 1;
        PrivateKey sk1 = PrivateKey::FromSeed(seed, sizeof(seed));
        seed[0] = 2;
        PrivateKey sk2 = PrivateKey::FromSeed(seed, sizeof(seed));

        // Generate first sig
        PublicKey pk1 = sk1.GetPublicKey();
        Signature sig1 = sk1.Sign(msg, sizeof(msg));

        // Generate second sig
        PublicKey pk2 = sk2.GetPublicKey();
        Signature sig2 = sk2.Sign(msg, sizeof(msg));

        // Aggregate signatures together
        std::vector<Signature> sigs = {sig1, sig2};
        Signature aggSig = Signature::AggregateSigs(sigs);

        // For same message, public keys can be aggregated into one.
        // The signature can be verified the same as a single signature,
        // using this public key.
        std::vector<PublicKey> pubKeys = {pk1, pk2};
        PublicKey aggPubKey = PublicKey::Aggregate(pubKeys);
        // Generate one more key
        seed[0] = 3;
        PrivateKey sk3 = PrivateKey::FromSeed(seed, sizeof(seed));
        PublicKey pk3 = sk3.GetPublicKey();
        uint8_t msg2[] = {100, 2, 254, 88, 90, 45, 23};

        // Generate the signatures, assuming we have 3 private keys
        sig1 = sk1.Sign(msg, sizeof(msg));
        sig2 = sk2.Sign(msg, sizeof(msg));
        Signature sig3 = sk3.Sign(msg2, sizeof(msg2));

        // They can be noninteractively combined by anyone
        // Aggregation below can also be done by the verifier, to
        // make batch verification more efficient
        std::vector<Signature> sigsL = {sig1, sig2};
        Signature aggSigL = Signature::AggregateSigs(sigsL);

        // Arbitrary trees of aggregates
        std::vector<Signature> sigsFinal = {aggSigL, sig3};
        Signature aggSigFinal = Signature::AggregateSigs(sigsFinal);

        // Serialize the final signature
        aggSigFinal.Serialize(sigBytes);
        // Deserialize aggregate signature
        aggSigFinal = Signature::FromBytes(sigBytes);

        // Create aggregation information (or deserialize it)
        AggregationInfo a1 = AggregationInfo::FromMsg(pk1, msg, sizeof(msg));
        AggregationInfo a2 = AggregationInfo::FromMsg(pk2, msg, sizeof(msg));
        AggregationInfo a3 = AggregationInfo::FromMsg(pk3, msg2, sizeof(msg2));
        std::vector<AggregationInfo> infos = {a1, a2};
        AggregationInfo a1a2 = AggregationInfo::MergeInfos(infos);
        std::vector<AggregationInfo> infos2 = {a1a2, a3};
        AggregationInfo aFinal = AggregationInfo::MergeInfos(infos2);

        // Verify final signature using the aggregation info
        aggSigFinal.SetAggregationInfo(aFinal);
        ok = aggSigFinal.Verify();

        // If you previously verified a signature, you can also divide
        // the aggregate signature by the signature you already verified.
        ok = aggSigL.Verify();
        std::vector<Signature> cache = {aggSigL};
        aggSigFinal = aggSigFinal.DivideBy(cache);

        // Final verification is now more efficient
        ok = aggSigFinal.Verify();

        std::vector<PrivateKey> privateKeysList = {sk1, sk2};
        std::vector<PublicKey> pubKeysList = {pk1, pk2};

        // Create an aggregate private key, that can generate
        // aggregate signatures
        const PrivateKey aggSk = PrivateKey::Aggregate(
                privateKeysList, pubKeysList);

        Signature aggSig3 = aggSk.Sign(msg, sizeof(msg));
    }
}

TEST_CASE("Threshold Signatures") {
    SECTION("Secret Key Shares") {
        size_t m = 3;
        size_t n = 5;

        std::vector<PrivateKey> sks;
        std::vector<PublicKey> pks;
        std::vector<InsecureSignature> sigs;
        std::vector<std::array<uint8_t, BLS::ID_SIZE>> ids(n);
        std::vector<PrivateKey> skShares;
        std::vector<PublicKey> pkShares;
        std::vector<InsecureSignature> sigShares;

        uint8_t hash[32];
        getRandomSeed(hash);

        for (size_t i = 0; i < n; i++) {
            getRandomSeed(ids[i].data());
        }

        for (size_t i = 0; i < m; i++) {
            uint8_t buf[32];
            getRandomSeed(buf);

            PrivateKey sk = PrivateKey::FromSeed(buf, 32);
            sks.push_back(sk);
            pks.push_back(sk.GetPublicKey());
            sigs.push_back(sk.SignInsecurePrehashed(hash));
            ASSERT(sigs.back().Verify({hash}, {sk.GetPublicKey()}));
        }

        InsecureSignature sig = sks[0].SignInsecurePrehashed(hash);
        REQUIRE(sig.Verify({hash}, {pks[0]}));

        for (size_t i = 0; i < n; i++) {
            PrivateKey skShare = BLS::PrivateKeyShare(sks, ids[i].data());
            PublicKey pkShare = BLS::PublicKeyShare(pks, ids[i].data());
            InsecureSignature sigShare1 = BLS::SignatureShare(sigs, ids[i].data());
            REQUIRE(skShare.GetPublicKey() == pkShare);

            InsecureSignature sigShare2 = skShare.SignInsecurePrehashed(hash);
            REQUIRE(sigShare1 == sigShare2);
            REQUIRE(sigShare1.Verify({hash}, {pkShare}));

            skShares.push_back(skShare);
            pkShares.push_back(pkShare);
            sigShares.push_back(sigShare1);
        }

        std::vector<PrivateKey> rsks;
        std::vector<PublicKey> rpks;
        std::vector<InsecureSignature> rsigs;
        std::vector<const uint8_t*> rids;
        for (size_t i = 0; i < 2; i++) {
            rsks.push_back(skShares[i]);
            rpks.push_back(pkShares[i]);
            rsigs.push_back(sigShares[i]);
            rids.push_back(ids[i].data());
        }
        PrivateKey recSk = BLS::RecoverPrivateKey(rsks, rids);
        PublicKey recPk = BLS::RecoverPublicKey(rpks, rids);
        InsecureSignature recSig = BLS::RecoverSig(rsigs, rids);
        REQUIRE(recSk != sks[0]);
        REQUIRE(recPk != pks[0]);
        REQUIRE(recSig != sig);

        rsks.push_back(skShares[2]);
        rpks.push_back(pkShares[2]);
        rsigs.push_back(sigShares[2]);
        rids.push_back(ids[2].data());
        recSk = BLS::RecoverPrivateKey(rsks, rids);
        recPk = BLS::RecoverPublicKey(rpks, rids);
        recSig = BLS::RecoverSig(rsigs, rids);
        REQUIRE(recSk == sks[0]);
        REQUIRE(recPk == pks[0]);
        REQUIRE(recSig == sig);
    }
}

int main(int argc, char* argv[]) {
    int result = Catch::Session().run(argc, argv);
    return result;
}
