
#include <test/gen/transaction_gen.h>

#include <base58.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <string>
#include <test/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/Gen.h>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>

BOOST_FIXTURE_TEST_SUITE(transaction_properties, BasicTestingSetup)
/** Helper function to run a SpendingInfo through the interpreter to check
  * validity of the transaction spending a spk */
bool run(SpendingInfo info)
{
    const CTxOut output = std::get<0>(info);
    const CTransaction tx = std::get<1>(info);
    const int input_idx = std::get<2>(info);
    const CTxIn input = tx.vin[input_idx];
    const CScript scriptSig = input.scriptSig;
    TransactionSignatureChecker checker(&tx, input_idx, output.nValue);
    const CScriptWitness wit = input.scriptWitness;
    //run it through the interpreter
    bool result = VerifyScript(scriptSig, output.scriptPubKey,
        &wit, STANDARD_SCRIPT_VERIFY_FLAGS, checker);
    return result;
}
/** Check COutpoint serialization symmetry */
RC_BOOST_PROP(outpoint_serialization_symmetry, (const COutPoint& outpoint))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << outpoint;
    COutPoint outpoint2;
    ss >> outpoint2;
    RC_ASSERT(outpoint2 == outpoint);
}
/** Check CTxIn serialization symmetry */
RC_BOOST_PROP(ctxin_serialization_symmetry, (const CTxIn& txin))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << txin;
    CTxIn txin2;
    ss >> txin2;
    RC_ASSERT(txin == txin2);
}

/** Check CTxOut serialization symmetry */
RC_BOOST_PROP(ctxout_serialization_symmetry, (const CTxOut& txout))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << txout;
    CTxOut txout2;
    ss >> txout2;
    RC_ASSERT(txout == txout2);
}

/** Check CTransaction serialization symmetry */
RC_BOOST_PROP(ctransaction_serialization_symmetry, (const CTransaction& tx))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << tx;
    CTransaction tx2(deserialize, ss);
    RC_ASSERT(tx == tx2);
}

/** Check that we can spend a p2pk tx that ProduceSignature created */
RC_BOOST_PROP(spend_p2pk_tx, ())
{
    const SpendingInfo& info = *SignedP2PKTx();
    RC_ASSERT(run(info));
}

/** Check that we can spend a p2pkh tx that ProduceSignature created */
RC_BOOST_PROP(spend_p2pkh_tx, ())
{
    const SpendingInfo& info = *SignedP2PKHTx();
    RC_ASSERT(run(info));
}

/** Check that we can spend a multisig tx that ProduceSignature created */
RC_BOOST_PROP(spend_multisig_tx, ())
{
    const SpendingInfo& info = *SignedMultisigTx();
    RC_ASSERT(run(info));
}
/** Check that we can spend a p2sh tx that ProduceSignature created */
RC_BOOST_PROP(spend_p2sh_tx, ())
{
    const SpendingInfo& info = *SignedP2SHTx();
    RC_ASSERT(run(info));
}

RC_BOOST_PROP(spend_p2wpkh_tx, ())
{
    const SpendingInfo& info = *SignedP2WPKHTx();
    RC_ASSERT(run(info));
}

RC_BOOST_PROP(spend_p2wsh_tx, ())
{
    const SpendingInfo& info = *SignedP2WSHTx();
    RC_ASSERT(run(info));
}

BOOST_AUTO_TEST_SUITE_END()
