#include "../headers.h"

using namespace std;
typedef vector<unsigned char> valtype;
extern bool Solver(const CScript& scriptPubKey, vector<pair<opcodetype, valtype> >& vSolutionRet);
extern uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, int nHashType);

BOOST_AUTO_TEST_SUITE(script_tests)

BOOST_AUTO_TEST_CASE(Solver_standard)
{
    vector<pair<opcodetype, valtype> > vSolution;

    CKey key;
    key.MakeNewKey();

    CScript script;
    script << key.GetPubKey() << OP_CHECKSIG;

    BOOST_CHECK(Solver(script, vSolution));
    BOOST_CHECK(vSolution.size() == 1);
    BOOST_CHECK(vSolution[0].first == OP_PUBKEY);
    BOOST_CHECK(vSolution[0].second == key.GetPubKey());

    uint160 hash160 = Hash160(key.GetPubKey());
    valtype vchHash(hash160.begin(), hash160.end());
    script = CScript() << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;

    BOOST_CHECK(Solver(script, vSolution));
    BOOST_CHECK(vSolution.size() == 1);
    BOOST_CHECK(vSolution[0].first == OP_PUBKEYHASH);
    BOOST_CHECK(vSolution[0].second == vchHash);
}

BOOST_AUTO_TEST_CASE(solver_other)
{
    vector<pair<opcodetype, valtype> > vSolution;

    CKey key;
    key.MakeNewKey();

    CScript script;
    script << key.GetPubKey();
    BOOST_CHECK(!Solver(script, vSolution));

    script = CScript();
    script << key.GetPubKey() << OP_0;
    BOOST_CHECK(!Solver(script, vSolution));

    script = CScript();
    script << key.GetPubKey() << OP_CHECKSIG << OP_0;
    BOOST_CHECK(!Solver(script, vSolution));
}

// TODO other SIGHASH variants
BOOST_AUTO_TEST_CASE(verifyScript)
{
    CKey key;
    key.MakeNewKey();

    CScript scriptPubKey = CScript() << key.GetPubKey() << OP_CHECKSIG;

    CTransaction txFrom;
    txFrom.vout.resize(1);
    txFrom.vout[0].scriptPubKey = scriptPubKey;

    CTransaction txTo;
    txTo.vin.resize(2);
    txTo.vout.resize(1);
    txTo.vout[0].nValue = 1;
    txTo.vin[0].prevout.n = 0;
    txTo.vin[0].prevout.hash = txFrom.GetHash();

    uint256 hash = SignatureHash(scriptPubKey, txTo, 0, SIGHASH_ALL);

    vector<unsigned char> vchSig;
    BOOST_CHECK(CKey::Sign(key.GetPrivKey(), hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    CScript scriptSig = CScript() << vchSig;

    BOOST_CHECK(VerifyScript(scriptSig, scriptPubKey, txTo, 0, 0));

    // Should fail if output is changed
    txTo.vout[0].nValue = 2;
    BOOST_CHECK(!VerifyScript(scriptSig, scriptPubKey, txTo, 0, 0));

    txTo.vout[0].nValue = 1;

    // Should succeed since other input scriptPubKey is not hashed
    txTo.vin[1].scriptSig << OP_1;
    BOOST_CHECK(VerifyScript(scriptSig, scriptPubKey, txTo, 0, 0));

    txTo.vin[0].scriptSig = scriptSig;

    BOOST_CHECK(VerifySignature(txFrom, txTo, 0, SIGHASH_ALL));
}

BOOST_AUTO_TEST_SUITE_END()
