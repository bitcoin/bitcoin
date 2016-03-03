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
    ("drkvjJe5sJgqomjLo3qGVgD8fcPo43UZPgaaskFEjpgSzpwfyEji7NK1mrdUUC1rtqdST6MW4eFxxt1328RR312x9XZsZUAxNo7pTssukwBT5Tx",
     "drkpRsmwKURS9g8uDqs1tGTJwh16JHZ27Q49iCXHQ2rD7fFVix4LRp47wHSYAFD8cPPSNoonYypuVXbPig9vkyTtxX3CoeF6B4KB9ozvafgNEj8",
     0x80000000)
    ("drkvjLuVs1zJu2rKwexyhS5mYeVuNs2umm4bZMg8hv4Zy28xLX2tXbr6tzytFTaZe9XyTJe4vsoPhX57zXPYz2mLDR2bm6yDJ722yp1U4adNnEx",
     "drkpRv3MKBiuEwFtNSzj62Kwpj7Cd77NVUYAPoxBN8EL5rSn6EMWr3bD4RnwwWZU26mQ5rhUvH9mstSGSSBeQZk5kb7isWvER224rpdn1q7ivwn",
     1)
    ("drkvjP5d4oYCHjjPn71RRgye9PPZSF21mnntVgLyJBqNuewAa4Y2FQGQrZt7fY3946FKR2G7iJFVp7Zbaq8NTMwDu4rY4Jp4mk2JcYYFMYERRv6",
     "drkpRxDUWyGnde8xCu3ApHDpRTzrgV6UVWGTL8d1xQ192VEzKmreZr1X1zhBMZkXd2YSXwTAj6kCxbPgaNhsXuY4XkcAPHsxUmnvXcx9YgAKCuZ",
     0x80000002)
    ("drkvjRgu7LN3zcaoeGoWgHP1WPRhfPtY3LNsqDaeYX8Y5XJbyuj94A6qm1S3DZDyVxgdmYMrKgE7C48a3AGfATUkxX1NeCnCrh5zLETPSpexfvd",
     "drkpRzpkZW6eLWzN54qG4sdBnU2zudxzm3rSffrhCjJJCMcRjd3mNbqwvSF6uby9bG3w3QUZKZh8P1DaoaSd7vTFer3pjuSWdi2s8thNBM9vCdA",
     2)
    ("drkvjTvHwmV1B6FnikHENYb6Ptb1kDBGfvh5GhTwoUf1PxsDM1MW7voCxatxttDMukqNErV2Tgi4Eo2gbacfFkuxg7UdYzLPgsypLvNEbN6fekp",
     "drkpS349PwDbWzfM9YJym8qGfyCJzTFjPeAe79jzTgpmWoB36ig8SNYK81i2avNc3vTN7CcFwUvhHpfkGmM5KLzbrEds99XcBnraHsAXgmc3EUv",
     1000000000)
    ("drkvjVe4RT6FJDdzF64gV69phbb1YSkaVRW74wrW3m39c3Fi48kteM3sDh42Xi8Jm1v5iYmZy2drmzjHC11gMcQoXNdkRXkUVMZT9sz9qZGZMZH",
     "drkpS4muscpqe83Yft6RsgPzygCJngq3D8yfuQ8YhyCuisZXor5WxnnyP7s6Dn9oxqaSic1Wique8sCsGwMSrR1KyCHQUeHqJhcHaCitRvHHSKf",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("drkvjJe5sJgqomjLnfLbbqHR7p7ZJEcz5JqQZL2y2mRGyD4YGDkuHX5xCko4pJ6qc5zQMebHyJv8MxWHjv9eABx7HSZpRQNRDHcUnFZ22sRq6So",
     "drkpRsmwKURS9g8uDTNLzRXbPtirYUhSo2JyPnK1gyb363NN1w5Xbxq4NBc8WJzBntwXcrZDJGHMmddapCt2oPrbZtMyG4KLKQDuj4d9wbaoren",
     0)
    ("drkvjMuq92NoHnZRJvvtjcfJBcX5fCNEnHKh7VnLCaSj3cvVxCCsrnS2MbNKPm4jQAzqZy5TqXusm3MFu5aL3LeXd3E6hT767dkrujfgU3isKDC",
     "drkpRw3gbC7Pdgxyjixe8CuUTh8NuSShVzoFwx4NrncVATEKhuXWBEB8X2BP5pFzRxnN2Wo3CGh6Co3iAiWrMgMeW7XNGEjPhtPerUXiJrUD8m1",
     0xFFFFFFFF)
    ("drkvjP4tPdQKfgjXUo95A4ewexCe3wb9KmVPCers7yMVhMgtQVSLUfrLZf9v19Jr9ubwMV11VNmV4FmrZkSfpF8eMBgjEXj9UN1sn7wHJwWQJmy",
     "drkpRxCjqo8v1b95ubApYeu7w2owJBfc3Uxx378unBXFpBziACkxo7bSj5xyhAjyADWqvk7vyHtjFoYpbxFd63xn5QxvgxxjUQ7ueFsF5LRhHsN",
     1)
    ("drkvjRsroePFqL2iaQpDDioWcu8sPUTeRxQeX6Miy31q1F3L5KJVnBP3TZYHqwVywnJDwfTZL1rihP5geYhgj9ZbcX9dpovzguT5htcEZKiw6Mb",
     "drkpS11iFp7rBESH1CqxcK3gtykAdiY79ftDMYdmdFBb85M9q2d86d89czMMXxwbBviYEhUdiay7fME2vf2n8wVA5tnDpjVYKW6J84HyYyycqPW",
     0xFFFFFFFE)
    ("drkvjT3ticKcSizRsN3oxCd3zQq4JCw1Uw39Yo5cVkiEr1JaXfdRsrK3z5uPUbdV99pfu77C8WvRjzQYJBs5R2g7Ksr66iFVFCYVa7gp6JUy6u3",
     "drkpS2BkAn4CndPzJA5ZLnsEGVSMYT1UCeWiPFMf9xszxqcQHNx4CJ4A9WiTAffypM6pnGmL2Qk73iocjn89etPemjTPeg2rTi7vTeJVcfjAABd",
     2)
    ("drkvjUQvg3Y7xuJfcdEhgGkjyAD2hGU7jFxECxhVuY4jUoAAbry13VEpAerPYzhmGjBNRAKRQSRqdhRXncvaF8N1e8hfjt5aBZqEFQszktZUYXN",
     "drkpS3Yn8DGiJoiE3RGT4rzvFEpKwWYaSyRo3QyYZkEVbdTzMaHdMvyvL5fTF5LyyNBtpad4KANvVpeJnL1fPCkspiC7TXYMX8FeA1e3hMDsE8f",
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
