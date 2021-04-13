// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>
#include <key.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vector>

#include <boost/test/unit_test.hpp>

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s.begin(), s.end());
    return sSerialized;
}

static bool
Verify(const CScript& scriptSig, const CScript& scriptPubKey, bool fStrict, ScriptError& err)
{
    // Create dummy to/from transactions:
    CMutableTransaction txFrom;
    txFrom.vout.resize(1);
    txFrom.vout[0].scriptPubKey = scriptPubKey;

    CMutableTransaction txTo;
    txTo.vin.resize(1);
    txTo.vout.resize(1);
    txTo.vin[0].prevout.n = 0;
    txTo.vin[0].prevout.hash = txFrom.GetHash();
    txTo.vin[0].scriptSig = scriptSig;
    txTo.vout[0].nValue = 1;

    return VerifyScript(scriptSig, scriptPubKey, nullptr, fStrict ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE, MutableTransactionSignatureChecker(&txTo, 0, txFrom.vout[0].nValue, MissingDataBehavior::ASSERT_FAIL), &err);
}


BOOST_FIXTURE_TEST_SUITE(script_p2sh_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sign)
{
    LOCK(cs_main);
    // Pay-to-script-hash looks like this:
    // scriptSig:    <sig> <sig...> <serialized_script>
    // scriptPubKey: HASH160 <hash> EQUAL

    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    FillableSigningProvider keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        BOOST_CHECK(keystore.AddKey(key[i]));
    }

    // 8 Scripts: checking all combinations of
    // different keys, straight/P2SH, pubkey/pubkeyhash
    CScript standardScripts[4];
    standardScripts[0] << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    standardScripts[1] = GetScriptForDestination(PKHash(key[1].GetPubKey()));
    standardScripts[2] << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    standardScripts[3] = GetScriptForDestination(PKHash(key[2].GetPubKey()));
    CScript evalScripts[4];
    for (int i = 0; i < 4; i++)
    {
        BOOST_CHECK(keystore.AddCScript(standardScripts[i]));
        evalScripts[i] = GetScriptForDestination(ScriptHash(standardScripts[i]));
    }

    CMutableTransaction txFrom;  // Funding transaction:
    std::string reason;
    txFrom.vout.resize(8);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = evalScripts[i];
        txFrom.vout[i].nValue = COIN;
        txFrom.vout[i+4].scriptPubKey = standardScripts[i];
        txFrom.vout[i+4].nValue = COIN;
    }
    BOOST_CHECK(IsStandardTx(CTransaction(txFrom), reason));

    CMutableTransaction txTo[8]; // Spending transactions
    for (int i = 0; i < 8; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
    }
    for (int i = 0; i < 8; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, CTransaction(txFrom), txTo[i], 0, SIGHASH_ALL), strprintf("SignSignature %d", i));
    }
    // All of the above should be OK, and the txTos have valid signatures
    // Check to make sure signature verification fails if we use the wrong ScriptSig:
    for (int i = 0; i < 8; i++) {
        PrecomputedTransactionData txdata(txTo[i]);
        for (int j = 0; j < 8; j++)
        {
            CScript sigSave = txTo[i].vin[0].scriptSig;
            txTo[i].vin[0].scriptSig = txTo[j].vin[0].scriptSig;
            bool sigOK = CScriptCheck(txFrom.vout[txTo[i].vin[0].prevout.n], CTransaction(txTo[i]), 0, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, false, &txdata)();
            if (i == j)
                BOOST_CHECK_MESSAGE(sigOK, strprintf("VerifySignature %d %d", i, j));
            else
                BOOST_CHECK_MESSAGE(!sigOK, strprintf("VerifySignature %d %d", i, j));
            txTo[i].vin[0].scriptSig = sigSave;
        }
    }
}

