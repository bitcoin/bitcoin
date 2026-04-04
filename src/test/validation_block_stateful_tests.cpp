// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <script/sign.h>
#include <test/util/block_validity.h>
#include <util/moneystr.h>

#include <boost/test/unit_test.hpp>

namespace {
//! Height offset for OP_CHECKLOCKTIMEVERIFY tests
static constexpr int CLTV_HEIGHT_OFFSET = 100;
//! Block offset for OP_CHECKSEQUENCEVERIFY tests
static constexpr uint32_t CSV_BLOCK_OFFSET = 10;
static void CheckBlockValid(const BlockValidationState& state)
{
    BOOST_CHECK_MESSAGE(state.IsValid(), strprintf("Expected block to be valid but got reject reason: %s", state.GetRejectReason()));
    BOOST_CHECK(state.GetResult() == BlockValidationResult::BLOCK_RESULT_UNSET);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "");
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), "");
}
static void CheckBlockInvalid(const BlockValidationState& state, BlockValidationResult expected_result, const std::string& expected_reason, const std::string& expected_debug_message)
{
    BOOST_CHECK_MESSAGE(state.IsInvalid(), "Expected block to be invalid but it is valid");
    BOOST_CHECK(state.GetResult() == expected_result);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), expected_reason);
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), expected_debug_message);
}
static void CheckScriptViolation(const BlockValidationState& state, const CBlock& block, const COutPoint& outpoint, const std::string& expected_reason)
{
    const auto debug = strprintf("input 0 of %s (wtxid %s), spending %s:%u",
                                 block.vtx.back()->GetHash().ToString(),
                                 block.vtx.back()->GetWitnessHash().ToString(),
                                 outpoint.hash.ToString(),
                                 outpoint.n);
    CheckBlockInvalid(state, BlockValidationResult::BLOCK_CONSENSUS, expected_reason, debug);
}
}

/*
 * Tests for stateful block validation rules (ConnectBlock/SpendBlock), which
 * require access to the UTXO set (chainstate) to verify inputs and scripts.
 */
BOOST_FIXTURE_TEST_SUITE(validation_block_stateful_tests, ValidationBlockValidityTestingSetup)

