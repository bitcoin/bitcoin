// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <key.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <vector>

#include <boost/test/unit_test.hpp>

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s.begin(), s.end());
    return sSerialized;
}

BOOST_FIXTURE_TEST_SUITE(sigopcount_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(GetSigOpCount)
{
    // Test CScript::GetSigOpCount()
    CScript s1;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(false), 0U);
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 0U);

    uint160 dummy;
    s1 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 2U);
    s1 << OP_IF << OP_CHECKSIG << OP_ENDIF;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 3U);
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(false), 21U);

    CScript p2sh = GetScriptForDestination(ScriptHash(s1));
    CScript scriptSig;
    scriptSig << OP_0 << Serialize(s1);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig), 3U);

    std::vector<CPubKey> keys;
    for (int i = 0; i < 3; i++)
    {
        CKey k = GenerateRandomKey();
        keys.push_back(k.GetPubKey());
    }
    CScript s2 = GetScriptForMultisig(1, keys);
    BOOST_CHECK_EQUAL(s2.GetSigOpCount(true), 3U);
    BOOST_CHECK_EQUAL(s2.GetSigOpCount(false), 20U);

    p2sh = GetScriptForDestination(ScriptHash(s2));
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(true), 0U);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(false), 0U);
    CScript scriptSig2;
    scriptSig2 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << Serialize(s2);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig2), 3U);
}

/**
 * Verifies script execution of the zeroth scriptPubKey of tx output and
 * zeroth scriptSig and witness of tx input.
 */
static ScriptError VerifyWithFlag(const CTransaction& output, const CMutableTransaction& input, script_verify_flags flags)
{
    ScriptError error;
    CTransaction inputi(input);
    bool ret = VerifyScript(inputi.vin[0].scriptSig, output.vout[0].scriptPubKey, &inputi.vin[0].scriptWitness, flags, TransactionSignatureChecker(&inputi, 0, output.vout[0].nValue, MissingDataBehavior::ASSERT_FAIL), &error);
    BOOST_CHECK((ret == true) == (error == SCRIPT_ERR_OK));

    return error;
}

/**
 * Builds a creationTx from scriptPubKey and a spendingTx from scriptSig
 * and witness such that spendingTx spends output zero of creationTx.
 * Also inserts creationTx's output into the coins view.
 */
static void BuildTxs(CMutableTransaction& spendingTx, CCoinsViewCache& coins, CMutableTransaction& creationTx, const CScript& scriptPubKey, const CScript& scriptSig, const CScriptWitness& witness)
{
    creationTx.version = 1;
    creationTx.vin = {
        CTxIn{COutPoint{}},
    };
    creationTx.vout = {
        CTxOut{1, scriptPubKey},
    };

    spendingTx.version = 1;
    spendingTx.vin = {
        CTxIn{COutPoint{creationTx.GetHash(), 0}, scriptSig},
    };
    spendingTx.vin[0].scriptWitness = witness;
    spendingTx.vout = {
        CTxOut{1, CScript{}},
    };

    AddCoins(coins, CTransaction(creationTx), 0);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost)
{
    // Transaction creates outputs
    CMutableTransaction creationTx;
    // Transaction that spends outputs and whose
    // sig op cost is going to be tested
    CMutableTransaction spendingTx;

    // Create utxo set
    CCoinsViewCache coins{&CoinsViewEmpty::Get()};
    // Create key
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();
    // Default flags
    const script_verify_flags flags{SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH};

    // Multisig script (legacy counting)
    {
        CScript scriptPubKey = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
        // Do not use a valid signature to avoid using wallet operations.
        CScript scriptSig = CScript() << OP_0 << OP_0;

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, CScriptWitness());
        // Legacy counting only includes signature operations in scriptSigs and scriptPubKeys
        // of a transaction and does not take the actual executed sig operations into account.
        // spendingTx in itself does not contain a signature operation.
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 0);
        // creationTx contains two signature operations in its scriptPubKey, but legacy counting
        // is not accurate.
        assert(GetTransactionSigOpCost(CTransaction(creationTx), coins, flags) == MAX_PUBKEYS_PER_MULTISIG * WITNESS_SCALE_FACTOR);
        // Sanity check: script verification fails because of an invalid signature.
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_CHECKMULTISIGVERIFY);
    }

    // Multisig nested in P2SH
    {
        CScript redeemScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
        CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        CScript scriptSig = CScript() << OP_0 << OP_0 << ToByteVector(redeemScript);

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, CScriptWitness());
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 2 * WITNESS_SCALE_FACTOR);
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_CHECKMULTISIGVERIFY);

        // P2SH sigops are not counted if we don't set the SCRIPT_VERIFY_P2SH flag
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, /*flags=*/0) == 0);
    }

    // P2WPKH witness program
    {
        CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkey));
        CScript scriptSig = CScript();
        CScriptWitness scriptWitness;
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(0);


        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 1);
        // No signature operations if we don't verify the witness.
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags & ~SCRIPT_VERIFY_WITNESS) == 0);
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_EQUALVERIFY);

        // The sig op cost for witness version != 0 is zero.
        assert(scriptPubKey[0] == 0x00);
        scriptPubKey[0] = 0x51;
        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 0);
        scriptPubKey[0] = 0x00;
        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);

        // The witness of a coinbase transaction is not taken into account.
        spendingTx.vin[0].prevout.SetNull();
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 0);
    }

    // P2WPKH nested in P2SH
    {
        CScript scriptSig = GetScriptForDestination(WitnessV0KeyHash(pubkey));
        CScript scriptPubKey = GetScriptForDestination(ScriptHash(scriptSig));
        scriptSig = CScript() << ToByteVector(scriptSig);
        CScriptWitness scriptWitness;
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(0);

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 1);
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_EQUALVERIFY);
    }

    // P2WSH witness program
    {
        CScript witnessScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
        CScript scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
        CScript scriptSig = CScript();
        CScriptWitness scriptWitness;
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(witnessScript.begin(), witnessScript.end());

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 2);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags & ~SCRIPT_VERIFY_WITNESS) == 0);
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_CHECKMULTISIGVERIFY);
    }

    // P2WSH nested in P2SH
    {
        CScript witnessScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
        CScript redeemScript = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
        CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        CScript scriptSig = CScript() << ToByteVector(redeemScript);
        CScriptWitness scriptWitness;
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(0);
        scriptWitness.stack.emplace_back(witnessScript.begin(), witnessScript.end());

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
        assert(GetTransactionSigOpCost(CTransaction(spendingTx), coins, flags) == 2);
        assert(VerifyWithFlag(CTransaction(creationTx), spendingTx, flags) == SCRIPT_ERR_CHECKMULTISIGVERIFY);
    }
}