BOOST_AUTO_TEST_CASE(norecurse)
{
    ScriptError err;
    // Make sure only the outer pay-to-script-hash does the
    // extra-validation thing:
    CScript invalidAsScript;
    invalidAsScript << OP_INVALIDOPCODE << OP_INVALIDOPCODE;

    CScript p2sh = GetScriptForDestination(ScriptHash(invalidAsScript));

    CScript scriptSig;
    scriptSig << Serialize(invalidAsScript);

    // Should not verify, because it will try to execute OP_INVALIDOPCODE
    BOOST_CHECK(!Verify(scriptSig, p2sh, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_BAD_OPCODE, ScriptErrorString(err));

    // Try to recur, and verification should succeed because
    // the inner HASH160 <> EQUAL should only check the hash:
    CScript p2sh2 = GetScriptForDestination(ScriptHash(p2sh));
    CScript scriptSig2;
    scriptSig2 << Serialize(invalidAsScript) << Serialize(p2sh);

    BOOST_CHECK(Verify(scriptSig2, p2sh2, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
}

BOOST_AUTO_TEST_CASE(set)
{
    LOCK(cs_main);
    // Test the CScript::Set* methods
    FillableSigningProvider keystore;
    CKey key[4];
    std::vector<CPubKey> keys;
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        BOOST_CHECK(keystore.AddKey(key[i]));
        keys.push_back(key[i].GetPubKey());
    }

    CScript inner[4];
    inner[0] = GetScriptForDestination(PKHash(key[0].GetPubKey()));
    inner[1] = GetScriptForMultisig(2, std::vector<CPubKey>(keys.begin(), keys.begin()+2));
    inner[2] = GetScriptForMultisig(1, std::vector<CPubKey>(keys.begin(), keys.begin()+2));
    inner[3] = GetScriptForMultisig(2, std::vector<CPubKey>(keys.begin(), keys.begin()+3));

    CScript outer[4];
    for (int i = 0; i < 4; i++)
    {
        outer[i] = GetScriptForDestination(ScriptHash(inner[i]));
        BOOST_CHECK(keystore.AddCScript(inner[i]));
    }

    CMutableTransaction txFrom;  // Funding transaction:
    std::string reason;
    txFrom.vout.resize(4);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = outer[i];
        txFrom.vout[i].nValue = CENT;
    }
    BOOST_CHECK(IsStandardTx(CTransaction(txFrom), reason));

    CMutableTransaction txTo[4]; // Spending transactions
    for (int i = 0; i < 4; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1*CENT;
        txTo[i].vout[0].scriptPubKey = inner[i];
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, CTransaction(txFrom), txTo[i], 0, SIGHASH_ALL), strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(IsStandardTx(CTransaction(txTo[i]), reason), strprintf("txTo[%d].IsStandard", i));
    }
}

BOOST_AUTO_TEST_CASE(is)
{
    // Test CScript::IsPayToScriptHash()
    uint160 dummy;
    CScript p2sh;
    p2sh << OP_HASH160 << ToByteVector(dummy) << OP_EQUAL;
    BOOST_CHECK(p2sh.IsPayToScriptHash());

    std::vector<unsigned char> direct = {OP_HASH160, 20};
    direct.insert(direct.end(), 20, 0);
    direct.push_back(OP_EQUAL);
    BOOST_CHECK(CScript(direct.begin(), direct.end()).IsPayToScriptHash());

    // Not considered pay-to-script-hash if using one of the OP_PUSHDATA opcodes:
    std::vector<unsigned char> pushdata1 = {OP_HASH160, OP_PUSHDATA1, 20};
    pushdata1.insert(pushdata1.end(), 20, 0);
    pushdata1.push_back(OP_EQUAL);
    BOOST_CHECK(!CScript(pushdata1.begin(), pushdata1.end()).IsPayToScriptHash());
    std::vector<unsigned char> pushdata2 = {OP_HASH160, OP_PUSHDATA2, 20, 0};
    pushdata2.insert(pushdata2.end(), 20, 0);
    pushdata2.push_back(OP_EQUAL);
    BOOST_CHECK(!CScript(pushdata2.begin(), pushdata2.end()).IsPayToScriptHash());
    std::vector<unsigned char> pushdata4 = {OP_HASH160, OP_PUSHDATA4, 20, 0, 0, 0};
    pushdata4.insert(pushdata4.end(), 20, 0);
    pushdata4.push_back(OP_EQUAL);
    BOOST_CHECK(!CScript(pushdata4.begin(), pushdata4.end()).IsPayToScriptHash());

    CScript not_p2sh;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_HASH160 << ToByteVector(dummy) << ToByteVector(dummy) << OP_EQUAL;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_NOP << ToByteVector(dummy) << OP_EQUAL;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_HASH160 << ToByteVector(dummy) << OP_CHECKSIG;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());
}