BOOST_AUTO_TEST_CASE(tbv_bad_txns_accumulated_fee_outofrange)
{
    // The sum of fees in a block must be within the valid range (0 to 21M).
    CBlock block = MakeBlock();
    const auto outpoint1 = AddCoin(CScript(), MAX_MONEY);
    const auto outpoint2 = AddCoin(CScript(), 1);
    if (!outpoint1 || !outpoint2) return;
    CMutableTransaction mtx, mtx2;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint1;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0;
    mtx2.vin.resize(1);
    mtx2.vin[0].prevout = *outpoint2;
    mtx2.vout.resize(1);
    mtx2.vout[0].nValue = 0;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.vtx.push_back(MakeTransactionRef(mtx2));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-accumulated-fee-outofrange";
    const auto debug = "accumulated fee in the block out of range";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_inputs_missingorspent)
{
    // Transactions must spend valid, unspent outputs currently in the UTXO set.
    CBlock block = MakeBlock();
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = COutPoint(Txid::FromUint256(uint256{123}), 0);
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-inputs-missingorspent";
    const auto debug = strprintf("CheckTxInputs: inputs missing/spent in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_inputvalues_outofrange)
{
    // The outputs being spent must have valid values (0 to 21M).
    CBlock block = MakeBlock();
    Txid txid = Txid::FromUint256(uint256{255});
    WITH_LOCK(cs_main, m_chainstate.CoinsTip().AddCoin(COutPoint(txid, 0), Coin(CTxOut(MAX_MONEY + 1, CScript()), 1, false), true));
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = COutPoint(txid, 0);
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-inputvalues-outofrange";
    const auto debug = strprintf(" in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_in_belowout)
{
    // Transactions cannot spend more than the total value of their inputs.
    CBlock block = MakeBlock();
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = COutPoint(m_coinbase_txns[0]->GetHash(), 0);
    mtx.vout.resize(1);
    const CAmount input_value = m_coinbase_txns[0]->vout[0].nValue;
    mtx.vout[0].nValue = input_value + 1;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-in-belowout";
    const auto debug = strprintf("value in (%s) < value out (%s) in transaction %s",
                                 FormatMoney(input_value),
                                 FormatMoney(mtx.vout[0].nValue),
                                 block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_inputs_sum_overflow)
{
    // The sum of input values in a transaction cannot exceed the maximum possible supply (overflow check).
    CBlock block = MakeBlock();
    CScript script = CScript() << OP_TRUE;
    auto outpoint1 = AddCoin(script, MAX_MONEY);
    auto outpoint2 = AddCoin(script, MAX_MONEY);
    if (!outpoint1.has_value() && !outpoint2.has_value()) return;
    CMutableTransaction mtx;
    mtx.vin.resize(2);
    mtx.vin[0].prevout = outpoint1.value();
    mtx.vin[1].prevout = outpoint2.value();
    mtx.vout.resize(1);
    mtx.vout[0].nValue = MAX_MONEY;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-inputvalues-outofrange";
    const auto debug = strprintf(" in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_block_script_verify_flag_failed)
{
    // Transactions must satisfy their input scripts. Providing an invalid scriptSig (e.g., OP_0) causes failure.
    CBlock block = MakeBlock();
    const CScript p2pk = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    const auto outpoint = AddCoin(p2pk, 50 * COIN);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].scriptSig = CScript() << OP_0;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 49 * COIN;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip147_null_dummy)
{
    // BIP147: The dummy argument to OP_CHECKMULTISIG must be null.
    CBlock block = MakeBlock();
    CScript multisig = CScript() << 1 << ToByteVector(coinbaseKey.GetPubKey()) << 1 << OP_CHECKMULTISIG;
    const auto outpoint = AddCoin(multisig);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    // Non-null dummy (OP_1) should trigger failure.
    std::vector<unsigned char> sig;
    uint256 hash = SignatureHash(multisig, mtx, 0, SIGHASH_ALL, 1 * COIN, SigVersion::BASE);
    BOOST_REQUIRE(coinbaseKey.Sign(hash, sig));
    sig.push_back(SIGHASH_ALL);
    mtx.vin[0].scriptSig = CScript() << OP_1 << sig;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Dummy CHECKMULTISIG argument must be zero)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip66_non_der_sig)
{
    // BIP66: Signatures must follow strict DER encoding.
    CBlock block = MakeBlock();
    CScript p2pkh = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const auto outpoint = AddCoin(p2pkh);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    uint256 hash = SignatureHash(p2pkh, mtx, 0, SIGHASH_ALL, 1 * COIN, SigVersion::BASE);
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(coinbaseKey.Sign(hash, sig));
    sig.push_back(SIGHASH_ALL);
    // Append a garbage byte to violate DER encoding
    sig.push_back(0);
    mtx.vin[0].scriptSig = CScript() << sig << ToByteVector(coinbaseKey.GetPubKey());
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Non-canonical DER signature)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip65_cltv_violation)
{
    // BIP65: OP_CHECKLOCKTIMEVERIFY fails when the transaction's sequence is SEQUENCE_FINAL,
    // because a finalised sequence disables the lock regardless of nLockTime.
    CBlock block = MakeBlock();
    const int lock_height = WITH_LOCK(cs_main, return m_chainstate.m_chain.Height()) + CLTV_HEIGHT_OFFSET;
    const CScript cltv_script = CScript() << lock_height << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_TRUE;
    const auto outpoint = AddCoin(cltv_script);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL; // Makes IsFinalTx pass, but disables CLTV
    mtx.nLockTime = 0;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Locktime requirement not satisfied)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_taproot_invalid_sig)
{
    // Taproot (BIP341/342) represents the latest set of softforks.
    // If we can spend a Taproot output, it confirms the deployment is active.
    CBlock block = MakeBlock();
    XOnlyPubKey internal_key(coinbaseKey.GetPubKey());
    TaprootBuilder builder;
    builder.Finalize(internal_key);
    WitnessV1Taproot taproot = builder.GetOutput();
    CScript p2tr_script = GetScriptForDestination(taproot);
    const auto outpoint = AddCoin(p2tr_script);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    // Create the Taproot signature (Keypath spend)
    FlatSigningProvider provider;
    provider.keys[coinbaseKey.GetPubKey().GetID()] = coinbaseKey;
    provider.tr_trees[taproot] = builder;
    std::map<COutPoint, Coin> input_coins;
    input_coins.insert({mtx.vin[0].prevout, Coin(CTxOut(1 * COIN, p2tr_script), 1, false)});
    std::map<int, bilingual_str> input_errors;
    BOOST_REQUIRE(SignTransaction(mtx, &provider, input_coins, SIGHASH_DEFAULT, input_errors));
    // Mangle the signature to make it invalid
    mtx.vin[0].scriptWitness.stack[0][0] ^= 1;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Invalid Schnorr signature)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip112_csv_violation)
{
    // BIP112: OP_CHECKSEQUENCEVERIFY fails when the input's nSequence does not satisfy
    // the relative lock time encoded in the script.
    CBlock block = MakeBlock();
    const CScript csv_script = CScript() << CSV_BLOCK_OFFSET << OP_CHECKSEQUENCEVERIFY << OP_DROP << OP_TRUE;
    const auto outpoint = AddCoin(csv_script);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.version = 2; // CSV requires nVersion >= 2
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].nSequence = CSV_BLOCK_OFFSET - 1; // One block short of the requirement
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Locktime requirement not satisfied)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip16_p2sh_invalid_redeem_script)
{
    // BIP16: For P2SH outputs the pushed redeem script must evaluate to true.
    // Pushing a script that evaluates to false (OP_FALSE) causes rejection.
    CBlock block = MakeBlock();
    const CScript redeem_script = CScript() << OP_FALSE;
    const CScript p2sh = GetScriptForDestination(ScriptHash(redeem_script));
    const auto outpoint = AddCoin(p2sh);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(redeem_script.begin(), redeem_script.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_segwit_v0_invalid_witness_script)
{
    // BIP141: A P2WSH output whose witness script evaluates to false is rejected.
    CBlock block = MakeBlock();
    const CScript witness_script = CScript() << OP_FALSE;
    const CScript p2wsh = GetScriptForDestination(WitnessV0ScriptHash(witness_script));
    const auto outpoint = AddCoin(p2wsh);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    // Witness: <witness_script>
    mtx.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(witness_script.begin(), witness_script.end())};
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    block.vtx.push_back(MakeTransactionRef(mtx));
    m_node.chainman->GenerateCoinbaseCommitment(
        block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_tapscript_invalid_script_path)
{
    // BIP342: A tapscript script-path spend is rejected if the leaf script fails.
    // We commit to a leaf script of OP_FALSE and attempt to spend it.
    CBlock block = MakeBlock();
    XOnlyPubKey internal_key(coinbaseKey.GetPubKey());
    const CScript leaf_script = CScript() << OP_FALSE;
    TaprootBuilder builder;
    builder.Add(0, leaf_script, TAPROOT_LEAF_TAPSCRIPT);
    builder.Finalize(internal_key);
    WitnessV1Taproot taproot = builder.GetOutput();
    const CScript p2tr_script = GetScriptForDestination(taproot);
    const auto outpoint = AddCoin(p2tr_script);
    if (!outpoint) return;
    // Build the control block for the script path
    const std::vector<unsigned char> control_block = *builder.GetSpendData().scripts.at({std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()), TAPROOT_LEAF_TAPSCRIPT}).begin();
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    // Witness stack for script path: <leaf_script> <control_block> (OP_FALSE consumes no args)
    mtx.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        control_block};
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    block.vtx.push_back(MakeTransactionRef(mtx));
    m_node.chainman->GenerateCoinbaseCommitment(
        block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bip143_wrong_amount_in_sighash)
{
    // BIP143: The SegWit v0 sighash commits to the value of the input being spent.
    // Signing over a different amount than what the UTXO holds produces an invalid
    // signature, which cannot be detected without the committed-amount check.
    CBlock block = MakeBlock();
    const CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()));
    const CAmount actual_value = 1 * COIN;
    const auto outpoint = AddCoin(p2wpkh, actual_value);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0.9 * COIN;
    // Sign over a wrong amount — this is the BIP143-specific failure
    const CAmount wrong_value = actual_value + 1;
    const CScript p2wpkh_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    uint256 hash = SignatureHash(p2wpkh_script, mtx, 0, SIGHASH_ALL, wrong_value, SigVersion::WITNESS_V0);
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(coinbaseKey.Sign(hash, sig));
    sig.push_back(SIGHASH_ALL);
    mtx.vin[0].scriptWitness.stack = {sig, ToByteVector(coinbaseKey.GetPubKey())};
    block.vtx.push_back(MakeTransactionRef(mtx));
    m_node.chainman->GenerateCoinbaseCommitment(
        block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "block-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)";
    CheckScriptViolation(TestValidity(block), block, *outpoint, reason);
}

