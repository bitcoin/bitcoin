// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Tests for the P2SKH (Pay-to-Schnorr-Key-Hash) scheme.
//
// Locking script:  OP_2 <hash160(P.x)>
// Signature:       64-byte Schnorr   S = k + e*d,  e = TaggedHash("P2SKH/challenge", R.x || h160 || msg)
// Verification:    key-recovery      P = e^-1 * (S*G - R),  check hash160(P.x) == program
//

#include <addresstype.h>
#include <hash.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <array>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(script_p2skh_tests, BasicTestingSetup)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Build a "crediting" transaction that pays to the given scriptPubKey. */
static CMutableTransaction MakeCreditingTx(const CScript& scriptPubKey, CAmount nValue = 1 * COIN)
{
    CMutableTransaction tx;
    tx.version = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0; // coinbase-style
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey = scriptPubKey;
    tx.vout[0].nValue = nValue;
    return tx;
}

/** Build a spending transaction that spends output 0 of fundingTx. */
static CMutableTransaction MakeSpendingTx(const CMutableTransaction& fundingTx)
{
    CMutableTransaction tx;
    tx.version = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = fundingTx.GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    tx.vout[0].nValue = fundingTx.vout[0].nValue - 1000; // leave some for fees
    return tx;
}

/**
 * Sign and verify a P2SKH spend in one go.
 *
 * @param key          Spending key
 * @param scriptPubKey OP_2 <hash160(key.x)>
 * @param flags        Script verify flags (should include SCRIPT_VERIFY_P2SKH)
 * @param expect_valid Whether the verification is expected to succeed
 * @param tamper       Optional lambda to mess with the spending tx before verification
 */
static bool RunV2Verify(
    const CKey& key,
    const CScript& scriptPubKey,
    script_verify_flags flags,
    bool expect_valid,
    std::function<void(CMutableTransaction&, std::vector<unsigned char>&)> tamper = nullptr)
{
    // 1. Build funding + spending transactions.
    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    // 2. Precompute transaction data (needs spent outputs for BIP341 sighash).
    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    // 3. Compute the sighash.
    //    P2SKH reuses the BIP341 (SigVersion::TAPROOT) sighash with SIGHASH_DEFAULT.
    ScriptExecutionData execdata;
    execdata.m_annex_present = false;
    execdata.m_annex_init = true;
    uint256 sighash;
    BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, spendTx, /*in_pos=*/0,
                                       SIGHASH_DEFAULT, SigVersion::TAPROOT,
                                       txdata, MissingDataBehavior::ASSERT_FAIL));

    // 4. Sign.
    uint256 aux;    // all-zero auxiliary randomness is fine for tests
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignP2SKH(sighash, sig, aux));

    // 5. Allow tests to tamper with either the tx or the signature.
    if (tamper) tamper(spendTx, sig);

    // 6. Put the signature into the witness stack.
    spendTx.vin[0].scriptWitness.stack = {sig};

    // 7. Verify.
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        /*scriptSig=*/CScript(),
        scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);

    BOOST_CHECK_EQUAL(ok, expect_valid);
    return ok;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

/** Basic happy path: valid key, valid signature. */
BOOST_AUTO_TEST_CASE(valid_spend)
{
    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    RunV2Verify(key, scriptPubKey, flags, /*expect_valid=*/true);
}

/** P2SKH flag absent → treated as unknown witness version, passes (soft-fork safety). */
BOOST_AUTO_TEST_CASE(flag_absent_passes)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    // Without SCRIPT_VERIFY_P2SKH the program is treated as an unknown
    // future witness version and is accepted unconditionally (soft-fork upgrade).
    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS; // no P2SKH
    RunV2Verify(key, scriptPubKey, flags, /*expect_valid=*/true);
}

/** Tampered signature (flip a byte) must fail. */
BOOST_AUTO_TEST_CASE(bad_signature)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    RunV2Verify(key, scriptPubKey, flags, /*expect_valid=*/false,
        [](CMutableTransaction&, std::vector<unsigned char>& sig) {
            sig[0] ^= 0xff; // corrupt first byte
        });
}

/** Signature from a completely different key must fail. */
BOOST_AUTO_TEST_CASE(wrong_key)
{
    CKey rightKey, wrongKey;
    rightKey.MakeNewKey(true);
    wrongKey.MakeNewKey(true);

    // Lock to rightKey but sign with wrongKey.
    XOnlyPubKey xpk{rightKey.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    // Build funding + spending transactions.
    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    ScriptExecutionData execdata;
    execdata.m_annex_present = false;
    execdata.m_annex_init = true;
    uint256 sighash;
    BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, spendTx, 0,
                                       SIGHASH_DEFAULT, SigVersion::TAPROOT,
                                       txdata, MissingDataBehavior::ASSERT_FAIL));

    uint256 aux;
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(wrongKey.SignP2SKH(sighash, sig, aux));

    spendTx.vin[0].scriptWitness.stack = {sig};

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        CScript(), scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);
    BOOST_CHECK(!ok);
}

/** Empty witness stack must fail. */
BOOST_AUTO_TEST_CASE(empty_witness)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    // Leave witness stack empty.
    spendTx.vin[0].scriptWitness.stack.clear();

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        CScript(), scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
}