/**
 * GetTransactionSigOpCost() accumulates legacy, P2SH and witness sigops. P2SH and
 * witness sigops both need the spent output of every input, and are computed in a
 * single pass. This test uses transactions that mix P2SH, non-P2SH and witness inputs
 * in several orders, so a mistake in which contribution is applied to which input
 * (or in the flag gating) changes the total.
 */
BOOST_AUTO_TEST_CASE(GetTxSigOpCost_mixed_inputs)
{
    CKey key{GenerateRandomKey()};
    const CPubKey pubkey{key.GetPubKey()};

    // A P2SH output whose redeem script is a 2-of-2 multisig, so it contributes
    // MAX_PUBKEYS_PER_MULTISIG sigops through GetSigOpCount(scriptSig).
    const CScript redeem_script{CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY};
    const CScript p2sh_spk{GetScriptForDestination(ScriptHash(redeem_script))};
    const CScript p2sh_sig{CScript() << OP_0 << OP_0 << ToByteVector(redeem_script)};

    // A plain P2PKH output: neither P2SH nor a witness program, so it contributes nothing
    // to either count even though it is spent.
    const CScript p2pkh_spk{GetScriptForDestination(PKHash(pubkey))};
    const CScript p2pkh_sig{CScript() << OP_0 << OP_0};

    // A P2WPKH output: contributes exactly one witness sigop.
    const CScript p2wpkh_spk{GetScriptForDestination(WitnessV0KeyHash(pubkey))};
    CScriptWitness witness;
    witness.stack.emplace_back(0);
    witness.stack.emplace_back(0);

    enum class Kind { P2SH, P2PKH, P2WPKH };

    // Expected contributions per input kind.
    const auto expect{[&](std::span<const Kind> kinds, script_verify_flags flags) {
        int64_t expected{0};
        for (const Kind kind : kinds) {
            switch (kind) {
            case Kind::P2SH:
                if (flags & SCRIPT_VERIFY_P2SH) expected += 2 * WITNESS_SCALE_FACTOR;
                break;
            case Kind::P2PKH:
                break;
            case Kind::P2WPKH:
                if (flags & SCRIPT_VERIFY_WITNESS) expected += 1;
                break;
            }
        }
        return expected;
    }};

    const script_verify_flags all_flags{SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS};

    std::vector<std::vector<Kind>> orders{
        {Kind::P2SH, Kind::P2PKH, Kind::P2WPKH},
        {Kind::P2WPKH, Kind::P2PKH, Kind::P2SH},
        {Kind::P2PKH, Kind::P2SH, Kind::P2WPKH},
        {Kind::P2SH},
        {Kind::P2WPKH},
        {Kind::P2PKH},
        {Kind::P2SH, Kind::P2SH},
        {Kind::P2SH, Kind::P2WPKH, Kind::P2SH, Kind::P2WPKH},
    };

    for (const auto& kinds : orders) {
        // Fresh coin view per order: outpoint hashes are derived from the outputs.
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction spending;
        spending.version = 1;

        for (const Kind kind : kinds) {
            CMutableTransaction creation;
            creation.version = 1;
            creation.vin = {CTxIn{COutPoint{}}};
            CTxIn input;
            switch (kind) {
            case Kind::P2SH:
                creation.vout = {CTxOut{1, p2sh_spk}};
                input = CTxIn{COutPoint{}, p2sh_sig};
                break;
            case Kind::P2PKH:
                creation.vout = {CTxOut{1, p2pkh_spk}};
                input = CTxIn{COutPoint{}, p2pkh_sig};
                break;
            case Kind::P2WPKH:
                creation.vout = {CTxOut{1, p2wpkh_spk}};
                input = CTxIn{COutPoint{}, CScript{}};
                input.scriptWitness = witness;
                break;
            }
            const CTransaction creation_tx{creation};
            // Distinguish the outpoints of identical outputs in the same order.
            input.prevout = COutPoint{creation_tx.GetHash(), 0};
            spending.vin.push_back(input);
            AddCoins(coins, creation_tx, 0);
        }
        spending.vout = {CTxOut{1, CScript{}}};

        const CTransaction spending_tx{spending};
        BOOST_CHECK_EQUAL(GetTransactionSigOpCost(spending_tx, coins, all_flags), expect(kinds, all_flags));
        // Note: WITNESS without P2SH is not a valid combination for this function
        // (CountWitnessSigOps asserts P2SH is set whenever WITNESS is).
        BOOST_CHECK_EQUAL(GetTransactionSigOpCost(spending_tx, coins, SCRIPT_VERIFY_P2SH), expect(kinds, SCRIPT_VERIFY_P2SH));
        BOOST_CHECK_EQUAL(GetTransactionSigOpCost(spending_tx, coins, /*flags=*/0), 0);
    }
}

