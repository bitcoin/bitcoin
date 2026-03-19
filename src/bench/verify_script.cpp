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
    P2TR,   // segwitv1, taproot key-path spend (Schnorr signature)
};

// Microbenchmark for verification of standard scripts.
static void VerifyScriptBench(benchmark::Bench& bench, ScriptType script_type)
{
    ECC_Context ecc_context{};

    // Create deterministic key material needed for output script creation / signing
    CKey privkey;
    privkey.Set(uint256::ONE.begin(), uint256::ONE.end(), /*fCompressedIn=*/true);
    CPubKey pubkey = privkey.GetPubKey();
    CKeyID key_id = pubkey.GetID();

    FlatSigningProvider keystore;
    keystore.keys.emplace(key_id, privkey);
    keystore.pubkeys.emplace(key_id, pubkey);

    // Create crediting and spending transactions with provided input type
    const auto dest{[&]() -> CTxDestination {
        switch (script_type) {
        case ScriptType::P2WPKH: return WitnessV0KeyHash(pubkey);
        case ScriptType::P2TR: return WitnessV1Taproot(XOnlyPubKey{pubkey});
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    }()};
    const CMutableTransaction& txCredit = BuildCreditingTransaction(GetScriptForDestination(dest), 1);
    CMutableTransaction txSpend = BuildSpendingTransaction(/*scriptSig=*/{}, /*scriptWitness=*/{}, CTransaction(txCredit));

    // Sign spending transaction, precompute transaction data
    PrecomputedTransactionData txdata;
    {
        std::map<COutPoint, Coin> coins;
        coins[txSpend.vin[0].prevout] = Coin(txCredit.vout[0], /*nHeightIn=*/100, /*fCoinBaseIn=*/false);
        std::map<int, bilingual_str> input_errors;
        bool complete = SignTransaction(txSpend, &keystore, coins, SIGHASH_ALL, input_errors);
        assert(complete);
        txdata.Init(txSpend, /*spent_outputs=*/{txCredit.vout[0]});
    }

    // Benchmark.
    bench.run([&] {
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
static void VerifyScriptP2TR(benchmark::Bench& bench)   { VerifyScriptBench(bench, ScriptType::P2TR);   }

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
    bench.run([&] {
        auto stack_copy = stack;
        ScriptError error;
        bool ret = EvalScript(stack_copy, script, 0, BaseSignatureChecker(), SigVersion::BASE, &error);
        assert(ret);
    });
}

BENCHMARK(VerifyScriptP2WPKH);
BENCHMARK(VerifyScriptP2TR);
BENCHMARK(VerifyNestedIfScript);
