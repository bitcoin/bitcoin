// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <wallet/ismine.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(ismine_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ismine_standard)
{
    CKey keys[2];
    CPubKey pubkeys[2];
    for (int i = 0; i < 2; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CKey uncompressedKey;
    uncompressedKey.MakeNewKey(false);
    CPubKey uncompressedPubkey = uncompressedKey.GetPubKey();
    auto chain = interfaces::MakeChain();

    CScript scriptPubKey;
    isminetype result;

    // P2PK compressed
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForRawPubKey(pubkeys[0]);

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PK uncompressed
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForRawPubKey(uncompressedPubkey);

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH compressed
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForDestination(pubkeys[0].GetID());

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH uncompressed
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForDestination(uncompressedPubkey.GetID());

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2SH
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);

        CScript redeemScript = GetScriptForDestination(pubkeys[0].GetID());
        scriptPubKey = GetScriptForDestination(CScriptID(redeemScript));

        // Keystore does not have redeemScript or key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript but no key
        BOOST_CHECK(keystore.AddCScript(redeemScript));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript and key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    //  (P2PKH inside) P2SH inside P2SH (invalid)
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);

        CScript redeemscript_inner = GetScriptForDestination(pubkeys[0].GetID());
        CScript redeemscript = GetScriptForDestination(CScriptID(redeemscript_inner));
        scriptPubKey = GetScriptForDestination(CScriptID(redeemscript));

        BOOST_CHECK(keystore.AddCScript(redeemscript));
        BOOST_CHECK(keystore.AddCScript(redeemscript_inner));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // scriptPubKey multisig
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);

        scriptPubKey = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});

        // Keystore does not have any keys
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 1/2 keys
        BOOST_CHECK(keystore.AddKey(uncompressedKey));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 2/2 keys
        BOOST_CHECK(keystore.AddKey(keys[1]));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 2/2 keys and the script
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2SH multisig
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        CScript redeemScript = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        scriptPubKey = GetScriptForDestination(CScriptID(redeemScript));

        // Keystore has no redeemScript
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript
        BOOST_CHECK(keystore.AddCScript(redeemScript));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // OP_RETURN
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_RETURN << ToByteVector(pubkeys[0]);

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // Nonstandard
    {
        CWallet keystore(*chain, WalletLocation(), CreateDummyWalletDatabase());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_9 << OP_ADD << OP_11 << OP_EQUAL;

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }
}

BOOST_AUTO_TEST_SUITE_END()
