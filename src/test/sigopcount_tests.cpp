// Copyright (c) 2012-2022 The Bitcoin Core developers
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

using namespace util::hex_literals;

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s.begin(), s.end());
    return sSerialized;
}

BOOST_FIXTURE_TEST_SUITE(sigopcount_tests, BasicTestingSetup)

// feeds malformed PUSHDATA sequences to confirm the parser never crashes and still counts sig‑ops that appear before the error.
BOOST_AUTO_TEST_CASE(GetLegacySigOpCountErrors)
{
    // bounds check with OP_CHECKSIG prefix
    for (const bool fAccurate : {false, true}) {
        // push-75 with zero bytes present
        {
            auto raw{"ac4b"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // push-2 with only 1 byte present
        {
            auto raw{"ac02aa"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // out of bounds OP_PUSHDATA1 data size
        {
            auto raw{"ac4c"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // not enough data after OP_PUSHDATA2
        {
            auto raw{"ac4d"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // not enough data after OP_PUSHDATA2
        {
            auto raw{"ac4dac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // out of bounds OP_PUSHDATA2 data size
        {
            auto raw{"ac4dfeffffffac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // not enough data after OP_PUSHDATA4
        {
            auto raw{"ac4e"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // not enough data after OP_PUSHDATA4
        {
            auto raw{"ac4eac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // not enough data after OP_PUSHDATA4
        {
            auto raw{"ac4eacac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // not enough data after OP_PUSHDATA4
        {
            auto raw{"ac4eacacac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
        // out of bounds OP_PUSHDATA4 data size
        {
            auto raw{"ac4efeffffffac"_hex_v_u8};
            CScript script{raw.begin(), raw.end()};
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
    }
}

// asserts the expected legacy/accurate sig‑op totals for all common known script templates (P2PKH, P2SH, P2WPKH/WSH, P2TR, compressed & uncompressed P2PK, OP_RETURN, multisig).
BOOST_AUTO_TEST_CASE(GetLegacySigOpCountKnownTemplates)
{
    CKey dummyKey;
    dummyKey.MakeNewKey(true);
    CPubKey pubkey = dummyKey.GetPubKey();

    for (const bool fAccurate : {false, true}) {
        // OP_RETURN with sigops after (non-standard, found first in block 229,712)
        {
            auto raw{"6a76a914cd2b3298b7f455f39805377e5f213093df3cc09a88ac"_hex_v_u8};
            const CScript script{raw.begin(), raw.end()};
            BOOST_REQUIRE_EQUAL(script.front(), OP_RETURN);
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // P2WPKH
        {
            const auto script{GetScriptForDestination(WitnessV0KeyHash(pubkey.GetID()))};
            BOOST_REQUIRE(script.IsPayToWitnessPubKeyHash());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 0);
        }

        // P2SH
        {
            const auto script{GetScriptForDestination(ScriptHash(CScript() << OP_TRUE))};
            BOOST_REQUIRE(script.IsPayToScriptHash());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 0);
        }

        // P2PKH
        {
            const auto script{GetScriptForDestination(PKHash(pubkey.GetID()))};
            BOOST_REQUIRE(script.IsPayToPubKeyHash());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // P2WSH
        {
            const auto script{GetScriptForDestination(WitnessV0ScriptHash(CScript() << OP_TRUE))};
            BOOST_REQUIRE(script.IsPayToWitnessScriptHash());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 0);
        }

        // P2TR
        {
            const auto script{GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(pubkey)))};
            BOOST_REQUIRE(script.IsPayToTaproot());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 0);
        }

        // P2PK (compressed)
        {
            const auto script{GetScriptForRawPubKey(pubkey)};
            BOOST_REQUIRE(script.IsCompressedPayToPubKey());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }

        // P2PK (uncompressed)
        {
            CKey uncompressedKey;
            uncompressedKey.MakeNewKey(false);
            const auto script{GetScriptForRawPubKey(uncompressedKey.GetPubKey())};
            BOOST_REQUIRE(script.IsUncompressedPayToPubKey());
            BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(fAccurate), 1);
        }
    }

    // MULTISIG
    {
        std::vector<CPubKey> keys;
        keys.push_back(pubkey);
        keys.push_back(pubkey); // Using the same key twice for simplicity
        const auto script{GetScriptForMultisig(1, keys)};
        BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(/*fAccurate=*/false), 20); // Default max pubkeys
        BOOST_CHECK_EQUAL(script.GetLegacySigOpCount(/*fAccurate=*/true), 2);   // Actual count
    }
}

BOOST_AUTO_TEST_CASE(GetSigOpCount)
{
    CScript s1;
    BOOST_CHECK_EQUAL(s1.GetLegacySigOpCount(/*fAccurate=*/false), 0U);
    BOOST_CHECK_EQUAL(s1.GetLegacySigOpCount(/*fAccurate=*/true), 0U);

    uint160 dummy;
    s1 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(s1.GetLegacySigOpCount(/*fAccurate=*/true), 2U);
    s1 << OP_IF << OP_CHECKSIG << OP_ENDIF;
    BOOST_CHECK_EQUAL(s1.GetLegacySigOpCount(/*fAccurate=*/true), 3U);
    BOOST_CHECK_EQUAL(s1.GetLegacySigOpCount(/*fAccurate=*/false), 21U);

    CScript p2sh = GetScriptForDestination(ScriptHash(s1));
    CScript scriptSig;
    scriptSig << OP_0 << Serialize(s1);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig), 3U);

    std::vector<CPubKey> keys;
    for (int i = 0; i < 3; i++) {
        CKey k = GenerateRandomKey();
        keys.push_back(k.GetPubKey());
    }
    CScript s2 = GetScriptForMultisig(1, keys);
    BOOST_CHECK_EQUAL(s2.GetLegacySigOpCount(/*fAccurate=*/true), 3U);
    BOOST_CHECK_EQUAL(s2.GetLegacySigOpCount(/*fAccurate=*/false), 20U);

    p2sh = GetScriptForDestination(ScriptHash(s2));
    BOOST_CHECK_EQUAL(p2sh.GetLegacySigOpCount(/*fAccurate=*/true), 0U);
    BOOST_CHECK_EQUAL(p2sh.GetLegacySigOpCount(/*fAccurate=*/false), 0U);
    CScript scriptSig2;
    scriptSig2 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << Serialize(s2);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig2), 3U);
}

BOOST_AUTO_TEST_CASE(randomized_sigopcount_matches_original)
{
    // Exact original methods
    auto GetScriptOpOriginal{[&](CScript::const_iterator& pc, CScript::const_iterator end, opcodetype& opcodeRet, std::vector<unsigned char>* pvchRet) -> bool {
        opcodeRet = OP_INVALIDOPCODE;
        if (pvchRet)
            pvchRet->clear();
        if (pc >= end)
            return false;

        // Read instruction
        if (end - pc < 1)
            return false;
        unsigned int opcode = *pc++;

        // Immediate operand
        if (opcode <= OP_PUSHDATA4) {
            unsigned int nSize = 0;
            if (opcode < OP_PUSHDATA1) {
                nSize = opcode;
            } else if (opcode == OP_PUSHDATA1) {
                if (end - pc < 1)
                    return false;
                nSize = *pc++;
            } else if (opcode == OP_PUSHDATA2) {
                if (end - pc < 2)
                    return false;
                nSize = ReadLE16(&pc[0]);
                pc += 2;
            } else if (opcode == OP_PUSHDATA4) {
                if (end - pc < 4)
                    return false;
                nSize = ReadLE32(&pc[0]);
                pc += 4;
            }
            if (end - pc < 0 || (unsigned int)(end - pc) < nSize)
                return false;
            if (pvchRet)
                pvchRet->assign(pc, pc + nSize);
            pc += nSize;
        }

        opcodeRet = static_cast<opcodetype>(opcode);
        return true;
    }};

    auto GetSigOpCountOriginal{[&](const CScript& script, bool fAccurate) -> unsigned int {
        unsigned int n = 0;
        CScript::const_iterator pc = script.begin();
        opcodetype lastOpcode = OP_INVALIDOPCODE;
        while (pc < script.end()) {
            opcodetype opcode;
            if (!GetScriptOpOriginal(pc, script.end(), opcode, nullptr)) // inlined GetOp
                break;
            if (opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY)
                n++;
            else if (opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY) {
                if (fAccurate && lastOpcode >= OP_1 && lastOpcode <= OP_16)
                    n += CScript::DecodeOP_N(lastOpcode);
                else
                    n += MAX_PUBKEYS_PER_MULTISIG;
            }
            lastOpcode = opcode;
        }
        return n;
    }};

    for (int i{0}; i < 10'000; ++i) {
        std::vector raw{m_rng.randbytes(m_rng.randrange(1000))};
        CScript script{raw.begin(), raw.end()};
        for (const bool fAccurate : {false, true}) {
            BOOST_CHECK_EQUAL(GetSigOpCountOriginal(script, fAccurate), script.GetLegacySigOpCount(fAccurate));
        }
    }
}

/**
 * Verifies script execution of the zeroth scriptPubKey of tx output and
 * zeroth scriptSig and witness of tx input.
 */
static ScriptError VerifyWithFlag(const CTransaction& output, const CMutableTransaction& input, uint32_t flags)
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
    creationTx.vin.resize(1);
    creationTx.vin[0].prevout.SetNull();
    creationTx.vin[0].scriptSig = CScript();
    creationTx.vout.resize(1);
    creationTx.vout[0].nValue = 1;
    creationTx.vout[0].scriptPubKey = scriptPubKey;

    spendingTx.version = 1;
    spendingTx.vin.resize(1);
    spendingTx.vin[0].prevout.hash = creationTx.GetHash();
    spendingTx.vin[0].prevout.n = 0;
    spendingTx.vin[0].scriptSig = scriptSig;
    spendingTx.vin[0].scriptWitness = witness;
    spendingTx.vout.resize(1);
    spendingTx.vout[0].nValue = 1;
    spendingTx.vout[0].scriptPubKey = CScript();

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
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    // Create key
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();
    // Default flags
    const uint32_t flags{SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH};

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

BOOST_AUTO_TEST_SUITE_END()
