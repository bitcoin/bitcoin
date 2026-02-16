// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/block_generator.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <hash.h>
#include <pow.h>
#include <primitives/block.h>
#include <random.h>
#include <secp256k1.h>
#include <script/script.h>
#include <script/solver.h>
#include <streams.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <ranges>
#include <vector>

namespace {
constexpr size_t WITNESS_RESERVED_VALUE_SIZE{uint256::size()};
constexpr std::array<uint8_t, 4> WITNESS_COMMITMENT_HEADER{0xaa, 0x21, 0xa9, 0xed};
constexpr int GENERATED_BLOCK_HEIGHT{1};

size_t GeomCount(FastRandomContext& rng, double thresh_prob)
{
    assert(thresh_prob >= 0.0);
    if (thresh_prob >= 1.0) {
        thresh_prob = std::nextafter(1.0, 0.0);
    }
    size_t n{1};
    while (rng.randrange<uint8_t>(100) < thresh_prob * 100) {
        ++n;
    }
    return n;
}

double RandomProbability(FastRandomContext& rng)
{
    return rng.rand64() * (1.0 / double(std::numeric_limits<uint64_t>::max()));
}

void AddWitnessCommitment(CBlock& block)
{
    assert(!block.vtx.empty() && !block.vtx[0]->vin.empty());

    const uint256 witness_root{BlockWitnessMerkleRoot(block)};
    std::vector<uint8_t> reserved_value(WITNESS_RESERVED_VALUE_SIZE);

    uint256 commitment{witness_root};
    CHash256().Write(commitment).Write(reserved_value).Finalize(commitment);

    std::vector<uint8_t> commitment_payload(WITNESS_COMMITMENT_HEADER.begin(), WITNESS_COMMITMENT_HEADER.end());
    commitment_payload.insert(commitment_payload.end(), commitment.begin(), commitment.end());

    CMutableTransaction coinbase{*block.vtx[0]};
    coinbase.vin[0].scriptWitness.stack = {std::move(reserved_value)};
    coinbase.vout.emplace_back(/*nValue=*/0, CScript{} << OP_RETURN << commitment_payload);
    block.vtx[0] = MakeTransactionRef(std::move(coinbase));
}

void AssertGeneratedBlockContextChecks(const CBlock& block)
{
    // CheckBlock/IsBlockMutated already cover structure, merkle root, and witness commitment correctness.
    assert(GetBlockWeight(block) <= MAX_BLOCK_WEIGHT);

    const int64_t lock_time_cutoff{block.GetBlockTime()};
    for (const auto& tx : block.vtx) {
        assert(IsFinalTx(*tx, GENERATED_BLOCK_HEIGHT, lock_time_cutoff));
        for (const auto& tx_in : tx->vin) {
            assert(tx_in.scriptSig.IsPushOnly());
        }
    }

    const CScript expected_coinbase_prefix{CScript{} << GENERATED_BLOCK_HEIGHT};
    const auto& script_sig{block.vtx[0]->vin[0].scriptSig};
    assert(script_sig.size() >= expected_coinbase_prefix.size());
    assert(std::equal(expected_coinbase_prefix.begin(), expected_coinbase_prefix.end(), script_sig.begin()));
}

CPubKey RandPub(FastRandomContext& rng)
{
    if (rng.randbool()) {
        auto pubkey{rng.randbytes<CPubKey::SIZE, uint8_t>()};
        pubkey[0] = SECP256K1_TAG_PUBKEY_UNCOMPRESSED;
        return CPubKey(pubkey.begin(), pubkey.end());
    } else {
        auto pubkey{rng.randbytes<CPubKey::COMPRESSED_SIZE, uint8_t>()};
        pubkey[0] = rng.randbool() ? SECP256K1_TAG_PUBKEY_EVEN : SECP256K1_TAG_PUBKEY_ODD;
        return CPubKey(pubkey.begin(), pubkey.end());
    }
}

auto CreateScriptFactory(FastRandomContext& rng, const benchmark::ScriptRecipe& rec)
{
    std::array<std::pair<double, std::function<CScript()>>, 12> table{
        std::pair{rec.anchor_prob, [&] { return GetScriptForDestination(PayToAnchor{}); }},
        std::pair{rec.multisig_prob, [&] {
            const size_t keys_count{1 + rng.randrange<size_t>(MAX_PUBKEYS_PER_MULTISIG)};
            const size_t required{1 + rng.randrange<size_t>(keys_count)};
            std::vector<CPubKey> keys;
            keys.reserve(keys_count);
            for (size_t i{}; i < keys_count; ++i) keys.emplace_back(RandPub(rng));
            return GetScriptForMultisig(required, keys);
        }},
        std::pair{rec.null_data_prob, [&] {
            const auto len{1 + rng.randrange<size_t>(100)}; // can exceed pre-v30 OP_RETURN 83-byte policy limits
            return CScript() << OP_RETURN << rng.randbytes<uint8_t>(len);
        }},
        std::pair{rec.pubkey_prob, [&] { return GetScriptForRawPubKey(RandPub(rng)); }},
        std::pair{rec.pubkeyhash_prob, [&] { return GetScriptForDestination(PKHash(RandPub(rng))); }},
        std::pair{rec.scripthash_prob, [&] { return GetScriptForDestination(ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.witness_v1_taproot_prob, [&] { return GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(RandPub(rng)))); }},
        std::pair{rec.witness_v0_keyhash_prob, [&] { return GetScriptForDestination(WitnessV0KeyHash(RandPub(rng))); }},
        std::pair{rec.witness_v0_scripthash_prob, [&] { return GetScriptForDestination(WitnessV0ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.witness_unknown_prob, [&] { return CScript() << OP_2 << rng.randbytes<uint8_t>(32); }},
        std::pair{rec.nonstandard_prob, [&] { return CScript() << OP_TRUE; }},
        std::pair{rec.random_bytes_prob, [&] {
            const auto raw{rng.randbytes<uint8_t>(1 + rng.randrange(100))};
            return CScript(raw.begin(), raw.end());
        }},
    };

    double sum{};
    for (const auto& p : table | std::views::keys) sum += p;
    assert(std::abs(sum - 1.0) < 0.01);

    return table;
}

CBlock BuildBlock(const CChainParams& params, const benchmark::ScriptRecipe& rec, const uint256& seed)
{
    // Bench callsites use regtest/test chains so block generation can rely on
    // low-difficulty test-chain parameters.
    assert(params.IsTestChain());

    FastRandomContext rng{seed};

    assert(rec.geometric_base_prob >= 0 && rec.geometric_base_prob < 1);
    const auto tx_count{rec.tx_count ? rec.tx_count : 1000 + rng.randrange(2000)};
    const auto& genesis_block{params.GenesisBlock()};
    const int64_t block_time{genesis_block.nTime + GENERATED_BLOCK_HEIGHT * 10 * 60};
    const auto lock_time_upper_bound{static_cast<uint32_t>(std::max<int64_t>(1, block_time))};

    CBlock block{};
    block.vtx.reserve(1 + tx_count);

    // coinbase
    {
        CMutableTransaction cb;
        cb.vin = {CTxIn(COutPoint())};
        cb.vin[0].scriptSig = CScript() << GENERATED_BLOCK_HEIGHT << CScriptNum(rng.randrange(1'000'000)) << OP_0; // BIP-34
        cb.vout = {CTxOut(rng.randrange(50 * COIN), CScript() << OP_TRUE)};
        block.vtx.push_back(MakeTransactionRef(std::move(cb)));
    }

    auto script_factory{CreateScriptFactory(rng, rec)};
    auto rand_lock_script{[&] {
        double probability{RandomProbability(rng)};
        for (const auto& [p, factory] : script_factory) {
            if (probability < p) return factory();
            probability -= p;
        }
        return script_factory.back().second();
    }};
    auto rand_signature{[&] {
        auto sig{rng.randbytes<uint8_t>(70 + rng.randrange<size_t>(4))};
        sig.back() = 0x01; // SIGHASH_ALL
        return sig;
    }};
    const double empty_scriptsig_prob{
        rec.anchor_prob + rec.witness_v1_taproot_prob + rec.witness_v0_keyhash_prob +
        rec.witness_v0_scripthash_prob + rec.witness_unknown_prob
    };
    auto rand_unlock_script{[&] {
        const double probability{RandomProbability(rng)};
        if (probability < empty_scriptsig_prob) return CScript{};
        if (probability < empty_scriptsig_prob + rec.multisig_prob) {
            const size_t sigs_count{1 + rng.randrange<size_t>(3)};
            CScript script{OP_0};
            for (size_t i{0}; i < sigs_count; ++i) script << rand_signature();
            return script;
        }
        const CPubKey pub{RandPub(rng)};
        const std::vector<uint8_t> pub_bytes{pub.begin(), pub.end()};
        return CScript{} << rand_signature() << pub_bytes;
    }};

    for (size_t i{0}; i < tx_count; ++i) {
        CMutableTransaction tx;
        tx.version = 2 + rng.randrange<int>(2); // 2 or 3
        tx.nLockTime = (rng.randrange<uint8_t>(100) < 90) ? 0
            : rng.randrange<uint32_t>(lock_time_upper_bound);

        const size_t in_count{GeomCount(rng, rec.geometric_base_prob)};
        tx.vin.resize(in_count);
        for (size_t in{0}; in < in_count; ++in) {
            auto& tx_in{tx.vin[in]};
            tx_in.prevout = {Txid::FromUint256(rng.rand256()), uint32_t(GeomCount(rng, rec.geometric_base_prob))};
            tx_in.scriptSig = rand_unlock_script();

            const size_t witness_count{GeomCount(rng, rec.geometric_base_prob)};
            tx_in.scriptWitness.stack.reserve(witness_count);
            for (size_t w{0}; w < witness_count; ++w) {
                tx_in.scriptWitness.stack.emplace_back(rng.randbytes<uint8_t>(1 + rng.randrange(100)));
            }

            tx_in.nSequence = CTxIn::SEQUENCE_FINAL;
        }

        const size_t out_count{GeomCount(rng, rec.geometric_base_prob)};
        tx.vout.resize(out_count);
        for (size_t out{0}; out < out_count; ++out) {
            auto& tx_out{tx.vout[out]};
            tx_out.nValue = rng.randrange(GeomCount(rng, rec.geometric_base_prob) * COIN);
            tx_out.scriptPubKey = rand_lock_script();
        }

        block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }

    assert(params.GetConsensus().SegwitHeight <= GENERATED_BLOCK_HEIGHT);
    AddWitnessCommitment(block);

    block.nVersion = 1 + rng.randrange<int>(VERSIONBITS_LAST_OLD_BLOCK_VERSION);
    block.nTime = block_time;
    block.hashPrevBlock = genesis_block.GetHash();
    block.nBits = genesis_block.nBits;
    block.nNonce = rng.rand32();
    block.hashMerkleRoot = BlockMerkleRoot(block);
    AssertGeneratedBlockContextChecks(block);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, params.GetConsensus())) {
        ++block.nNonce;
    }
    assert(!IsBlockMutated(block, /*check_witness_root=*/true));

    // Context-free block validity checks.
    {
        BlockValidationState validationState;
        const bool valid{CheckBlock(block, validationState, params.GetConsensus())};
        assert(valid);
    }

    return block;
}
} // namespace

namespace benchmark {
DataStream GenerateBlockData(const CChainParams& chain_params, const ScriptRecipe& recipe, const uint256& seed)
{
    DataStream stream;
    stream << TX_WITH_WITNESS(GenerateBlock(chain_params, recipe, seed));
    return stream;
}

CBlock GenerateBlock(const CChainParams& chain_params, const ScriptRecipe& recipe, const uint256& seed)
{
    return BuildBlock(chain_params, recipe, seed);
}
} // namespace benchmark