/** Two stack items instead of one must fail (no script-path for V2). */
BOOST_AUTO_TEST_CASE(too_many_stack_items)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    ScriptExecutionData execdata;
    execdata.m_annex_present = false;
    execdata.m_annex_init = true;
    uint256 sighash;
    BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, spendTx, 0,
                                       SIGHASH_DEFAULT, SigVersion::TAPROOT,
                                       txdata, MissingDataBehavior::ASSERT_FAIL));

    uint256 aux;
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignP2SKH(sighash, sig, aux));

    // Push an extra dummy item to make it two items.
    spendTx.vin[0].scriptWitness.stack = {sig, {0x42}};

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        CScript(), scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
}

/** Solver correctly identifies OP_2 + 20-byte program as SCHNORR_KEYHASH. */
BOOST_AUTO_TEST_CASE(solver_identifies_type)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    std::vector<std::vector<unsigned char>> solutions;
    TxoutType type = Solver(scriptPubKey, solutions);
    BOOST_CHECK_EQUAL(static_cast<int>(type), static_cast<int>(TxoutType::SCHNORR_KEYHASH));
    BOOST_REQUIRE_EQUAL(solutions.size(), 1U);
    BOOST_CHECK_EQUAL(solutions[0].size(), 20U);
    BOOST_CHECK(std::equal(h160.begin(), h160.end(), solutions[0].begin()));
}

/** ExtractDestination returns a SchnorrKeyHash for OP_2 + 20-byte program. */
BOOST_AUTO_TEST_CASE(extract_destination)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    CTxDestination dest;
    bool ok = ExtractDestination(scriptPubKey, dest);
    BOOST_CHECK(ok);
    BOOST_CHECK(std::holds_alternative<SchnorrKeyHash>(dest));

    SchnorrKeyHash kh = std::get<SchnorrKeyHash>(dest);
    BOOST_CHECK(std::equal(h160.begin(), h160.end(), kh.begin()));
}

/** GetScriptForDestination round-trips through SchnorrKeyHash. */
BOOST_AUTO_TEST_CASE(get_script_for_destination)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    SchnorrKeyHash kh{uint160{std::vector<unsigned char>(h160.begin(), h160.end())}};
    CScript scriptPubKey = GetScriptForDestination(kh);

    // Must be OP_2 + push of 20 bytes.
    int version;
    std::vector<unsigned char> program;
    BOOST_REQUIRE(scriptPubKey.IsWitnessProgram(version, program));
    BOOST_CHECK_EQUAL(version, 2);
    BOOST_CHECK_EQUAL(program.size(), 20U);
    BOOST_CHECK(std::equal(h160.begin(), h160.end(), program.begin()));
}

/** A valid spend with SIGHASH_ALL (65-byte signature) also works. */
BOOST_AUTO_TEST_CASE(valid_spend_sighash_all)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    ScriptExecutionData execdata;
    execdata.m_annex_present = false;
    execdata.m_annex_init = true;
    uint256 sighash;
    BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, spendTx, 0,
                                       SIGHASH_ALL, SigVersion::TAPROOT,
                                       txdata, MissingDataBehavior::ASSERT_FAIL));

    uint256 aux;
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignP2SKH(sighash, sig, aux));

    // Append sighash type byte to make it 65 bytes.
    sig.push_back(SIGHASH_ALL);

    spendTx.vin[0].scriptWitness.stack = {sig};

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        CScript(), scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);
    BOOST_CHECK(ok);
}

/** Signature with wrong sighash type appended must fail. */
BOOST_AUTO_TEST_CASE(bad_sighash_type)
{
    CKey key;
    key.MakeNewKey(true);

    XOnlyPubKey xpk{key.GetPubKey()};
    CKeyID h160 = xpk.GetHash160();

    CScript scriptPubKey;
    scriptPubKey << OP_2 << std::vector<unsigned char>(h160.begin(), h160.end());

    CMutableTransaction fundTx = MakeCreditingTx(scriptPubKey);
    CMutableTransaction spendTx = MakeSpendingTx(fundTx);

    std::vector<CTxOut> spent_outputs{fundTx.vout[0]};
    PrecomputedTransactionData txdata;
    txdata.Init(spendTx, std::move(spent_outputs), /*force=*/true);

    ScriptExecutionData execdata;
    execdata.m_annex_present = false;
    execdata.m_annex_init = true;
    uint256 sighash;
    BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, spendTx, 0,
                                       SIGHASH_DEFAULT, SigVersion::TAPROOT,
                                       txdata, MissingDataBehavior::ASSERT_FAIL));

    uint256 aux;
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignP2SKH(sighash, sig, aux));

    // Append an invalid sighash type byte (0x05 is reserved/invalid).
    sig.push_back(0x05);

    spendTx.vin[0].scriptWitness.stack = {sig};

    script_verify_flags flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SKH;
    ScriptError serror = SCRIPT_ERR_OK;
    bool ok = VerifyScript(
        CScript(), scriptPubKey,
        &spendTx.vin[0].scriptWitness,
        flags,
        MutableTransactionSignatureChecker(&spendTx, 0, fundTx.vout[0].nValue, txdata,
                                            MissingDataBehavior::ASSERT_FAIL),
        &serror);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
}

BOOST_AUTO_TEST_SUITE_END()
