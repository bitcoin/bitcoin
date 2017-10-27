// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key/stealth.h"
#include "key.h"
#include "pubkey.h"
#include "keystore.h"
#include "base58.h"
#include "serialize.h"
#include "streams.h"

#include "test/test_particl.h"

#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/atomic.hpp>

// test_particl --log_level=all  --run_test=stealth_tests

BOOST_FIXTURE_TEST_SUITE(stealth_tests, BasicTestingSetup)

static const std::string strSecret1C("GzFRfngjf5aHMuAzWDZWzJ8eYqMzp29MmkCp6NgzkXFibrh45tTc");
static const std::string strSecret2C("H5hDgLvFjLcZG9jyxkUTJ28P6N5T7iMBQ79boMuaPafxXuy8hb9n");
static const std::string strSecret3C("Gyi9eQQC7qYRqwgAs1K1CdTEr9U7XR1RvDQdYjpK2dDU92N2cutY");

BOOST_AUTO_TEST_CASE(stealth_key_1)
{
    const char *testAddr = "SPGx5uFievEkR6gCz9FAoWYWADGPZAddbsQeXLtMggpDGkvPAWC5qiSNkYbG6H8obrDJkxvp8FzCRZkimapA22Nrjpw7EofyoNSe4x";

    CStealthAddress sxAddr;

    BOOST_CHECK(true == sxAddr.SetEncoded(testAddr));

    BOOST_CHECK(HexStr(sxAddr.scan_pubkey.begin(), sxAddr.scan_pubkey.end()) == "029b62c32a5561946b1b43cce1235a3b47d82abde25807cb9df2a65a1941558a8d");
    BOOST_CHECK(HexStr(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end()) == "02f0e2f682c8a07fdba7a3a97f823261008c7f53156c311d20216af0b6cc8148c3");

    BOOST_CHECK(sxAddr.Encoded() == testAddr);
    BOOST_MESSAGE(sxAddr.Encoded());
};

BOOST_AUTO_TEST_CASE(stealth_key_serialise)
{
    CStealthAddress sxAddr;

    CBitcoinSecret bsecret1, bsecret2;
    BOOST_CHECK(bsecret1.SetString(strSecret1C));
    BOOST_CHECK(bsecret2.SetString(strSecret2C));

    sxAddr.scan_secret = bsecret1.GetKey();
    CKey spend_secret = bsecret2.GetKey();

    sxAddr.spend_secret_id = spend_secret.GetPubKey().GetID();


    CPubKey pkTemp = sxAddr.scan_secret.GetPubKey();
    BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);

    sxAddr.scan_pubkey.resize(EC_COMPRESSED_SIZE);
    memcpy(&sxAddr.scan_pubkey[0], pkTemp.begin(), EC_COMPRESSED_SIZE);

    pkTemp = spend_secret.GetPubKey();
    BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);
    sxAddr.spend_pubkey.resize(EC_COMPRESSED_SIZE);
    memcpy(&sxAddr.spend_pubkey[0], pkTemp.begin(), EC_COMPRESSED_SIZE);

    const char *testAddr = "SPGyji8uZFip6H15GUfj6bsutRVLsCyBFL3P7k7T7MUDRaYU8GfwUHpfxonLFAvAwr2RkigyGfTgWMfzLAAP8KMRHq7RE8cwpEEekH";
    BOOST_CHECK(sxAddr.Encoded() == testAddr);

    sxAddr.prefix.number_bits = 8;
    sxAddr.prefix.bitfield = 0xaaaaaaaa;

    const char *testAddr2 = "2w3KXRWZtyMyYrHdKqK7voiwiRy66gbpxEF3WcmHUF3M7qV3tj6AsRTFQyKV5wYmgu5PJFy1RN8kmNaF63ZSQfJ18DHMKvxdj1DpCmmQ";
    BOOST_CHECK(sxAddr.Encoded() == testAddr2);

    CDataStream ss(SER_DISK, 0);

    ss << sxAddr;
    CStealthAddress sxAddr2;
    ss >> sxAddr2;

    BOOST_CHECK(sxAddr2.Encoded() == testAddr2);
    BOOST_CHECK(sxAddr2.prefix.number_bits = 8);
    BOOST_CHECK(sxAddr2.prefix.bitfield == 0xaaaaaaaa);

    BOOST_CHECK(sxAddr2.scan_secret.IsValid());
    BOOST_CHECK(sxAddr.scan_secret == sxAddr2.scan_secret);

    BOOST_CHECK(sxAddr.spend_secret_id == sxAddr2.spend_secret_id);


    ss.clear();
    BOOST_CHECK(ss.empty());

    // Add 2nd spend pubkey
    sxAddr.number_signatures = 1;

    sxAddr.spend_pubkey.resize(EC_COMPRESSED_SIZE * 2);

    CBitcoinSecret bsecret3;
    BOOST_CHECK(bsecret3.SetString(strSecret3C));
    CKey kSpend2 = bsecret3.GetKey();

    ec_point pkSpend2;
    BOOST_CHECK(SecretToPublicKey(kSpend2, pkSpend2) == 0);

    memcpy(&sxAddr.spend_pubkey[EC_COMPRESSED_SIZE], &pkSpend2[0], EC_COMPRESSED_SIZE);

    const char *testAddr3 = "3XuQSjM7p8o7qg7jrk7UmXXpHVv6tdPodYvEZKeSx1ENYTE51dL5Jj4GwMFdare6uL3znPj7jVN6Bo9jqnvRwyywuur8GcijEcjYcbaLteR2eG47eqRNACfRtRzYJRbTmZCPCxqELsrcxznaossEi";
    BOOST_CHECK(sxAddr.Encoded() == testAddr3);

    ss << sxAddr;
    ss >> sxAddr2;
    BOOST_CHECK(sxAddr2.number_signatures == 1);
    BOOST_CHECK(sxAddr2.spend_pubkey.size() == EC_COMPRESSED_SIZE * 2);

    // verify pkSpend 2
    BOOST_CHECK(memcmp(&pkSpend2[0], &sxAddr.spend_pubkey[EC_COMPRESSED_SIZE], EC_COMPRESSED_SIZE) == 0);

    // verify pkSpend 1
    BOOST_CHECK(sxAddr.spend_pubkey == sxAddr2.spend_pubkey);
}


