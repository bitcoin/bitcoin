

#include "key.h"
#include "keystore.h"
#include "main.h"
#include "script.h"
#include "uint256.h"

#include <boost/assign/std/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::assign;

typedef vector<unsigned char> valtype;

extern uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);

BOOST_AUTO_TEST_SUITE(multisig_tests)

CScript
sign_multisig(CScript scriptPubKey, vector<CKey> keys, CTransaction transaction, int whichIn)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL);

    CScript result;
    result << OP_0; // CHECKMULTISIG bug workaround
    BOOST_FOREACH(const CKey &key, keys)
    {
        vector<unsigned char> vchSig;
        BOOST_CHECK(key.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        result << vchSig;
    }
    return result;
}

BOOST_AUTO_TEST_CASE(multisig_verify)
{
    unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

    CKey key[4];
    for (int i = 0; i < 4; i++)
        key[i].MakeNewKey(true);

    CScript a_and_b;
    a_and_b << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;

    CScript a_or_b;
    a_or_b << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;

    CScript escrow;
    escrow << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << key[2].GetPubKey() << OP_3 << OP_CHECKMULTISIG;

    CTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = escrow;

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
    BOOST_CHECK(VerifyScript(s, a_and_b, txTo[0], 0, flags, 0));

    for (int i = 0; i < 4; i++)
    {
        keys.clear();
        keys += key[i];
        s = sign_multisig(a_and_b, keys, txTo[0], 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, txTo[0], 0, flags, 0), strprintf("a&b 1: %d", i));

        keys.clear();
        keys += key[1],key[i];
        s = sign_multisig(a_and_b, keys, txTo[0], 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, txTo[0], 0, flags, 0), strprintf("a&b 2: %d", i));
    }

    // Test a OR b:
    for (int i = 0; i < 4; i++)
    {
        keys.clear();
        keys += key[i];
        s = sign_multisig(a_or_b, keys, txTo[1], 0);
        if (i == 0 || i == 1)
            BOOST_CHECK_MESSAGE(VerifyScript(s, a_or_b, txTo[1], 0, flags, 0), strprintf("a|b: %d", i));
        else
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, txTo[1], 0, flags, 0), strprintf("a|b: %d", i));
    }
    s.clear();
    s << OP_0 << OP_0;
    BOOST_CHECK(!VerifyScript(s, a_or_b, txTo[1], 0, flags, 0));
    s.clear();
    s << OP_0 << OP_1;
    BOOST_CHECK(!VerifyScript(s, a_or_b, txTo[1], 0, flags, 0));


    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            keys.clear();
            keys += key[i],key[j];
            s = sign_multisig(escrow, keys, txTo[2], 0);
            if (i < j && i < 3 && j < 3)
                BOOST_CHECK_MESSAGE(VerifyScript(s, escrow, txTo[2], 0, flags, 0), strprintf("escrow 1: %d %d", i, j));
            else
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, txTo[2], 0, flags, 0), strprintf("escrow 2: %d %d", i, j));
        }
}

