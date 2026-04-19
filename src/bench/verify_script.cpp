// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/bench.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <span.h>
#include <test/util/transaction_utils.h>
#include <uint256.h>
#include <util/translation.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

enum class ScriptType {
    P2WPKH, // segwitv0, witness-pubkey-hash (ECDSA signature)
    P2TR_KeyPath, // segwitv1, taproot key-path spend (Schnorr signature)
    P2TR_ScriptPath, // segwitv1, taproot script-path spend (Tapscript leaf with a single OP_CHECKSIG)
};

static size_t ExpectedWitnessStackSize(ScriptType script_type)
{
    switch (script_type) {
    case ScriptType::P2WPKH: return 2; // [pubkey, signature]
    case ScriptType::P2TR_KeyPath: return 1; // [signature]
    case ScriptType::P2TR_ScriptPath: return 3; // [signature, tapscript, control block]
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

// Microbenchmark for verification of standard scripts.
static void VerifyScriptBench(benchmark::Bench& bench, ScriptType script_type)
{
    ECC_Context ecc_context{};

    // Create deterministic key material needed for output script creation / signing
    CKey privkey;
    privkey.Set(uint256::ONE.begin(), uint256::ONE.end(), /*fCompressedIn=*/true);
    CPubKey pubkey = privkey.GetPubKey();
    XOnlyPubKey xonly_pubkey{pubkey};
    CKeyID key_id = pubkey.GetID();

    FlatSigningProvider keystore;
    keystore.keys.emplace(key_id, privkey);
    keystore.pubkeys.emplace(key_id, pubkey);

    // Create crediting and spending transactions with provided input type
    const auto dest{[&]() -> CTxDestination {
        switch (script_type) {
        case ScriptType::P2WPKH: return WitnessV0KeyHash(pubkey);
        case ScriptType::P2TR_KeyPath: return WitnessV1Taproot(xonly_pubkey);
        case ScriptType::P2TR_ScriptPath:
            TaprootBuilder builder;
            builder.Add(0, CScript() << ToByteVector(xonly_pubkey) << OP_CHECKSIG, TAPROOT_LEAF_TAPSCRIPT);
            builder.Finalize(XOnlyPubKey::NUMS_H); // effectively unspendable key-path
            const auto output{builder.GetOutput()};
            keystore.tr_trees.emplace(output, builder);
            return output;
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    }()};
    const CMutableTransaction& txCredit = BuildCreditingTransaction(GetScriptForDestination(dest), 1);
    CMutableTransaction txSpend = BuildSpendingTransaction(/*scriptSig=*/{}, /*scriptWitness=*/{}, CTransaction(txCredit));

    // Sign spending transaction, precompute transaction data
    PrecomputedTransactionData txdata;
    {
        const std::map<COutPoint, Coin> coins{
            {txSpend.vin[0].prevout, Coin(txCredit.vout[0], /*nHeightIn=*/100, /*fCoinBaseIn=*/false)}
        };
        std::map<int, bilingual_str> input_errors;
        assert(SignTransaction(txSpend, &keystore, coins, SIGHASH_ALL, input_errors));
        // Weak sanity check on witness data to ensure we produced the intended spending type
        assert(txSpend.vin[0].scriptWitness.stack.size() == ExpectedWitnessStackSize(script_type));
        txdata.Init(txSpend, /*spent_outputs=*/{txCredit.vout[0]});
    }

    // Benchmark.
    bench.unit("script").run([&] {
        ScriptError err;
        bool success = VerifyScript(
            txSpend.vin[0].scriptSig,
            txCredit.vout[0].scriptPubKey,
            &txSpend.vin[0].scriptWitness,
            STANDARD_SCRIPT_VERIFY_FLAGS,
            MutableTransactionSignatureChecker(&txSpend, 0, txCredit.vout[0].nValue, txdata, MissingDataBehavior::ASSERT_FAIL),
            &err);
        assert(err == SCRIPT_ERR_OK);
        assert(success);
    });
}

static void VerifyScriptP2WPKH(benchmark::Bench& bench) { VerifyScriptBench(bench, ScriptType::P2WPKH); }
static void VerifyScriptP2TR_KeyPath(benchmark::Bench& bench) { VerifyScriptBench(bench, ScriptType::P2TR_KeyPath); }
static void VerifyScriptP2TR_ScriptPath(benchmark::Bench& bench) { VerifyScriptBench(bench, ScriptType::P2TR_ScriptPath); }

static void VerifyNestedIfScript(benchmark::Bench& bench)
{
    std::vector<std::vector<unsigned char>> stack;
    CScript script;
    for (int i = 0; i < 100; ++i) {
        script << OP_1 << OP_IF;
    }
    for (int i = 0; i < 1000; ++i) {
        script << OP_1;
    }
    for (int i = 0; i < 100; ++i) {
        script << OP_ENDIF;
    }
    bench.unit("script").epochIterations(1)
        .setup([&] { stack.clear(); })
        .run([&] {
            ScriptError error;
            const bool ret{EvalScript(stack, script, /*flags=*/0, BaseSignatureChecker(), SigVersion::BASE, &error)};
            assert(ret && error == SCRIPT_ERR_OK);
        });
}

BENCHMARK(VerifyScriptP2WPKH);
BENCHMARK(VerifyScriptP2TR_KeyPath);
BENCHMARK(VerifyScriptP2TR_ScriptPath);
BENCHMARK(VerifyNestedIfScript);
