// Copyright (c) 2013-2015 The Bitcoin Core developers
// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "base58.h"
#include "key.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_iop.h"

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

    TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

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
    ("9PPHAFDqaktTVmXV5XVM1VzQFxgPHHdjRtsW6hM5pDUigMu8PoV27QmPgqyiFgckwv8vEzPRj261iGCfxbpKbdJxcZXBVmfRvYuvr2dwq2VgUYm3",
     "dywPw75G43VTMBbkZg4KynAZezAUnC5BTqnn93h2PRn36JZmMD95uyAHEFjqybT89FnEs39dUnGWY1GjEiNXz548BZURTVP2YGqrZH4d36wk7BAt",
     0x80000000)
    ("9PPHAFG6zkbkxrnc4g6UihkGtqiVPcTHnGwz7NxWiBa6oL6KgAmKHpzvmy84fTtKefSpmzbiHtKZ8zqk3aDHjaLgzdQeDyJEBUDq4Ya5PL3ZMKxA",
     "dywPw77XU3CkpGrsYpfTgyvSHsCatWtjpDsG9jJTHPsRDGkxdaRP6PPpKNtCPNiUUfVcpkCXB9ZqQPda7S8ZhieQNMYVyZFhgWoZSz5FtXzvb7TD",
     1)
    ("9PPHAFJH7xPJrFVV8WYXAS1AmSTP3fqGtGyiQKHBYmqscGj7tQJpRYoM5vgxtsxnE5PY7xKLLfk1F7SEXAX2Z3frtK4UAGW52wrqLBJcAd2kCUFa",
     "dywPw79hbEzJhfZkcf7W8iBLATwUYaGivDtzSfd87z9C2DPkqoxtE7CEdLT6cnmfYGRPsCHGrxPRqULXXa55vqzCM8hzR52fQaZLJesaG4zkbLkr",
     0x80000002)
    ("9PPHAFLtPzv8hxNLYNiKFgba8oTRBtz9QYXJPepRE2BAmSbVKpA1YMZBWq8WpRyy4XFySJqS5H7yrVNoVcrAqkmQRNWczrQ3B2ou1tzXJiEyNRqq",
     "dywPw7CJsHX8ZNSc2XHJDxmjXpwWgoRbSVSaS1AMoEUVBPG8HDp5Lux54EteYLotAEeuMhkJFYrNktkMRoGpgS17YFoS5ReDxjVaFG9KUhayQq2d",
     2)
    ("9PPHAFP7nqMFf8r1XTBnyNrnDgxaVyoS9B7cb6JJXH8hEm33wBFduRKst2hyk7JxSw48An9ZFR8ToY7hcBGWqr4qd676FmBbMrznqugS9rrxoLxB",
     "dywPw7EYG7xFWYvH1bkmwf2wciSfztEtB82tdSeF6VS1ehhgtauhhyimRSU7U28HchKJnmYRxAmcKoZobGTj8dRetTC27ptK4HaPxR7neD5HswBs",
     1000000000)
    ("9PPHAFQqZK2ruFyPiyXaRVQLwzfaVn31SzcRctYh5XR5Ny7SRtP3Hwk8YHp8ok8sPnKCtFqqnvUPc5KQCmgurwvLTwNFNdj1SfUNUie45775hZ2T",
     "dywPw7GG2bdrkg3fD86ZPmaWM29fzgUTUwXhfEtdejiPnun5PJ376W925haGXez4pcERsNwqCx8bGecLiGdjWAVfca9ffAP5HQV9fhTLzxDQaeCV",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("9PPHAFDqaktTVmXV5X6rLc9UYQt73XpsraVkvNvsYWRTWLHFG6U3JavAdGssr2iqvdPHCtwfWvkfsfHBDKc3pkVsmhSBSdbdPPQRWM1cwJLut1fv",
     "dywPw75G43VTMBbkZffqJtKdwSNCYSGKtXR2xjGp7iimvGwtDW8779K4Age1ZwWuCSHnxHCNuXYxzHNmRouG67UWtAqkDwo6nRBmHrKFGTqxtxPT",
     0)
    ("9PPHAFH7L2c9TFYKA3NSdjvrRUgWZtnd7HUFCw6cugEUxQh7DnSVHABWhRiT6cBopRUHe7G9gnyfd4N2BUmUWdeaC32qiueN4HkZtUVjbjaRonjr",
     "dywPw78XoKD9JfcaeBwRc271pWAc4oE59EPXFHSZUtXoNMMkBC6Z5iaQEqUapX2B15MdngrcjRZNiiYBZAQtufm1w74ucwyWqofw2yj9pq6N515H",
     0xFFFFFFFF)
    ("9PPHAFJGPHDAydSVGDEepANr4x2C8HXr1pxQu2FhSbdPj4RscEjijn4w1dnEhDa3wBCtjtn5ESpXEMaSn9SLrQZ4JmBJMSiz7eUpuLt1CaMfHqHA",
     "dywPw79grZpAq3WkkModnSZ1TyWHdByJ3msgwNbe1ovi916WZePnYLTpZ3YNR8NeyocNGb5wdCaaMmYgfbedgQ8d4gNMBNhkBaBfHmWVMbaNsVgY",
     1)
    ("9PPHAFM5MhE9uo5nTJrKxE2zduy8Md4iWw9LALhCJSh44NKE3uZau5aTiXgd54NF4y5b2UxXnHTcTzhkcEEbsKTVG2WmG31Bxs2G7Geg9ppHygK3",
     "dywPw7CVpyq9mDA3wTRJvWDA2wTDrXWAYt4cCh38sezNUJys1KDehdyMFwSknyArbqKZxu3JKwsekB6MsvMQqT29SgrAUWUGzRHdgFJv65Dxausv",
     0xFFFFFFFE)
    ("9PPHAFNFPcC6GQUkAboZYxWpBHUpYXoBsz7xfNPvByQkUD5VJMuuqBFPj4CzAh2NaAT7USQBR5xgB3K5TssmG1LbmjsTiJuWTRKMX8skjMmoJwZd",
     "dywPw7Dfrto67pZ1ekNYXEgyaJxv3SEduw3EhijrmBi4t9k8FmZydjeHGTy7tbsazTjxFScb2FhRjZTwTjUWCxy3wNgqeLQpJZVfJatvc8y5RmyW",
     2)
    ("9PPHAFPcRZdJmvf4QM4kSgawsGECWvrizESsk2ZY5PC6xqsLtS7FQLtKVEmwAmRSrJ2UAxTPeMtBaw26TNJpkqSHg48KHx5LYMgeFpAwv2QGE1jM",
     "dywPw7F2trEJdLjKtVdjQxm7GHiJ1qJB2BN9nNuUebVRNnXyqqmKCuHD2eY4tgHFzcm3KUvSkYT4Z1Zn9n2PihHRARfaN9GKocuo2HGGADc4k7Ut",
     0);

TestVector test3 =
  TestVector("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be")
    ("9PPHAFDqaktTVmXV5WAJW8XyLcsN5QUCxLMvC6Ge6uiV4byL44iGaF4QbE97NWiNuSyrr52RNkkYt3x14kugjbCvD7mqakqXc7xsM8j8dpptxY79",
     "dywPw75G43VTMBbkZejHUQi8jeMTaJuezHHCEScag81oUYdy1UNLNoTJ8duF6RWdNPK2HMu3f6Uf6BoW9AYsQLufJxKFUU5fRcd7G92ud7jpL3qF",
      0x80000000)
    ("9PPHAFGCnVrHdStrRhhPuMgJqg87k6ppLVDL6Vkt3ixAuYHFRKjQ2pZBTJbu5pSDRBLTbGPvDXiJctuCZPNd5XSrPh27XRwUVfpN9cuVYVDc5im6",
     "dywPw77dFnTHUry7urGNsdrUEhcDF1GGNS8c8r6pcwFVKUwtNjPTqNx4ziN2ojGyijGMCp3h57RSxqY8bp6WYv8F4i5T6uDaUMnP5G3sH5BEUkrD",
      0);

void RunTest(const TestVector &test) {
    std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
    CExtKey key;
    CExtPubKey pubkey;
    key.SetMaster(&seed[0], seed.size());
    pubkey = key.Neuter();
    for (const TestDerivation &derive : test.vDerive) {
        unsigned char data[74];
        key.Encode(data);
        pubkey.Encode(data);

        // Test private key
        CIoPExtKey b58key; b58key.SetKey(key);
        // std::cout << b58key.ToString() << '\n';
        // std::cout << derive.prv << '\n';
        
        BOOST_CHECK(b58key.ToString() == derive.prv);

        CIoPExtKey b58keyDecodeCheck(derive.prv);
        CExtKey checkKey = b58keyDecodeCheck.GetKey();
        assert(checkKey == key); //ensure a base58 decoded key also matches

        // Test public key
        CIoPExtPubKey b58pubkey; b58pubkey.SetKey(pubkey);
        BOOST_CHECK(b58pubkey.ToString() == derive.pub);

        CIoPExtPubKey b58PubkeyDecodeCheck(derive.pub);
        CExtPubKey checkPubKey = b58PubkeyDecodeCheck.GetKey();
        assert(checkPubKey == pubkey); //ensure a base58 decoded pubkey also matches

        // Derive new keys
        CExtKey keyNew;
        BOOST_CHECK(key.Derive(keyNew, derive.nChild));
        CExtPubKey pubkeyNew = keyNew.Neuter();
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
        BOOST_CHECK(ssPriv.size() == 75);

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
