// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bip352.h>

#include <addresstype.h>
#include <bench/bench.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <util/check.h>

#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

enum class InputType {
    P2PKH,           // legacy pubkey-hash
    P2SH_P2WPKH,     // nested segwitv0 witness-pubkey-hash
    P2WPKH,          // segwitv0 witness-pubkey-hash
    P2TR,            // segwitv1, taproot script-path spend (covers more extraction logic than key-path)
    P2PKH_WorstCase, // legacy pubkey-hash, with scriptSig malleated for worst-case performance (see below)
};

// The extraction doesn't verify signatures, so dummy ones with realistic sizes suffice.
static const std::vector<unsigned char> DUMMY_ECDSA_SIG(71, 0x01);   // low-R/low-S DER signature (following BIP-461) + sighash byte
static const std::vector<unsigned char> DUMMY_SCHNORR_SIG(64, 0x01); // BIP-340 signature (SIGHASH_DEFAULT)
// Likewise, the taproot commitment isn't verified, so a dummy leaf script (<32-byte push> OP_CHECKSIG) suffices.
static const CScript DUMMY_LEAF_SCRIPT{CScript() << std::vector<unsigned char>(32, 0x01) << OP_CHECKSIG};

static void GetPubKeyBench(benchmark::Bench& bench, InputType input_type)
{
    ECC_Context ecc_context{};
    const CPubKey pubkey{GenerateRandomKey().GetPubKey()};

    CScript spk;
    CTxIn txin;
    bip352::PubKey expected_pubkey{pubkey};
    switch (input_type) {
    case InputType::P2PKH:
        spk = GetScriptForDestination(PKHash{pubkey});
        txin.scriptSig = CScript() << DUMMY_ECDSA_SIG << ToByteVector(pubkey);
        break;
    case InputType::P2SH_P2WPKH: {
        const CScript redeem_script{GetScriptForDestination(WitnessV0KeyHash{pubkey})};
        spk = GetScriptForDestination(ScriptHash{redeem_script});
        txin.scriptSig = CScript() << ToByteVector(redeem_script);
        txin.scriptWitness.stack = {DUMMY_ECDSA_SIG, ToByteVector(pubkey)};
        break;
    }
    case InputType::P2WPKH:
        spk = GetScriptForDestination(WitnessV0KeyHash{pubkey});
        txin.scriptWitness.stack = {DUMMY_ECDSA_SIG, ToByteVector(pubkey)};
        break;
    case InputType::P2TR: {
        // Script-path spend, as it runs all of the key-path extraction logic plus the control
        // block checks and the NUMS-H comparison. The control block only needs the right shape
        // (leaf version byte followed by a non-NUMS internal key, no merkle path).
        spk = GetScriptForDestination(WitnessV1Taproot{XOnlyPubKey{pubkey}});
        std::vector<unsigned char> control_block{TAPROOT_LEAF_TAPSCRIPT};
        const XOnlyPubKey internal_key{GenerateRandomKey().GetPubKey()};
        control_block.insert(control_block.end(), internal_key.begin(), internal_key.end());
        txin.scriptWitness.stack = {DUMMY_SCHNORR_SIG, ToByteVector(DUMMY_LEAF_SCRIPT), control_block};
        expected_pubkey = XOnlyPubKey{pubkey};
        break;
    }
    case InputType::P2PKH_WorstCase: {
        // Worst case for the P2PKH scriptSig scan: prepend as many 33-byte pushes (each of which
        // is hashed and compared against the pubkey hash) as the consensus scriptSig size limit
        // allows, followed by the actual <sig> <pubkey>. Data pushes don't count towards
        // MAX_OPS_PER_SCRIPT, the resulting stack stays below MAX_STACK_SIZE, and leftover stack
        // elements are fine for legacy scripts (CLEANSTACK is policy only). Note that the
        // scriptSig exceeds MAX_STANDARD_SCRIPTSIG_SIZE, i.e. such a spend is non-standard.
        spk = GetScriptForDestination(PKHash{pubkey});
        const std::vector<unsigned char> dummy_pubkey(CPubKey::COMPRESSED_SIZE, 0x02);
        const size_t push_size{(CScript() << dummy_pubkey).size()};
        const size_t sig_and_pubkey_size{(CScript() << DUMMY_ECDSA_SIG << ToByteVector(pubkey)).size()};
        const size_t num_dummy_pushes{(MAX_SCRIPT_SIZE - sig_and_pubkey_size) / push_size};
        for (size_t i = 0; i < num_dummy_pushes; ++i) {
            txin.scriptSig << dummy_pubkey;
        }
        txin.scriptSig << DUMMY_ECDSA_SIG << ToByteVector(pubkey);
        Assert(txin.scriptSig.size() <= MAX_SCRIPT_SIZE);
        Assert(num_dummy_pushes + 2 <= MAX_STACK_SIZE);
        break;
    }
    } // no default case, so the compiler can warn about missing cases

    Assert(bip352::GetPubKeyFromInput(txin, spk) == expected_pubkey);
    bench.unit("input").run([&] {
        auto extracted_pubkey = bip352::GetPubKeyFromInput(txin, spk);
        ankerl::nanobench::doNotOptimizeAway(extracted_pubkey);
    });
}

static void BIP352_GetPubKey_P2PKH(benchmark::Bench& bench)           { GetPubKeyBench(bench, InputType::P2PKH);           }
static void BIP352_GetPubKey_P2SH_P2WPKH(benchmark::Bench& bench)     { GetPubKeyBench(bench, InputType::P2SH_P2WPKH);     }
static void BIP352_GetPubKey_P2WPKH(benchmark::Bench& bench)          { GetPubKeyBench(bench, InputType::P2WPKH);          }
static void BIP352_GetPubKey_P2TR(benchmark::Bench& bench)            { GetPubKeyBench(bench, InputType::P2TR);            }
static void BIP352_GetPubKey_P2PKH_WorstCase(benchmark::Bench& bench) { GetPubKeyBench(bench, InputType::P2PKH_WorstCase); }

BENCHMARK(BIP352_GetPubKey_P2PKH);
BENCHMARK(BIP352_GetPubKey_P2SH_P2WPKH);
BENCHMARK(BIP352_GetPubKey_P2WPKH);
BENCHMARK(BIP352_GetPubKey_P2TR);
BENCHMARK(BIP352_GetPubKey_P2PKH_WorstCase);
