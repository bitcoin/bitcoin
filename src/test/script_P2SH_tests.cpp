#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "../main.h"
#include "../script.h"
#include "../wallet.h"

using namespace std;

// Test routines internal to script.cpp:
extern uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s);
    return sSerialized;
}

static bool
Verify(const CScript& scriptSig, const CScript& scriptPubKey, bool fStrict)
{
    // Create dummy to/from transactions:
    CTransaction txFrom;
    txFrom.vout.resize(1);
    txFrom.vout[0].scriptPubKey = scriptPubKey;

    CTransaction txTo;
    txTo.vin.resize(1);
    txTo.vout.resize(1);
    txTo.vin[0].prevout.n = 0;
    txTo.vin[0].prevout.hash = txFrom.GetHash();
    txTo.vin[0].scriptSig = scriptSig;
    txTo.vout[0].nValue = 1;

    return VerifyScript(scriptSig, scriptPubKey, txTo, 0, fStrict ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE, 0);
}


BOOST_AUTO_TEST_SUITE(script_P2SH_tests)

BOOST_AUTO_TEST_CASE(sign)
{
    // Pay-to-script-hash looks like this:
    // scriptSig:    <sig> <sig...> <serialized_script>
    // scriptPubKey: HASH160 <hash> EQUAL

    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    CBasicKeyStore keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
    }

    // 8 Scripts: checking all combinations of
    // different keys, straight/P2SH, pubkey/pubkeyhash
    CScript standardScripts[4];
    standardScripts[0] << key[0].GetPubKey() << OP_CHECKSIG;
    standardScripts[1].SetDestination(key[1].GetPubKey().GetID());
    standardScripts[2] << key[1].GetPubKey() << OP_CHECKSIG;
    standardScripts[3].SetDestination(key[2].GetPubKey().GetID());
    CScript evalScripts[4];
    for (int i = 0; i < 4; i++)
    {
        keystore.AddCScript(standardScripts[i]);
        evalScripts[i].SetDestination(standardScripts[i].GetID());
    }

    CTransaction txFrom;  // Funding transaction:
    txFrom.vout.resize(8);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = evalScripts[i];
        txFrom.vout[i].nValue = COIN;
        txFrom.vout[i+4].scriptPubKey = standardScripts[i];
        txFrom.vout[i+4].nValue = COIN;
    }
    BOOST_CHECK(txFrom.IsStandard());

    CTransaction txTo[8]; // Spending transactions
    for (int i = 0; i < 8; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
        BOOST_CHECK_MESSAGE(IsMine(keystore, txFrom.vout[i].scriptPubKey), strprintf("IsMine %d", i));
    }
    for (int i = 0; i < 8; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, txFrom, txTo[i], 0), strprintf("SignSignature %d", i));
    }
    // All of the above should be OK, and the txTos have valid signatures
    // Check to make sure signature verification fails if we use the wrong ScriptSig:
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            CScript sigSave = txTo[i].vin[0].scriptSig;
            txTo[i].vin[0].scriptSig = txTo[j].vin[0].scriptSig;
            bool sigOK = VerifySignature(CCoins(txFrom, 0), txTo[i], 0, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, 0);
            if (i == j)
                BOOST_CHECK_MESSAGE(sigOK, strprintf("VerifySignature %d %d", i, j));
            else
                BOOST_CHECK_MESSAGE(!sigOK, strprintf("VerifySignature %d %d", i, j));
            txTo[i].vin[0].scriptSig = sigSave;
        }
}

BOOST_AUTO_TEST_CASE(norecurse)
{
    // Make sure only the outer pay-to-script-hash does the
    // extra-validation thing:
    CScript invalidAsScript;
    invalidAsScript << OP_INVALIDOPCODE << OP_INVALIDOPCODE;

    CScript p2sh;
    p2sh.SetDestination(invalidAsScript.GetID());

    CScript scriptSig;
    scriptSig << Serialize(invalidAsScript);

    // Should not verify, because it will try to execute OP_INVALIDOPCODE
    BOOST_CHECK(!Verify(scriptSig, p2sh, true));

    // Try to recur, and verification should succeed because
    // the inner HASH160 <> EQUAL should only check the hash:
    CScript p2sh2;
    p2sh2.SetDestination(p2sh.GetID());
    CScript scriptSig2;
    scriptSig2 << Serialize(invalidAsScript) << Serialize(p2sh);

    BOOST_CHECK(Verify(scriptSig2, p2sh2, true));
}

