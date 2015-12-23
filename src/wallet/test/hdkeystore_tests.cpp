// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/wallet.h"
#include "base58.h"

#include <set>
#include <stdint.h>
#include <utility>
#include <vector>

#include "test/test_bitcoin.h"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

extern CWallet* pwalletMain;

BOOST_FIXTURE_TEST_SUITE(hdkeystore_tests, TestingSetup)


BOOST_AUTO_TEST_CASE(hdkeystore_tests)
{
    LOCK(pwalletMain->cs_wallet);

    // create a master key
    CHDChain chain;
    chain.keypathTemplate = "m/c'";
    std::vector<unsigned char> vSeed = ParseHex("9886e45b8435b488a4cb753121db41a07f66a6a73e0a705ce24cee3a3bce87db");
    CKeyingMaterial seed = CKeyingMaterial(32);
    seed.assign(vSeed.front(), vSeed.back());

    CExtKey masterKey;
    masterKey.SetMaster(&seed[0], seed.size());
    CBitcoinExtKey masterXPriv;
    masterXPriv.SetKey(masterKey);
    BOOST_CHECK(masterXPriv.ToString() == "xprv9s21ZrQH143K3p7CoBzQ9XPGDfaK8YHfuy11V3tGCG715SX1FYhZRP4rqCDKryZDiFtcvfr9A9aQCSioUTScA6reJktbLqEW6soRZfyZqU9");

    // set the chain hash
    CExtPubKey masterPubKey = masterKey.Neuter();
    chain.chainID = masterPubKey.pubkey.GetHash();

    // store the chain
    pwalletMain->AddHDChain(chain);
    pwalletMain->AddMasterSeed(chain.chainID, seed);

    // create some keys
    CKey newKey;
    std::string keypath;
    pwalletMain->DeriveKeyAtIndex(chain.chainID, newKey, keypath, 0, false);
    // we should still be at index0 (key was not added to the wallet)
    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, false) == 0);

    CKeyMetadata metaData(GetTime());
    metaData.keypath = keypath;
    metaData.chainID = chain.chainID;
    pwalletMain->mapKeyMetadata[newKey.GetPubKey().GetID()] = metaData;
    pwalletMain->AddKeyPubKey(newKey, newKey.GetPubKey());
    // now the key was added, next index should be 1
    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, false) == 1);
    BOOST_CHECK(CBitcoinAddress(newKey.GetPubKey().GetID()).ToString() == "1AFW8Aq7jXmtqLHjicMtov56FRBeYqSHj7");

    pwalletMain->DeriveKeyAtIndex(chain.chainID, newKey, keypath, pwalletMain->GetNextChildIndex(chain.chainID, false), false);
    metaData = CKeyMetadata(GetTime());
    metaData.keypath = keypath;
    metaData.chainID = chain.chainID;
    pwalletMain->mapKeyMetadata[newKey.GetPubKey().GetID()] = metaData;
    pwalletMain->AddKeyPubKey(newKey, newKey.GetPubKey());
    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, false) == 2);
    BOOST_CHECK(CBitcoinAddress(newKey.GetPubKey().GetID()).ToString() == "1JCWakvgoCcKHLydjrbFqeAM1vyF36sRe8");

    // derive internal/change key
    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, true) == 0); //no internal key should be there
    pwalletMain->DeriveKeyAtIndex(chain.chainID, newKey, keypath, pwalletMain->GetNextChildIndex(chain.chainID, true), true);
    metaData = CKeyMetadata(GetTime());
    metaData.keypath = keypath;
    metaData.chainID = chain.chainID;
    pwalletMain->mapKeyMetadata[newKey.GetPubKey().GetID()] = metaData;
    pwalletMain->AddKeyPubKey(newKey, newKey.GetPubKey());
    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, true) == 1);
    BOOST_CHECK(CBitcoinAddress(newKey.GetPubKey().GetID()).ToString() == "1HbQusfqAYSUEBqVfHzhasVqGatn6Jjqwq");

    pwalletMain->EncryptSeeds();
    BOOST_CHECK(pwalletMain->HaveKey(newKey.GetPubKey().GetID()) == true);

    CKey keyTest;
    pwalletMain->GetKey(newKey.GetPubKey().GetID(), keyTest);
    BOOST_CHECK(CBitcoinSecret(keyTest).ToString() == "L1igds57v39YDHZ1LirRZQkRiikPkPCECiDarFaiYxJ6JSyhXLn6"); //m/1'/0'

//
//    CPubKey pkey = pwalletMain->GenerateNewKey();
//    std::string test23 = CBitcoinAddress(pkey.GetID()).ToString();
//
//    BOOST_CHECK(CBitcoinAddress(pkey.GetID()).ToString() == "1PEzTQaYAqcnLR6Ug23pzFqTEakBttFrgo");
//    BOOST_CHECK(pwalletMain->GetNextChildIndex(chain.chainID, false) == 3);
}

BOOST_AUTO_TEST_SUITE_END()