BOOST_AUTO_TEST_CASE(switchover)
{
    // Test switch over code
    CScript notValid;
    ScriptError err;
    notValid << OP_11 << OP_12 << OP_EQUALVERIFY;
    CScript scriptSig;
    scriptSig << Serialize(notValid);

    CScript fund = GetScriptForDestination(ScriptHash(notValid));


    // Validation should succeed under old rules (hash is correct):
    BOOST_CHECK(Verify(scriptSig, fund, false, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    // Fail under new:
    BOOST_CHECK(!Verify(scriptSig, fund, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EQUALVERIFY, ScriptErrorString(err));
}

BOOST_AUTO_TEST_CASE(AreInputsStandard)
{
    LOCK(cs_main);
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    FillableSigningProvider keystore;
    CKey key[6];
    std::vector<CPubKey> keys;
    for (int i = 0; i < 6; i++)
    {
        key[i].MakeNewKey(true);
        BOOST_CHECK(keystore.AddKey(key[i]));
    }
    for (int i = 0; i < 3; i++)
        keys.push_back(key[i].GetPubKey());

    CMutableTransaction txFrom;
    txFrom.vout.resize(7);

    // First three are standard:
    CScript pay1 = GetScriptForDestination(PKHash(key[0].GetPubKey()));
    BOOST_CHECK(keystore.AddCScript(pay1));
    CScript pay1of3 = GetScriptForMultisig(1, keys);

    txFrom.vout[0].scriptPubKey = GetScriptForDestination(ScriptHash(pay1)); // P2SH (OP_CHECKSIG)
    txFrom.vout[0].nValue = 1000;
    txFrom.vout[1].scriptPubKey = pay1; // ordinary OP_CHECKSIG
    txFrom.vout[1].nValue = 2000;
    txFrom.vout[2].scriptPubKey = pay1of3; // ordinary OP_CHECKMULTISIG
    txFrom.vout[2].nValue = 3000;

    // vout[3] is complicated 1-of-3 AND 2-of-3
    // ... that is OK if wrapped in P2SH:
    CScript oneAndTwo;
    oneAndTwo << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey());
    oneAndTwo << OP_3 << OP_CHECKMULTISIGVERIFY;
    oneAndTwo << OP_2 << ToByteVector(key[3].GetPubKey()) << ToByteVector(key[4].GetPubKey()) << ToByteVector(key[5].GetPubKey());
    oneAndTwo << OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK(keystore.AddCScript(oneAndTwo));
    txFrom.vout[3].scriptPubKey = GetScriptForDestination(ScriptHash(oneAndTwo));
    txFrom.vout[3].nValue = 4000;

    // vout[4] is max sigops:
    CScript fifteenSigops; fifteenSigops << OP_1;
    for (unsigned i = 0; i < MAX_P2SH_SIGOPS; i++)
        fifteenSigops << ToByteVector(key[i%3].GetPubKey());
    fifteenSigops << OP_15 << OP_CHECKMULTISIG;
    BOOST_CHECK(keystore.AddCScript(fifteenSigops));
    txFrom.vout[4].scriptPubKey = GetScriptForDestination(ScriptHash(fifteenSigops));
    txFrom.vout[4].nValue = 5000;

    // vout[5/6] are non-standard because they exceed MAX_P2SH_SIGOPS
    CScript sixteenSigops; sixteenSigops << OP_16 << OP_CHECKMULTISIG;
    BOOST_CHECK(keystore.AddCScript(sixteenSigops));
    txFrom.vout[5].scriptPubKey = GetScriptForDestination(ScriptHash(sixteenSigops));
    txFrom.vout[5].nValue = 5000;
    CScript twentySigops; twentySigops << OP_CHECKMULTISIG;
    BOOST_CHECK(keystore.AddCScript(twentySigops));
    txFrom.vout[6].scriptPubKey = GetScriptForDestination(ScriptHash(twentySigops));
    txFrom.vout[6].nValue = 6000;

    AddCoins(coins, CTransaction(txFrom), 0);

    CMutableTransaction txTo;
    txTo.vout.resize(1);
    txTo.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key[1].GetPubKey()));

    txTo.vin.resize(5);
    for (int i = 0; i < 5; i++)
    {
        txTo.vin[i].prevout.n = i;
        txTo.vin[i].prevout.hash = txFrom.GetHash();
    }
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL));
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 1, SIGHASH_ALL));
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 2, SIGHASH_ALL));
    // SignSignature doesn't know how to sign these. We're
    // not testing validating signatures, so just create
    // dummy signatures that DO include the correct P2SH scripts:
    txTo.vin[3].scriptSig << OP_11 << OP_11 << std::vector<unsigned char>(oneAndTwo.begin(), oneAndTwo.end());
    txTo.vin[4].scriptSig << std::vector<unsigned char>(fifteenSigops.begin(), fifteenSigops.end());

    BOOST_CHECK(::AreInputsStandard(CTransaction(txTo), coins, false));
    // 22 P2SH sigops for all inputs (1 for vin[0], 6 for vin[3], 15 for vin[4]
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(CTransaction(txTo), coins), 22U);

    CMutableTransaction txToNonStd1;
    txToNonStd1.vout.resize(1);
    txToNonStd1.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key[1].GetPubKey()));
    txToNonStd1.vout[0].nValue = 1000;
    txToNonStd1.vin.resize(1);
    txToNonStd1.vin[0].prevout.n = 5;
    txToNonStd1.vin[0].prevout.hash = txFrom.GetHash();
    txToNonStd1.vin[0].scriptSig << std::vector<unsigned char>(sixteenSigops.begin(), sixteenSigops.end());

    BOOST_CHECK(!::AreInputsStandard(CTransaction(txToNonStd1), coins, false));
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(CTransaction(txToNonStd1), coins), 16U);

    CMutableTransaction txToNonStd2;
    txToNonStd2.vout.resize(1);
    txToNonStd2.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key[1].GetPubKey()));
    txToNonStd2.vout[0].nValue = 1000;
    txToNonStd2.vin.resize(1);
    txToNonStd2.vin[0].prevout.n = 6;
    txToNonStd2.vin[0].prevout.hash = txFrom.GetHash();
    txToNonStd2.vin[0].scriptSig << std::vector<unsigned char>(twentySigops.begin(), twentySigops.end());

    BOOST_CHECK(!::AreInputsStandard(CTransaction(txToNonStd2), coins, false));
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(CTransaction(txToNonStd2), coins), 20U);
}

BOOST_AUTO_TEST_SUITE_END()