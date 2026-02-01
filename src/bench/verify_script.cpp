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

// Microbenchmark for verification of a basic P2WPKH script. Can be easily
// modified to measure performance of other types of scripts.
static void VerifyScriptBench(benchmark::Bench& bench)
{
    ECC_Context ecc_context{};

    // Create deterministic key material needed for output script creation / signing
    FlatSigningProvider keystore;
    CPubKey pubkey;
    {
        CKey privkey;
        privkey.Set(uint256::ONE.begin(), uint256::ONE.end(), /*fCompressedIn=*/true);
        pubkey = privkey.GetPubKey();
        CKeyID key_id = pubkey.GetID();
        keystore.keys.emplace(key_id, privkey);
        keystore.pubkeys.emplace(key_id, pubkey);
    }

    // Create crediting and spending transactions
    CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    const CMutableTransaction& txCredit = BuildCreditingTransaction(scriptPubKey, 1);
    CMutableTransaction txSpend = BuildSpendingTransaction(/*scriptSig=*/{}, /*scriptWitness=*/{}, CTransaction(txCredit));

    // Sign spending transaction
    {
        std::map<COutPoint, Coin> coins;
        coins[txSpend.vin[0].prevout] = Coin(txCredit.vout[0], /*nHeightIn=*/100, /*fCoinBaseIn=*/false);
        std::map<int, bilingual_str> input_errors;
        bool complete = SignTransaction(txSpend, &keystore, coins, SIGHASH_ALL, input_errors);
        assert(complete);
    }

    // Benchmark.
    bench.run([&] {
        ScriptError err;
        bool success = VerifyScript(
            txSpend.vin[0].scriptSig,
            txCredit.vout[0].scriptPubKey,
            &txSpend.vin[0].scriptWitness,
            STANDARD_SCRIPT_VERIFY_FLAGS,
            MutableTransactionSignatureChecker(&txSpend, 0, txCredit.vout[0].nValue, MissingDataBehavior::ASSERT_FAIL),
            &err);
        assert(err == SCRIPT_ERR_OK);
        assert(success);
    });
}

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

BENCHMARK(VerifyScriptBench);
BENCHMARK(VerifyNestedIfScript);