void makeNewStealthKey(CStealthAddress &sxAddr, CBasicKeyStore &keystore)
{
    InsecureNewKey(sxAddr.scan_secret, true);

    CKey spend_secret;
    InsecureNewKey(spend_secret, true);
    //sxAddr.spend_secret_id = spend_secret.GetPubKey().GetID();

    SecretToPublicKey(sxAddr.scan_secret, sxAddr.scan_pubkey);
    SecretToPublicKey(spend_secret, sxAddr.spend_pubkey);

    // verify
    CPubKey pkTemp = sxAddr.scan_secret.GetPubKey();
    BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);
    BOOST_CHECK(memcmp(&sxAddr.scan_pubkey[0], pkTemp.begin(), EC_COMPRESSED_SIZE) == 0);

    pkTemp = spend_secret.GetPubKey();
    BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);
    BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);
    BOOST_CHECK(memcmp(&sxAddr.spend_pubkey[0], pkTemp.begin(), EC_COMPRESSED_SIZE) == 0);

    keystore.AddKeyPubKey(spend_secret, pkTemp);
}

BOOST_AUTO_TEST_CASE(stealth_key_address)
{
    SeedInsecureRand();
    CBasicKeyStore keystore;
    ECC_Start_Stealth();

    for (size_t k = 0; k < 32; ++k)
    {
        CStealthAddress sxAddr;
        makeNewStealthKey(sxAddr, keystore);
        sxAddr.prefix.number_bits = k;
        sxAddr.prefix.bitfield = 0xaaaaaaaa;
        BOOST_MESSAGE(sxAddr.Encoded());

        CBitcoinAddress addrC(sxAddr.Encoded());
        BOOST_CHECK(addrC.IsValid() == true);

        BOOST_CHECK(addrC.ToString() == sxAddr.Encoded());


        CTxDestination dest = addrC.Get();

        BOOST_CHECK(dest.type() == typeid(CStealthAddress));
        CStealthAddress sxAddrOut = boost::get<CStealthAddress>(dest);

        BOOST_CHECK(sxAddrOut == sxAddr);
        BOOST_CHECK(sxAddrOut.Encoded() == sxAddr.Encoded());

        CBitcoinAddress addrC2(dest);
        BOOST_CHECK(addrC.ToString() == addrC2.ToString());

    };

    ECC_Stop_Stealth();
}

BOOST_AUTO_TEST_CASE(stealth_key)
{
    SeedInsecureRand();
    CBasicKeyStore keystore;

    ECC_Start_Stealth();

    for (size_t i = 0; i < 16; ++i)
    {
        CStealthAddress sxAddr;

        makeNewStealthKey(sxAddr, keystore);

        CKey sEphem;
        CKey secretShared;
        ec_point pkSendTo;

        // Send, secret = ephem_secret, pubkey = scan_pubkey
        // NOTE: StealthSecret can fail if hash is out of range, retry with new ephemeral key
        int k, nTries = 24;
        for (k = 0; k < nTries; ++k)
        {
            InsecureNewKey(sEphem, true);
            if (StealthSecret(sEphem, sxAddr.scan_pubkey, sxAddr.spend_pubkey, secretShared, pkSendTo) == 0)
                break;
        };
        BOOST_CHECK_MESSAGE(k < nTries, "StealthSecret failed.");
        BOOST_CHECK(pkSendTo.size() == EC_COMPRESSED_SIZE);


        ec_point ephem_pubkey;
        CPubKey pkTemp = sEphem.GetPubKey();
        BOOST_CHECK(pkTemp.size() == EC_COMPRESSED_SIZE);
        ephem_pubkey.resize(EC_COMPRESSED_SIZE);
        memcpy(&ephem_pubkey[0], pkTemp.begin(), EC_COMPRESSED_SIZE);

        CKey secretShared_verify;
        ec_point pkSendTo_verify;

        // Receive, secret = scan_secret, pubkey = ephem_pubkey
        BOOST_CHECK(StealthSecret(sxAddr.scan_secret, ephem_pubkey, sxAddr.spend_pubkey, secretShared_verify, pkSendTo_verify) == 0);

        BOOST_CHECK(pkSendTo == pkSendTo_verify);
        BOOST_CHECK(secretShared == secretShared_verify);

        CKeyID iSpend = sxAddr.GetSpendKeyID();
        CKey kSpend;
        BOOST_CHECK(keystore.GetKey(iSpend, kSpend));
        CKey kSpendOut;
        BOOST_CHECK(StealthSharedToSecretSpend(secretShared_verify, kSpend, kSpendOut) == 0);
        pkTemp = kSpendOut.GetPubKey();
        BOOST_CHECK(pkSendTo == pkTemp);

        CKey kSpendOut_test2;
        BOOST_CHECK(StealthSecretSpend(sxAddr.scan_secret, ephem_pubkey, kSpend, kSpendOut_test2) == 0);
        pkTemp = kSpendOut_test2.GetPubKey();
        BOOST_CHECK(pkSendTo == pkTemp);
    };

    ECC_Stop_Stealth();
}

BOOST_AUTO_TEST_SUITE_END()
