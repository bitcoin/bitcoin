// Copyright (c) 2013-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "base58.h"
#include "key.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_dash.h"

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
    ("dpubZ9169KDAEUnyoBhjjmT2VaEodr6pUTDoqCEAeqgbfr2JfkB88BbK77jbTYbcYXb2FVz7DKBdW4P618yd51MwF8DjKVopSbS7Lkgi6Y7Ckb8",
     "dprvLJcNFzfZn6ywEjid4Ver3tsK3puSZFZr3oxCtC1cPdxVsZzjQwGPHvpLxZ83zP37V35uVQNG7GYHjxtkEUWhUKrFTt1bEj49EvPzRfWNPQH",
     0x80000000)
    ("dpubZBGW92WdKjuxwnqSwXKfNcLuxffArXhpWof4cw4ie3Db32UJYR8QEG61EpAKHqVZFiGg5Yj4EhTAyXwm236KK1gTX8c5MuLKrgpGQGTaNAV",
     "dprvLLsnFhy2sN6vPLrLGFXUvvyRNeTnwL3rjRP6rHPjMq9nErHuqAoUR5AkjpUPQ6R5C5ybrhh8VdPATivTt4ntGPvmXkgjUgm2ww2qroEB9Ts",
     1)
    ("dpubZDSdLp4WiSo2nEstfnDXyMEa23eGrZS7T8KuDCqXag1oGZySGDYiBpzEetcthnCuDRtiryBAMHweZqgaVNGCzfWPpLSvqYLbVRM3h9fkKv6",
     "dprvLP3uTVWvG4yzDntmzWRMXfs5S2StwMn9fk3wSZAYJTwzUPo3YyDnNe4z9sfT12C7eAjHfXHZaLLabfSh1Qas3ZRD3XeTYSXtcjMDPeReKGg",
     0x80000002)
    ("dpubZG3uPLtNRKeSeQfyvNcuLMGiFCWo8726nfZaTY8gkYPEgRAZ4yP96GYACuoj9eeDZwzTUM9mjEWd2ApsCTok47fEQER4vVQHD7GBnSUChBT",
     "dprvLRfBW2LmxwqQ5xgsF6pitfuDfBKRCuN91HHcgtThULKRtEzAMj4DH5cuhut4yFhc9dkgFzEUzkAUpsBSbRW4AersQ9D1hNmqE16S2LQbbCi",
     2)
    ("dpubZJHJDn1KboKRit9hcdpzDrS2L1oXkhLJE9SsiVfA4ywr3Wnv8k5WHr15tEo7ZSnx3G7dcMdimyQjabAsHmEwmi8VK1yFkgJ7DoB2vzheMZ7",
     "dprvLTtaLTTj9RWPASAawN2onB4Xjzc9qVgLSmAuwqzAnmt3FLcXRVkaUf5qPEHXRv73DRtNsuU3uZceJ45tnr3QN3SuoPJ7FTbYNyZbXkHHL6B",
     1000000000)
    ("dpubZL14hTcZivhdFDw9jBPiXZS28FNqaC9L2PqRxn3JH4LLkeCJfALAYxA9X4i4QhsfWxQB7hZXKB7LB1ZtPcjncyHcBZPLZ9sk2knxB9dPD6v",
     "dprvLVcLp94yGYtagmx33ubY5t4XYEBTezVNF1ZUC8NJzrGXxU1uwv1EjmEu264jLqE7pqHdfGSzkc9mJE6GKv48V16T8t4LNNMFfK7xGuw1qYe",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("dpubZ9169KDAEUnynoD4qvXJwmxZt3FFA5UdWn1twnRReE9AxjCKJLNFY1uBoegbFmwzA4Du7yqnu8tLivhrCCH6P3DgBS1HH5vmf8MpNZPvMv6",
     "dprvLJcNFzfZn6ywEMDxAej8W6b5J23sEspfjPjwB8kSN25NAZ1vb63KipywJcu7AtbCj5qLEgpiPNaUqVcrGtuQ5hB1vJ5qP4xspB2Dndk4mqU",
     0)
    ("dpubZCGqR2u7iVd4K4oMyhuC1aN6EzzVs3xv4wmG7bSsie18eheHsbiKgrUSP7eV3rxRNNi4zCqYJDjJt68Y5LyWidsxTUjxBS59ncUUoiV7Hou",
     "dprvLMt7XiMXG7p1kcpFJS71Ztzbeyo7wrJxHZVJLwmtSRwKrXTuAMPPsfZBt8AuoxS38k5A8hESpXzcC1FfqBQT1vLQvUVtmZ8cwavn9q2tk8V",
     0xFFFFFFFF)
    ("dpubZDRtfdve6PoAUw1YQ9tqUv3edkDQQY8cA6qo2zMeNNmX6zskVV8dtvG2zVtbobZX9tdce3h9bS9uYkzsrFTdSnLazZN1YALAezk5eZACqNH",
     "dprvLP3AnKP3e1z7vV2Rit6f3EgA3j22VLUeNiZqGLgf6AhiJphMnEoi5jLnVUetYDAX2yQ3uiS5sYVidEzSZZ1abDmyMCjEY4rsjNGJvJWqMTT",
     1)
    ("dpubZGEs5euaG36MaYggTp3QSrysyH5uWj3sUYLet41ygG7xmpjunzfLnpeQqJ5jbUFok56AUgnPEZTjdZFtm9tai7oVaqZrkhmNamR2u21VLF6",
     "dprvLRr9CLMyofHK26hZnYFE1BcPPFtXbXPuhA4h7QLzQ449yeZX5kLQydjALGrWZvNDLvkkf1WUH6AvwwmbcSXxbhbGUyG3PAqGDAh3PuPivHi",
     0xFFFFFFFE)
    ("dpubZHQtzcqvsS44sVvHCHrwpNg4t1ZGZhgNWF4YQmiPX2PDEB4qtfbMKM1WTxDEnqnFhWjoHBr6HAnbHCRHT316RUVwrjtMJzrnSzVcRwYawkr",
     "dprvLT2B7JJLR4F2K3wAX24mNhJaHzMteW2Qirnae83QEpKQRztTBRGRWA6FxyauCLkVtW3SxqHTfTkWm4ry8PSTHYGSJuoMXNrtYkhZTfLdWhK",
     2)
    ("dpubZJmvx44SPcNJcm7AvMzdo843H56Np2bTAQgRpZ4t9pEoJNQR4JX7VuxWYMHWvR8xDZx2Z7MWAsoamdUnH8gzjjMXVuiSFN9X8Hgo6Zfpw3T",
     "dprvLUPD4jWqwEZG4K84F6CTMSgYh3tztpwVP2QU3uPtscAzWCE2M4CBgj3G3PFuMMqZvouBFavH7ZbCockUrhogLX1A7mJranzcF837YG1usdU",
     0);

void RunTest(const TestVector &test) {
    std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
    CExtKey key;
    CExtPubKey pubkey;
    key.SetMaster(&seed[0], seed.size());
    pubkey = key.Neuter();
    BOOST_FOREACH(const TestDerivation &derive, test.vDerive) {
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
        CExtPubKey pubkeyNew = keyNew.Neuter();
        if (!(derive.nChild & 0x80000000)) {
            // Compare with public derivation
            CExtPubKey pubkeyNew2;
            BOOST_CHECK(pubkey.Derive(pubkeyNew2, derive.nChild));
            BOOST_CHECK(pubkeyNew == pubkeyNew2);
        }
        key = keyNew;
        pubkey = pubkeyNew;
    }
}

BOOST_FIXTURE_TEST_SUITE(bip32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bip32_test1) {
    RunTest(test1);
}

BOOST_AUTO_TEST_CASE(bip32_test2) {
    RunTest(test2);
}

BOOST_AUTO_TEST_SUITE_END()
