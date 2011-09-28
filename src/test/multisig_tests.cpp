#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include "../keystore.h"
#include "../main.h"
#include "../script.h"

using namespace std;
using namespace boost::assign;

typedef vector<unsigned char> valtype;

extern uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool Solver(const CScript& scriptPubKey, vector<vector<pair<opcodetype, valtype> > >& vSolutionsRet);

BOOST_AUTO_TEST_SUITE(multisig_tests)

CScript
sign_multisig(CScript scriptPubKey, vector<CKey> keys, CTransaction transaction, int whichIn)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL);

    CScript result;
    BOOST_FOREACH(CKey key, keys)
    {
        vector<unsigned char> vchSig;
        BOOST_CHECK(key.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        result << vchSig;
    }
    return result;
}
CScript
sign_multisig(CScript scriptPubKey, CKey key, CTransaction transaction, int whichIn)
{
    vector<CKey> keys;
    keys.push_back(key);
    return sign_multisig(scriptPubKey, keys, transaction, whichIn);
}

BOOST_AUTO_TEST_CASE(multisig_verify)
{
    CKey key[4];
    for (int i = 0; i < 4; i++)
        key[i].MakeNewKey();

    CScript a_and_b;
    a_and_b << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG;

    CScript a_or_b;
    a_or_b << OP_IF << key[1].GetPubKey() << OP_CHECKSIG << OP_ELSE
           << key[0].GetPubKey() << OP_CHECKSIG
           << OP_ENDIF;

    CScript a_and_b_or_c;
    a_and_b_or_c << OP_IF << key[2].GetPubKey() << OP_CHECKSIG << OP_ELSE
                 << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG
                 << OP_ENDIF;

    CTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = a_and_b_or_c;

    CTransaction txTo[3]; // Spending transaction
    for (int i = 0; i < 3; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
    }

    vector<CKey> keys;
    CScript s;

    // Test a AND b:
    keys.clear();
    keys += key[0],key[1]; // magic operator+= from boost.assign
    s = sign_multisig(a_and_b, keys, txTo[0], 0);
    BOOST_CHECK(VerifyScript(s, a_and_b, txTo[0], 0, 0));

    for (int i = 0; i < 4; i++)
    {
        keys.clear();
        keys += key[i];
        s = sign_multisig(a_and_b, keys, txTo[0], 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, txTo[0], 0, 0), strprintf("a&b 1: %d", i));

        keys.clear();
        keys += key[1],key[i];
        s = sign_multisig(a_and_b, keys, txTo[0], 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, txTo[0], 0, 0), strprintf("a&b 2: %d", i));
    }

    // Test a OR b:
    for (int i = 0; i < 4; i++)
    {
        keys.clear();
        keys += key[i];
        s = sign_multisig(a_or_b, keys, txTo[1], 0);
        s << (i == 0 ? OP_0 : OP_1);
        if (i == 0 || i == 1)
            BOOST_CHECK_MESSAGE(VerifyScript(s, a_or_b, txTo[1], 0, 0), strprintf("a|b: %d", i));
        else
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, txTo[1], 0, 0), strprintf("a|b: %d", i));
    }
    s.clear();
    s << OP_0 << OP_0;
    BOOST_CHECK(!VerifyScript(s, a_or_b, txTo[1], 0, 0));
    s.clear();
    s << OP_0 << OP_1;
    BOOST_CHECK(!VerifyScript(s, a_or_b, txTo[1], 0, 0));


    // Test a AND b OR c:
    keys.clear();
    keys += key[0],key[1];
    s = sign_multisig(a_and_b_or_c, keys, txTo[2], 0);
    s << OP_0; // a AND b case
    BOOST_CHECK(VerifyScript(s, a_and_b_or_c, txTo[2], 0, 0));
    keys.clear();
    keys += key[2];
    s = sign_multisig(a_and_b_or_c, keys, txTo[2], 0);
    s << OP_1; // OR c case
    BOOST_CHECK(VerifyScript(s, a_and_b_or_c, txTo[2], 0, 0));


    for (int i = 0; i < 4; i++)
    {
        keys.clear();
        keys += key[i];
        s = sign_multisig(a_and_b_or_c, keys, txTo[2], 0);
        s << OP_0; // a AND b case
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b_or_c, txTo[2], 0, 0), strprintf("a&b|c 1: %d", i));

        if (i != 2)
        {
            keys.clear();
            keys += key[i];
            s = sign_multisig(a_and_b_or_c, keys, txTo[2], 0);
            s << OP_1; // c case
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b_or_c, txTo[2], 0, 0), strprintf("a&b|c 2: %d", i));
        }
    }
}

BOOST_AUTO_TEST_CASE(multisig_IsStandard)
{
    CKey key[3];
    for (int i = 0; i < 3; i++)
        key[i].MakeNewKey();

    CScript a_and_b;
    a_and_b << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG;
    BOOST_CHECK(::IsStandard(a_and_b));

    CScript a_or_b;
    a_or_b << OP_IF << key[1].GetPubKey() << OP_CHECKSIG << OP_ELSE
           << key[0].GetPubKey() << OP_CHECKSIG
           << OP_ENDIF;
    BOOST_CHECK(::IsStandard(a_or_b));

    CScript a_and_b_or_c;
    a_and_b_or_c << OP_IF << key[2].GetPubKey() << OP_CHECKSIG << OP_ELSE
                 << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG
                 << OP_ENDIF;
    BOOST_CHECK(::IsStandard(a_and_b_or_c));
}