BOOST_AUTO_TEST_CASE(tbv_empty_scriptsig)
{
    // A non-coinbase transaction with an empty scriptSig spending OP_TRUE is valid.
    CBlock block = MakeBlock();
    const auto outpoint = AddCoin(CScript() << OP_TRUE);
    if (!outpoint) return;

    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].scriptSig = CScript(); // Empty
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    CheckBlockValid(TestValidity(block));
}

BOOST_AUTO_TEST_CASE(tbv_scriptsig_non_push)
{
    // A scriptSig with non-push opcodes is valid in a block (consensus) even if non-standard.
    CBlock block = MakeBlock();
    const auto outpoint = AddCoin(CScript() << OP_TRUE);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].scriptSig = CScript() << OP_1 << OP_DROP;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    CheckBlockValid(TestValidity(block));
}

BOOST_AUTO_TEST_CASE(tbv_double_spend_same_block)
{
    // A block containing two transactions spending the same UTXO is invalid.
    CBlock block = MakeBlock();
    const auto outpoint = AddCoin();
    if (!outpoint) return;
    for (int tx_idx = 0; tx_idx < 2; ++tx_idx) {
        CMutableTransaction mtx;
        mtx.vin.resize(1);
        mtx.vin[0].prevout = *outpoint;
        mtx.vin[0].scriptSig = CScript() << OP_TRUE;
        mtx.vout.resize(1);
        mtx.vout[0].nValue = 1;
        block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    }
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-inputs-missingorspent";
    const auto debug = strprintf("CheckTxInputs: inputs missing/spent in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_zero_value_output)
{
    // A transaction with a zero-value output is consensus-valid.
    CBlock block = MakeBlock();
    const auto outpoint = AddCoin();
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vin[0].scriptSig = CScript() << OP_TRUE;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 0;
    mtx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    CheckBlockValid(TestValidity(block));
}

BOOST_AUTO_TEST_SUITE_END()
