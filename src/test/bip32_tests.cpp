// Copyright (c) 2013-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <base58.h>
#include <key.h>
#include <key/extkey.h>
#include <uint256.h>
#include <util.h>
#include <utilstrencodings.h>
#include <test/test_bitcoin.h>

#include <string>
#include <vector>

struct TestDerivation {
    std::string pub;
    std::string prv;
    unsigned int nChild;
};

struct TestVector {
    std::string strHexMaster;
    std::vector<TestDerivation> vDerive;

    explicit TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

    TestVector& operator()(std::string pub, std::string prv, unsigned int nChild) {
        vDerive.push_back(TestDerivation());
        TestDerivation &der = vDerive.back();
        der.pub = pub;
        der.prv = prv;
        der.nChild = nChild;
        return *this;
    }
};

TestVector test1 =
  TestVector("000102030405060708090a0b0c0d0e0f")
    ("PPARTKMMf4AUDYzRSCijXD2q3gcT5KwjATBsPXMEDbu2Vi6GvpMD8AjJiEQobuJwbqNxDRjCS2ar17bCGBB6Zucp8o3N6SQ4vzdam6gaSyhErU4U",
     "XPARHAr37YxmFP8wykrnN4tBUuFyfYbnB6gLntLsPbLpN8qfbAbkmz5nZRqEWcJc2SHY2VRzoo3Tg4X8epvmq4odD6HLvP4UjAje1SSr7C7Y6HXx",
     0x80000000)
    ("PPARTKPd53smgeFYRMKsEQnhgZeZBemHWqGMQCxf7ZzQcgHUDBdWJaxqoMZA1gaWJagrkRwUztpPRrEGM9a4hreYWrvppe2sBuwUycci1HQ7x3vN",
     "XPARHAtJXYg4iUQ4xuTv5Ge47nJ5msRLXUkpoZxJHZSCV72rsXt3xQKKeYyavPZxMqzuzCUtWALnYSsyXYgoYiPuPtMRSSw9sQhLu9TUxdJxX8f6",
     1)
    ("PPARTKRoCFfKa2xRVBmug93bZAPSqi9GcqJ5h9HKxAGBRcvGRRB1SJmG7K84F6exszda6Pf73gEqXxpkpjsoXKyiQYaekwEi3PaVFFMEnaHGuhY6",
     "XPARHAvUekTcbs6x2juxWztwzP2yRvoKdUnZ6WGy89hyJ3ff5mRZ687jxWYV9od9RSvh2eZeByANyXavwgdKmqjhNfWusxi7bUT7kpFoLAEK5JMq",
     0x80000002)
    ("PPARTKUQUJC9RjqGu3whmPdzvXPUywJ996qfgUpZdQbUanndrq2CZ7X6YDZcAeg9iSW1QkBCnHcp9LmKoCCwp35Fwc2obX8gBUXYvy39vfZyjuCg",
     "XPARHAy5vnzSTZyoSc5kcFVMMk31a9xC9kL95qpCoQ3GTDY2XBGkCvsaPQz35MfN3RACXA2faZdKtwzkquq4XRkcZncMYKKg9dPMhRXYYnpG9F4R",
     2)
    ("PPARTKWds8dGNvJwt8RBV5uD1QteJ27RsjRysvJSvfZ147ECUC7pvBHnuR956L196rJA9DVKxRdJ6PWDukdHp8Nh9KdGrRvENJiSkyj4mpAghRTg",
     "XPARHB1KKdRZQkTURgZEKwkZSdYAtEmUtNvTHHJ66eznvXyb8YNNZzeGkcZW12ymVspbxDpoHBYZTrpD1P1xydB9uyzwaiZmFBUBQaW1iJEhoWbQ",
     1000000000)
    ("PPARTKYMdcJsd3SL5ekxwCSmjibeHpM1BYvnuiYqUuqPCKJaxuFEJhi3ZgFE9xq43hZErhBcVvyDtvhvWM3gqEEBzAtRyJTeT7C2Pnggh4LwtqoA",
     "XPARHB33677AesardCu1n4J8AwFAt314CCRGK5YUeuHB4k3ydFVmxX4XQsff4fqYhnjj2qECXxuYQhrk8PByMAFAe6xb844XUJNw7rqa53Pi4eQg",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("PPARTKMMf4AUDYzRSCLErKBuL8pAqa8sb8p8DCw1wtqmKgUPo7LEKLt5efJyCFR2aYdKBLHSDwFWAWfhWtxpo2ojHvxN3JLGPq85RR4FZFfwSWHQ",
     "XPARHAr37YxmFP8wykUHhB3FmMThRnnvbnJbcZvf7tHZC7DnTTamyAEZVrjQ6xNP5co67jUkEYKv8LdAqvTVw7E1uhefgqUYyK5Yk1hULZ65k6Dp",
     0)
    ("PPARTKQdQKtAB31FWibq9SyHDCcaMw6cqqncVm6mK4enmktFkoJgHv9Rip9YSpszULiKcYbvPoUVuukYV48FUuxRiGZ2KaP14jUDoYYNDgsBefuk",
     "XPARHAuJrpgTCs9n4GjszJpdeRG6x9kfrVH5u86QV46aeBdeR9ZDwjVua1ZyMXsetFrvx98z4SLKrmnayGy8kfWWxdsq5qey2hZiV97NtvLUxQeP",
     0xFFFFFFFF)
    ("PPARTKRnTaVBhQuRctU3KsRGrfxFvKqqkPGnBrFqqz3hYQd29FbukY2r32DL3SGEb6SviL7qwTKMXCxy5io7pgrupzhUx7Td86CUpQvdpXbRBaXT",
     "XPARHAvTv5HUjF3xASc6AjGdHtbnWYVtm2mFbDFV1yVVQqNQobrTQMPKtDdkx9E8rz7fS3NJxDMXVpo65iCsXPt86DBGeGPCNU5SjvtiRgjV3aym",
     1)
    ("PPARTKUbRzWAdaYioz5iTw5RRduC9fNiFVThTAhLhq7MsiWNavRmuqYNjv7iRH4RitKczvJJVHxSkr6GuobNqbmLnG2wrhjpyJjv2LhJmn53fSDC",
     "XPARHAyGtVJTfQhFMYDmJnvmrrYijt2mG8xArXgyspZ9k9FmFGgKZetrb7Y9Kz2LV1ps8MKfexebtELmJ2uegSmeUDf5wQ9jBKBR8Qh9AAP4eG4E",
     0xFFFFFFFE)
    ("PPARTKVmTuU6zBwgXH2x4fZEy1QtLa7BcYSKxCQ4bMq4HZGdqNn6qwDJkSe5WuiZE5h9Ssjx86TWTthbmTEYEHeTHyPeJye9Ts31SCvPMK2wwixe",
     "XPARHAzSvQGQ226D4qAzuXQbQE4QvnmEdBvoMZPhmMGr9z22Vj2eVkZnbe4WRcj4seFFQttxMGUNsciLsr2k3xiYxuVm7E6GVTPSkkH9gE9T3hqH",
     2)
    ("PPARTKX8VruKVi7zm2J8xPdNezAGJyAiinmF2rZgUmcQnC4VRSySR6rEWdD2Wz7dWDGW9PoAMNP1snQckwfbj7k9CHeVtcoyYoQJAtDaXyh2twFh",
     "XPARHB1oxMhcXYGXJaSBoFUj6ConuBpmjSFiSDZKem4Cecot5oDz4vCiMpdTRh8jsoGLUwCp5ZE1h4pBZtadZh2vBxUVq2wmzWoaUSeVEJjizfsx",
     0);

TestVector test3 =
  TestVector("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be")
    ("PPARTKMMf4AUDYzRSBPh1qaQ8LoRsSnCgtgHUvGnWJ8nsxAUb5aTb12KccaCijQZZNDtpWNC5mFPAuLXNLGThsWmjMJ2BRaAcZgXGCmmFn7Ntz1j",
     "XPARHAr37YxmFP8wyjXjrhRkZZSxTfSFhYAktHGRgHaakNusFRq1EpNoTozddSN7FZpKSpBQz7FcEF3uZH77FLfALV8AwMm7cWWtiJR8hCzrUe75",
      0x80000000)
    ("PPARTKPiro8JMEMnnNvnR4ijdQ4BY98p53XhPKm2T7NUitUPxLbb3aX6Uh2zS38Q56aVZhjgvYD8ukHirxjQ3okhuvYJ86g7W7Y24gx8ASXM6ArD",
     "XPARHAtQKHvbP4WKKw4qFva64chi8Mns5h2Angkfd6pGbKDncgr8hPsaKtTRLk8TbumeNGL4Q8CQ6tnY1vekPusk6EtNZnu2fFgAXRS6MAQjzukJ",
      0);

void RunTest(const TestVector &test) {
    std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
    CExtKey key;
    CExtPubKey pubkey;
    key.SetMaster(seed.data(), seed.size());
    pubkey = key.Neutered();
    for (const TestDerivation &derive : test.vDerive) {
        unsigned char data[74];
        key.Encode(data);
        pubkey.Encode(data);

        // Test private key
        CBitcoinExtKey b58key; b58key.SetKey(key);

        BOOST_CHECK(b58key.ToString() == derive.prv);

        CBitcoinExtKey b58keyDecodeCheck(derive.prv);
        CExtKey checkKey = b58keyDecodeCheck.GetKey();

        assert(checkKey == key); //ensure a base58 decoded key also matches

        // Test public key
        CBitcoinExtPubKey b58pubkey; b58pubkey.SetKey(pubkey);
        BOOST_CHECK(b58pubkey.ToString() == derive.pub);

        CBitcoinExtPubKey b58PubkeyDecodeCheck(derive.pub);
        CExtPubKey checkPubKey = b58PubkeyDecodeCheck.GetKey();
        assert(checkPubKey == pubkey); //ensure a base58 decoded pubkey also matches

        // Derive new keys
        CExtKey keyNew;
        BOOST_CHECK(key.Derive(keyNew, derive.nChild));
        CExtPubKey pubkeyNew = keyNew.Neutered();
        if (!(derive.nChild & 0x80000000)) {
            // Compare with public derivation
            CExtPubKey pubkeyNew2;
            BOOST_CHECK(pubkey.Derive(pubkeyNew2, derive.nChild));
            BOOST_CHECK(pubkeyNew == pubkeyNew2);
        }
        key = keyNew;
        pubkey = pubkeyNew;

        CDataStream ssPub(SER_DISK, CLIENT_VERSION);
        ssPub << pubkeyNew;
        BOOST_CHECK(ssPub.size() == 75);

        CDataStream ssPriv(SER_DISK, CLIENT_VERSION);
        ssPriv << keyNew;
        BOOST_CHECK(ssPriv.size() == 74);

        CExtPubKey pubCheck;
        CExtKey privCheck;
        ssPub >> pubCheck;
        ssPriv >> privCheck;

        BOOST_CHECK(pubCheck == pubkeyNew);
        BOOST_CHECK(privCheck == keyNew);
    }
}

BOOST_FIXTURE_TEST_SUITE(bip32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bip32_test1) {
    RunTest(test1);
}

BOOST_AUTO_TEST_CASE(bip32_test2) {
    RunTest(test2);
}

BOOST_AUTO_TEST_CASE(bip32_test3) {
    RunTest(test3);
}

BOOST_AUTO_TEST_SUITE_END()
