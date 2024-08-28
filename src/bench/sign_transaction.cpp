// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/bench.h>
#include <coins.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <span.h>
#include <test/util/random.h>
#include <uint256.h>
#include <util/translation.h>

#include <cassert>
#include <map>
#include <vector>

enum class InputType {
    P2WPKH, // segwitv0, witness-pubkey-hash (ECDSA signature)
    P2TR,   // segwitv1, taproot key-path spend (Schnorr signature)
};

static void SignTransactionSingleInput(benchmark::Bench& bench, InputType input_type)
{
    ECC_Context ecc_context{};

    FlatSigningProvider keystore;
    std::vector<CScript> prev_spks;

    // Create a bunch of keys / UTXOs to avoid signing with the same key repeatedly
    for (int i = 0; i < 32; i++) {
        CKey privkey = GenerateRandomKey();
        CPubKey pubkey = privkey.GetPubKey();
        CKeyID key_id = pubkey.GetID();
        keystore.keys.emplace(key_id, privkey);
        keystore.pubkeys.emplace(key_id, pubkey);

        // Create specified locking script type
        CScript prev_spk;
        switch (input_type) {
        case InputType::P2WPKH: prev_spk = GetScriptForDestination(WitnessV0KeyHash(pubkey)); break;
        case InputType::P2TR:   prev_spk = GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey{pubkey})); break;
        default: assert(false);
        }
        prev_spks.push_back(prev_spk);
    }

    // Simple 1-input tx with artificial outpoint
    // (note that for the purpose of signing with SIGHASH_ALL we don't need any outputs)
    COutPoint prevout{/*hashIn=*/Txid::FromUint256(uint256::ONE), /*nIn=*/1337};
    CMutableTransaction unsigned_tx;
    unsigned_tx.vin.emplace_back(prevout);

    // Benchmark.
    int iter = 0;
    bench.minEpochIterations(100).run([&] {
        CMutableTransaction tx{unsigned_tx};
        std::map<COutPoint, Coin> coins;
        CScript prev_spk = prev_spks[(iter++) % prev_spks.size()];
        coins[prevout] = Coin(CTxOut(10000, prev_spk), /*nHeightIn=*/100, /*fCoinBaseIn=*/false);
        std::map<int, bilingual_str> input_errors;
        bool complete = SignTransaction(tx, &keystore, coins, SIGHASH_ALL, input_errors);
        assert(complete);
    });
}

static void SignTransactionECDSA(benchmark::Bench& bench)   { SignTransactionSingleInput(bench, InputType::P2WPKH); }
static void SignTransactionSchnorr(benchmark::Bench& bench) { SignTransactionSingleInput(bench, InputType::P2TR);   }

static void SignSchnorrTapTweakBenchmark(benchmark::Bench& bench, bool use_null_merkle_root)
{
    FastRandomContext rng;
    ECC_Context ecc_context{};

    auto key = GenerateRandomKey();
    auto msg = rng.rand256();
    auto merkle_root = use_null_merkle_root ? uint256() : rng.rand256();
    auto aux = rng.rand256();
    std::vector<unsigned char> sig(64);

    bench.minEpochIterations(100).run([&] {
        bool success = key.SignSchnorr(msg, sig, &merkle_root, aux);
        assert(success);
    });
}

static void SignSchnorrWithMerkleRoot(benchmark::Bench& bench)
{
    SignSchnorrTapTweakBenchmark(bench, /*use_null_merkle_root=*/false);
}

static void SignSchnorrWithNullMerkleRoot(benchmark::Bench& bench)
{
    SignSchnorrTapTweakBenchmark(bench, /*use_null_merkle_root=*/true);
}

BENCHMARK(SignTransactionECDSA, benchmark::PriorityLevel::HIGH);
BENCHMARK(SignTransactionSchnorr, benchmark::PriorityLevel::HIGH);
BENCHMARK(SignSchnorrWithMerkleRoot, benchmark::PriorityLevel::HIGH);
BENCHMARK(SignSchnorrWithNullMerkleRoot, benchmark::PriorityLevel::HIGH);