BOOST_AUTO_TEST_CASE(multisig_IsStandard)
{
    CKey key[4];
    for (int i = 0; i < 4; i++)
        key[i].MakeNewKey(true);

    txnouttype whichType;

    CScript a_and_b;
    a_and_b << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(::IsStandard(a_and_b, whichType));

    CScript a_or_b;
    a_or_b  << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(::IsStandard(a_or_b, whichType));

    CScript escrow;
    escrow << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << key[2].GetPubKey() << OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK(::IsStandard(escrow, whichType));

    CScript one_of_four;
    one_of_four << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << key[2].GetPubKey() << key[3].GetPubKey() << OP_4 << OP_CHECKMULTISIG;
    BOOST_CHECK(!::IsStandard(one_of_four, whichType));

    CScript malformed[6];
    malformed[0] << OP_3 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
    malformed[1] << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << OP_3 << OP_CHECKMULTISIG;
    malformed[2] << OP_0 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
    malformed[3] << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_0 << OP_CHECKMULTISIG;
    malformed[4] << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_CHECKMULTISIG;
    malformed[5] << OP_1 << key[0].GetPubKey() << key[1].GetPubKey();

    for (int i = 0; i < 6; i++)
        BOOST_CHECK(!::IsStandard(malformed[i], whichType));
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
    // one key that would satisfy an (a|b) or 2-of-3 keys needed
    // to spend an escrow transaction.
    //
    CBasicKeyStore keystore, emptykeystore, partialkeystore;
    CKey key[3];
    CTxDestination keyaddr[3];
    for (int i = 0; i < 3; i++)
    {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
        keyaddr[i] = key[i].GetPubKey().GetID();
    }
    partialkeystore.AddKey(key[0]);

    {
        vector<valtype> solutions;
        txnouttype whichType;
        CScript s;
        s << key[0].GetPubKey() << OP_CHECKSIG;
        BOOST_CHECK(Solver(s, whichType, solutions));
        BOOST_CHECK(solutions.size() == 1);
        CTxDestination addr;
        BOOST_CHECK(ExtractDestination(s, addr));
        BOOST_CHECK(addr == keyaddr[0]);
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
    }
    {
        vector<valtype> solutions;
        txnouttype whichType;
        CScript s;
        s << OP_DUP << OP_HASH160 << key[0].GetPubKey().GetID() << OP_EQUALVERIFY << OP_CHECKSIG;
        BOOST_CHECK(Solver(s, whichType, solutions));
        BOOST_CHECK(solutions.size() == 1);
        CTxDestination addr;
        BOOST_CHECK(ExtractDestination(s, addr));
        BOOST_CHECK(addr == keyaddr[0]);
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
    }
    {
        vector<valtype> solutions;
        txnouttype whichType;
        CScript s;
        s << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
        BOOST_CHECK(Solver(s, whichType, solutions));
        BOOST_CHECK_EQUAL(solutions.size(), 4U);
        CTxDestination addr;
        BOOST_CHECK(!ExtractDestination(s, addr));
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
        BOOST_CHECK(!IsMine(partialkeystore, s));
    }
    {
        vector<valtype> solutions;
        txnouttype whichType;
        CScript s;
        s << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;
        BOOST_CHECK(Solver(s, whichType, solutions));
        BOOST_CHECK_EQUAL(solutions.size(), 4U);
        vector<CTxDestination> addrs;
        int nRequired;
        BOOST_CHECK(ExtractDestinations(s, whichType, addrs, nRequired));
        BOOST_CHECK(addrs[0] == keyaddr[0]);
        BOOST_CHECK(addrs[1] == keyaddr[1]);
        BOOST_CHECK(nRequired == 1);
        BOOST_CHECK(IsMine(keystore, s));
        BOOST_CHECK(!IsMine(emptykeystore, s));
        BOOST_CHECK(!IsMine(partialkeystore, s));
    }
    {
        vector<valtype> solutions;
        txnouttype whichType;
        CScript s;
        s << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << key[2].GetPubKey() << OP_3 << OP_CHECKMULTISIG;
        BOOST_CHECK(Solver(s, whichType, solutions));
        BOOST_CHECK(solutions.size() == 5);
    }
}

BOOST_AUTO_TEST_CASE(multisig_Sign)
{
    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    CBasicKeyStore keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
    }

    CScript a_and_b;
    a_and_b << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;

    CScript a_or_b;
    a_or_b  << OP_1 << key[0].GetPubKey() << key[1].GetPubKey() << OP_2 << OP_CHECKMULTISIG;

    CScript escrow;
    escrow << OP_2 << key[0].GetPubKey() << key[1].GetPubKey() << key[2].GetPubKey() << OP_3 << OP_CHECKMULTISIG;

    CTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = escrow;

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