BOOST_AUTO_TEST_CASE(multisig_Solver1)
{
    // Tests Solver() that returns lists of keys that are
    // required to satisfy a ScriptPubKey
    //
    // Also tests IsMine() and ExtractAddress()
    //
    // Note: ExtractAddress for the multisignature transactions
    // always returns false for this release, even if you have
    // one key that would satisfy an (a|b) or (a&b|c) transaction.
    //
    CBasicKeyStore keystore, emptykeystore;
    CKey key[3];
    CBitcoinAddress keyaddr[3];
    for (int i = 0; i < 3; i++)
    {
        key[i].MakeNewKey();
        keystore.AddKey(key[i]);
        keyaddr[i].SetPubKey(key[i].GetPubKey());
    }

    {
        vector<vector<pair<opcodetype, valtype> > > solutions;
        CScript s;
        s << key[0].GetPubKey() << OP_CHECKSIG;
        BOOST_CHECK(Solver(s, solutions));
        BOOST_CHECK(solutions.size() == 1);
        BOOST_CHECK(solutions[0].size() == 1);
        CBitcoinAddress addr;
        BOOST_CHECK(ExtractAddress(s, &keystore, addr));
        BOOST_CHECK(addr == keyaddr[0]);
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
    }
    {
        vector<vector<pair<opcodetype, valtype> > > solutions;
        CScript s;
        s << OP_DUP << OP_HASH160 << Hash160(key[0].GetPubKey()) << OP_EQUALVERIFY << OP_CHECKSIG;
        BOOST_CHECK(Solver(s, solutions));
        BOOST_CHECK(solutions.size() == 1);
        BOOST_CHECK(solutions[0].size() == 1);
        CBitcoinAddress addr;
        BOOST_CHECK(ExtractAddress(s, &keystore, addr));
        BOOST_CHECK(addr == keyaddr[0]);
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
    }
    {
        vector<vector<pair<opcodetype, valtype> > > solutions;
        CScript s;
        s << key[0].GetPubKey() << OP_CHECKSIGVERIFY << key[1].GetPubKey() << OP_CHECKSIG;
        BOOST_CHECK(Solver(s, solutions));
        BOOST_CHECK(solutions.size() == 1);
        BOOST_CHECK(solutions[0].size() == 2);
        CBitcoinAddress addr;
        BOOST_CHECK(!ExtractAddress(s, &keystore, addr));
        BOOST_CHECK(!IsMine(keystore, s));
    }
    {
        vector<vector<pair<opcodetype, valtype> > > solutions;
        CScript s;
        s << OP_IF
          << key[0].GetPubKey() << OP_CHECKSIG
          << OP_ELSE
          << key[1].GetPubKey() << OP_CHECKSIG
          << OP_ENDIF;
        BOOST_CHECK(Solver(s, solutions));
        BOOST_CHECK(solutions.size() == 2);
        BOOST_CHECK(solutions[0].size() == 1);
        BOOST_CHECK(solutions[1].size() == 1);
        CBitcoinAddress addr;
        BOOST_CHECK(!ExtractAddress(s, &keystore, addr));
        BOOST_CHECK(!IsMine(keystore, s));
    }
    {
        vector<vector<pair<opcodetype, valtype> > > solutions;
        CScript s;
        s << OP_IF
          << key[0].GetPubKey() << OP_CHECKSIG
          << OP_ELSE
          << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[2].GetPubKey() << OP_CHECKSIG
          << OP_ENDIF;
        BOOST_CHECK(Solver(s, solutions));
        BOOST_CHECK(solutions.size() == 2);
        BOOST_CHECK(solutions[0].size() == 1);
        BOOST_CHECK(solutions[1].size() == 2);
        CBitcoinAddress addr;
        BOOST_CHECK(!ExtractAddress(s, &keystore, addr));
        BOOST_CHECK(!IsMine(keystore, s));
    }
}

BOOST_AUTO_TEST_CASE(multisig_Sign)
{
    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    CBasicKeyStore keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey();
        keystore.AddKey(key[i]);
    }

    CScript a_and_b;
    a_and_b << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG;

    CScript a_or_b;
    a_or_b << OP_IF << key[1].GetPubKey() << OP_CHECKSIG << OP_ELSE
           << key[0].GetPubKey() << OP_CHECKSIG
           << OP_ENDIF;

    CScript a_and_b_or_c;
    a_and_b_or_c << OP_IF << key[2].GetPubKey() << OP_CHECKSIG << OP_ELSE
                 << key[1].GetPubKey() << OP_CHECKSIGVERIFY << key[0].GetPubKey() << OP_CHECKSIG
                 << OP_ENDIF;

    CTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = a_and_b_or_c;

    CTransaction txTo[3]; // Spending transaction
    for (int i = 0; i < 3; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
    }

    for (int i = 0; i < 3; i++)
    {
        BOOST_CHECK_MESSAGE(SignSignature(keystore, txFrom, txTo[i], 0), strprintf("SignSignature %d", i));
    }
}


BOOST_AUTO_TEST_SUITE_END()