BOOST_AUTO_TEST_CASE(set)
{
    // Test the CScript::Set* methods
    CBasicKeyStore keystore;
    CKey key[4];
    std::vector<CKey> keys;
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
        keys.push_back(key[i]);
    }

    CScript inner[4];
    inner[0].SetDestination(key[0].GetPubKey().GetID());
    inner[1].SetMultisig(2, std::vector<CKey>(keys.begin(), keys.begin()+2));
    inner[2].SetMultisig(1, std::vector<CKey>(keys.begin(), keys.begin()+2));
    inner[3].SetMultisig(2, std::vector<CKey>(keys.begin(), keys.begin()+3));

    CScript outer[4];
    for (int i = 0; i < 4; i++)
    {
        outer[i].SetDestination(inner[i].GetID());
        keystore.AddCScript(inner[i]);
    }

    CTransaction txFrom;  // Funding transaction:
    txFrom.vout.resize(4);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = outer[i];
        txFrom.vout[i].nValue = CENT;
    }
    BOOST_CHECK(txFrom.IsStandard());

    CTransaction txTo[4]; // Spending transactions
    for (int i = 0; i < 4; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1*CENT;
        txTo[i].vout[0].scriptPubKey = inner[i];
        BOOST_CHECK_MESSAGE(IsMine(keystore, txFrom.vout[i].scriptPubKey), strprintf("IsMine %d", i));
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, txFrom, txTo[i], 0), strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(txTo[i].IsStandard(), strprintf("txTo[%d].IsStandard", i));
    }
}

BOOST_AUTO_TEST_CASE(is)
{
    // Test CScript::IsPayToScriptHash()
    uint160 dummy;
    CScript p2sh;
    p2sh << OP_HASH160 << dummy << OP_EQUAL;
    BOOST_CHECK(p2sh.IsPayToScriptHash());

    // Not considered pay-to-script-hash if using one of the OP_PUSHDATA opcodes:
    static const unsigned char direct[] =    { OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL };
    BOOST_CHECK(CScript(direct, direct+sizeof(direct)).IsPayToScriptHash());
    static const unsigned char pushdata1[] = { OP_HASH160, OP_PUSHDATA1, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL };
    BOOST_CHECK(!CScript(pushdata1, pushdata1+sizeof(pushdata1)).IsPayToScriptHash());
    static const unsigned char pushdata2[] = { OP_HASH160, OP_PUSHDATA2, 20,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL };
    BOOST_CHECK(!CScript(pushdata2, pushdata2+sizeof(pushdata2)).IsPayToScriptHash());
    static const unsigned char pushdata4[] = { OP_HASH160, OP_PUSHDATA4, 20,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL };
    BOOST_CHECK(!CScript(pushdata4, pushdata4+sizeof(pushdata4)).IsPayToScriptHash());

    CScript not_p2sh;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_HASH160 << dummy << dummy << OP_EQUAL;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_NOP << dummy << OP_EQUAL;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());

    not_p2sh.clear(); not_p2sh << OP_HASH160 << dummy << OP_CHECKSIG;
    BOOST_CHECK(!not_p2sh.IsPayToScriptHash());
}

BOOST_AUTO_TEST_CASE(switchover)
{
    // Test switch over code
    CScript notValid;
    notValid << OP_11 << OP_12 << OP_EQUALVERIFY;
    CScript scriptSig;
    scriptSig << Serialize(notValid);

    CScript fund;
    fund.SetDestination(notValid.GetID());


    // Validation should succeed under old rules (hash is correct):
    BOOST_CHECK(Verify(scriptSig, fund, false));
    // Fail under new:
    BOOST_CHECK(!Verify(scriptSig, fund, true));
}