/**
 * The legacy sigop count covers every scriptSig and scriptPubKey of the transaction and
 * is independent of the flags; the witness contribution is added per input. This pins the
 * relative weighting so a merge of the two passes cannot silently double-count.
 */
BOOST_AUTO_TEST_CASE(GetTxSigOpCost_legacy_and_witness_weighting)
{
    // Two multisig outputs: plain legacy sigops in the scriptPubKeys.
    CMutableTransaction mtx;
    mtx.version = 1;
    mtx.vin = {CTxIn{COutPoint{Txid::FromUint256(uint256::ONE), 0}, CScript{}, CTxIn::SEQUENCE_FINAL}};
    const CScript multisig{CScript() << 1 << std::vector<unsigned char>(33, 0x02) << std::vector<unsigned char>(33, 0x03) << 2 << OP_CHECKMULTISIG};
    mtx.vout = {CTxOut{1, multisig}, CTxOut{1, multisig}};

    CCoinsViewCache coins{&CoinsViewEmpty::Get()};

    // The prevout is not P2SH and not a witness program, so it adds nothing to either
    // the P2SH or the witness count. The cost must be exactly the legacy count scaled by
    // WITNESS_SCALE_FACTOR.
    CMutableTransaction creation;
    creation.version = 1;
    creation.vin = {CTxIn{COutPoint{}}};
    creation.vout = {CTxOut{1, CScript() << OP_TRUE}};
    const CTransaction creation_tx{creation};
    AddCoins(coins, creation_tx, 0);
    mtx.vin[0].prevout = COutPoint{creation_tx.GetHash(), 0};

    const CTransaction tx{mtx};
    const int64_t legacy{static_cast<int64_t>(GetLegacySigOpCount(tx)) * WITNESS_SCALE_FACTOR};
    BOOST_CHECK_GT(legacy, 0);

    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(tx, coins, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS), legacy);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(tx, coins, /*flags=*/0), legacy);

    // Coinbase transactions short-circuit after the legacy count, without touching the
    // coins view (an empty view would otherwise trip the !coin.IsSpent() assertion).
    CMutableTransaction coinbase{mtx};
    coinbase.vin[0].prevout.SetNull();
    const CCoinsViewCache empty_coins{&CoinsViewEmpty::Get()};
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction{coinbase}, empty_coins,
                                              SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS),
                      legacy);
}

BOOST_AUTO_TEST_SUITE_END()
