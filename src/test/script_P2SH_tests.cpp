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
extern bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                         bool fValidatePayToScriptHash, int nHashType);

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

    return VerifyScript(scriptSig, scriptPubKey, txTo, 0, fStrict, 0);
}


BOOST_AUTO_TEST_SUITE(script_P2SH_tests)

BOOST_AUTO_TEST_CASE(sign)
{
    // Pay-to-script-hash looks like this:
    // scriptSig:    <sig> <sig...> OP_CODESEPARATOR <script>
    // scriptPubKey: <hash> CHECKHASHVERIFY DROP

    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    CBasicKeyStore keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey();
        keystore.AddKey(key[i]);
    }

    // 8 Scripts: checking all combinations of
    // different keys, straight/P2SH, pubkey/pubkeyhash
    CScript standardScripts[4];
    standardScripts[0] << key[0].GetPubKey() << OP_CHECKSIG;
    standardScripts[1].SetBitcoinAddress(key[1].GetPubKey());
    standardScripts[2] << key[1].GetPubKey() << OP_CHECKSIG;
    standardScripts[3].SetBitcoinAddress(key[2].GetPubKey());
    CScript evalScripts[4];
    for (int i = 0; i < 4; i++)
    {
        keystore.AddCScript(standardScripts[i]);
        evalScripts[i].SetPayToScriptHash(standardScripts[i]);
    }

    CTransaction txFrom;  // Funding transaction:
    txFrom.vout.resize(8);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = evalScripts[i];
        txFrom.vout[i+4].scriptPubKey = standardScripts[i];
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
            bool sigOK = VerifySignature(txFrom, txTo[i], 0, true, 0);
            if (i == j)
                BOOST_CHECK_MESSAGE(sigOK, strprintf("VerifySignature %d %d", i, j));
            else
                BOOST_CHECK_MESSAGE(!sigOK, strprintf("VerifySignature %d %d", i, j));
            txTo[i].vin[0].scriptSig = sigSave;
        }
}

BOOST_AUTO_TEST_CASE(set)
{
    // Test the CScript::Set* methods
    CBasicKeyStore keystore;
    CKey key[4];
    std::vector<CKey> keys;
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey();
        keystore.AddKey(key[i]);
        keys.push_back(key[i]);
    }

    CScript inner[4];
    inner[0].SetBitcoinAddress(key[0].GetPubKey());
    inner[1].SetMultisig(2, std::vector<CKey>(keys.begin(), keys.begin()+2));
    inner[2].SetMultisig(1, std::vector<CKey>(keys.begin(), keys.begin()+2));
    inner[3].SetMultisig(2, std::vector<CKey>(keys.begin(), keys.begin()+3));

    CScript outer[4];
    for (int i = 0; i < 4; i++)
    {
        outer[i].SetPayToScriptHash(inner[i]);
        keystore.AddCScript(inner[i]);
    }

    CTransaction txFrom;  // Funding transaction:
    txFrom.vout.resize(4);
    for (int i = 0; i < 4; i++)
    {
        txFrom.vout[i].scriptPubKey = outer[i];
    }
    BOOST_CHECK(txFrom.IsStandard());

    CTransaction txTo[4]; // Spending transactions
    for (int i = 0; i < 4; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
        txTo[i].vout[0].scriptPubKey = inner[i];
        BOOST_CHECK_MESSAGE(IsMine(keystore, txFrom.vout[i].scriptPubKey), strprintf("IsMine %d", i));
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, txFrom, txTo[i], 0), strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(txTo[i].IsStandard(), strprintf("txTo[%d].IsStandard", i));
    }
}

BOOST_AUTO_TEST_CASE(solver)
{
    // Test CScript::Solver
    uint160 dummy;
    CScript p2sh;
    txnouttype type;
    std::vector<std::vector<unsigned char> > vSolutions;
    p2sh << dummy << OP_CHECKHASHVERIFY << OP_DROP;
    BOOST_CHECK(Solver(p2sh, type, vSolutions));
    BOOST_CHECK(type == TX_SCRIPTHASH);
}

BOOST_AUTO_TEST_CASE(switchover)
{
    // Test switchover code
    CScript notValid;
    notValid << OP_2;
    CScript scriptSig;
    scriptSig << OP_CODESEPARATOR << OP_1;

    CScript fund;
    fund.SetPayToScriptHash(notValid);


    // Validation should succeed under old rules (hash is correct):
    BOOST_CHECK(Verify(scriptSig, fund, false));
    // Fail under new:
    BOOST_CHECK(!Verify(scriptSig, fund, true));
}

BOOST_AUTO_TEST_CASE(AreInputsStandard)
{
    std::map<uint256, std::pair<CTxIndex, CTransaction> > mapInputs;
    CBasicKeyStore keystore;
    CKey key[3];
    vector<CKey> keys;
    for (int i = 0; i < 3; i++)
    {
        key[i].MakeNewKey();
        keystore.AddKey(key[i]);
        keys.push_back(key[i]);
    }

    CTransaction txFrom;
    txFrom.vout.resize(5);

    // First three are standard:
    CScript pay1; pay1.SetBitcoinAddress(key[0].GetPubKey());
    keystore.AddCScript(pay1);
    CScript payScriptHash1; payScriptHash1.SetPayToScriptHash(pay1);
    CScript pay1of3; pay1of3.SetMultisig(1, keys);

    txFrom.vout[0].scriptPubKey = payScriptHash1;
    txFrom.vout[1].scriptPubKey = pay1;
    txFrom.vout[2].scriptPubKey = pay1of3;

    // Last two non-standard:
    CScript empty;
    keystore.AddCScript(empty);
    txFrom.vout[3].scriptPubKey = empty;
    // This is missing DROP:
    txFrom.vout[4].scriptPubKey << Hash160(empty) << OP_CHECKHASHVERIFY;

    mapInputs[txFrom.GetHash()] = make_pair(CTxIndex(), txFrom);

    CTransaction txTo;
    txTo.vout.resize(1);
    txTo.vout[0].scriptPubKey.SetBitcoinAddress(key[1].GetPubKey());

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

    BOOST_CHECK(txTo.AreInputsStandard(mapInputs));

    CTransaction txToNonStd;
    txToNonStd.vout.resize(1);
    txToNonStd.vout[0].scriptPubKey.SetBitcoinAddress(key[1].GetPubKey());
    txToNonStd.vin.resize(1);
    txToNonStd.vin[0].prevout.n = 4;
    txToNonStd.vin[0].prevout.hash = txFrom.GetHash();
    txToNonStd.vin[0].scriptSig << Serialize(empty);

    BOOST_CHECK(!txToNonStd.AreInputsStandard(mapInputs));

    txToNonStd.vin[0].scriptSig.clear();
    BOOST_CHECK(!txToNonStd.AreInputsStandard(mapInputs));
}

BOOST_AUTO_TEST_SUITE_END()