BOOST_AUTO_TEST_CASE(AreInputsStandard)
{
    CCoinsView coinsDummy;
    CCoinsViewCache coins(coinsDummy);
    CBasicKeyStore keystore;
    CKey key[3];
    vector<CKey> keys;
    for (int i = 0; i < 3; i++)
    {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
        keys.push_back(key[i]);
    }

    CTransaction txFrom;
    txFrom.vout.resize(6);

    // First three are standard:
    CScript pay1; pay1.SetDestination(key[0].GetPubKey().GetID());
    keystore.AddCScript(pay1);
    CScript payScriptHash1; payScriptHash1.SetDestination(pay1.GetID());
    CScript pay1of3; pay1of3.SetMultisig(1, keys);

    txFrom.vout[0].scriptPubKey = payScriptHash1;
    txFrom.vout[0].nValue = 1000;
    txFrom.vout[1].scriptPubKey = pay1;
    txFrom.vout[1].nValue = 2000;
    txFrom.vout[2].scriptPubKey = pay1of3;
    txFrom.vout[2].nValue = 3000;

    // Last three non-standard:
    CScript empty;
    keystore.AddCScript(empty);
    txFrom.vout[3].scriptPubKey = empty;
    txFrom.vout[3].nValue = 4000;
    // Can't use SetPayToScriptHash, it checks for the empty Script. So:
    txFrom.vout[4].scriptPubKey << OP_HASH160 << Hash160(empty) << OP_EQUAL;
    txFrom.vout[4].nValue = 5000;
    CScript oneOfEleven;
    oneOfEleven << OP_1;
    for (int i = 0; i < 11; i++)
        oneOfEleven << key[0].GetPubKey();
    oneOfEleven << OP_11 << OP_CHECKMULTISIG;
    txFrom.vout[5].scriptPubKey.SetDestination(oneOfEleven.GetID());
    txFrom.vout[5].nValue = 6000;

    coins.SetCoins(txFrom.GetHash(), CCoins(txFrom, 0));

    CTransaction txTo;
    txTo.vout.resize(1);
    txTo.vout[0].scriptPubKey.SetDestination(key[1].GetPubKey().GetID());

    txTo.vin.resize(3);
    txTo.vin[0].prevout.n = 0;
    txTo.vin[0].prevout.hash = txFrom.GetHash();
    BOOST_CHECK(SignSignature(keystore, txFrom, txTo, 0));
    txTo.vin[1].prevout.n = 1;
    txTo.vin[1].prevout.hash = txFrom.GetHash();
    BOOST_CHECK(SignSignature(keystore, txFrom, txTo, 1));
    txTo.vin[2].prevout.n = 2;
    txTo.vin[2].prevout.hash = txFrom.GetHash();
    BOOST_CHECK(SignSignature(keystore, txFrom, txTo, 2));

    BOOST_CHECK(txTo.AreInputsStandard(coins));
    BOOST_CHECK_EQUAL(txTo.GetP2SHSigOpCount(coins), 1U);

    // Make sure adding crap to the scriptSigs makes them non-standard:
    for (int i = 0; i < 3; i++)
    {
        CScript t = txTo.vin[i].scriptSig;
        txTo.vin[i].scriptSig = (CScript() << 11) + t;
        BOOST_CHECK(!txTo.AreInputsStandard(coins));
        txTo.vin[i].scriptSig = t;
    }

    CTransaction txToNonStd;
    txToNonStd.vout.resize(1);
    txToNonStd.vout[0].scriptPubKey.SetDestination(key[1].GetPubKey().GetID());
    txToNonStd.vout[0].nValue = 1000;
    txToNonStd.vin.resize(2);
    txToNonStd.vin[0].prevout.n = 4;
    txToNonStd.vin[0].prevout.hash = txFrom.GetHash();
    txToNonStd.vin[0].scriptSig << Serialize(empty);
    txToNonStd.vin[1].prevout.n = 5;
    txToNonStd.vin[1].prevout.hash = txFrom.GetHash();
    txToNonStd.vin[1].scriptSig << OP_0 << Serialize(oneOfEleven);

    BOOST_CHECK(!txToNonStd.AreInputsStandard(coins));
    BOOST_CHECK_EQUAL(txToNonStd.GetP2SHSigOpCount(coins), 11U);

    txToNonStd.vin[0].scriptSig.clear();
    BOOST_CHECK(!txToNonStd.AreInputsStandard(coins));
}

BOOST_AUTO_TEST_SUITE_END()
