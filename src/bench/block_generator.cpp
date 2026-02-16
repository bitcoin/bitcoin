// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/block_generator.h>

#include <addresstype.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <hash.h>
#include <kernel/chainparams.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <random.h>
#include <script/script.h>
#include <script/solver.h>
#include <secp256k1.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/script.h>
#include <util/check.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace {
constexpr size_t WITNESS_RESERVED_VALUE_SIZE{uint256::size()};
constexpr std::array<uint8_t, 4> WITNESS_COMMITMENT_HEADER{0xaa, 0x21, 0xa9, 0xed};

double RandomProbability(FastRandomContext& rng)
{
    return rng.rand64() * (1.0 / double(std::numeric_limits<uint64_t>::max()));
}

void AddWitnessCommitment(CBlock& block)
{
    assert(!block.vtx.empty() && !block.vtx[0]->vin.empty());

    std::vector<uint8_t> reserved_value(WITNESS_RESERVED_VALUE_SIZE);

    uint256 commitment{BlockWitnessMerkleRoot(block)};
    CHash256().Write(commitment).Write(reserved_value).Finalize(commitment);

    std::vector<uint8_t> commitment_payload(WITNESS_COMMITMENT_HEADER.begin(), WITNESS_COMMITMENT_HEADER.end());
    commitment_payload.insert(commitment_payload.end(), commitment.begin(), commitment.end());

    CMutableTransaction coinbase{*block.vtx[0]};
    coinbase.vin[0].scriptWitness.stack = {std::move(reserved_value)};
    coinbase.vout.resize(1); // Replace the old commitment if overweight transactions were removed.
    coinbase.vout.emplace_back(/*nValue=*/0, CScript{} << OP_RETURN << commitment_payload);
    block.vtx[0] = MakeTransactionRef(std::move(coinbase));
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
        std::pair{rec.witness_v0_scripthash_prob, [&] { return P2WSH_OP_TRUE; }},
        std::pair{rec.witness_unknown_prob, [&] { return GetScriptForDestination(WitnessUnknown{2, rng.randbytes<uint8_t>(32)}); }},
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
} // namespace

namespace benchmark {
DataStream GenerateBlockData(const CChainParams& chain_params, const ScriptRecipe& recipe, const uint256& seed)
{
    DataStream stream;
    stream << TX_WITH_WITNESS(GenerateBlock(chain_params, recipe, seed));
    return stream;
}

CBlock GenerateBlock(const CChainParams& params, const ScriptRecipe& rec, const uint256& seed)
{
    // Only regtest has the low difficulty and height-one deployments used below.
    Assert(params.GetChainType() == ChainType::REGTEST);

    FastRandomContext rng{seed};

    assert(rec.geometric_base_prob >= 0 && rec.geometric_base_prob < 1);
    const double geom_prob{std::min(rec.geometric_base_prob, 0.99)};
    auto geom_count{[&] {
        size_t n{1};
        while (RandomProbability(rng) < geom_prob) ++n;
        return n;
    }};

    const auto tx_count{rec.tx_count ? rec.tx_count : 1000 + rng.randrange(2000)};
    const auto& genesis_block{params.GenesisBlock()};
    const uint32_t block_time{genesis_block.nTime + GENERATED_BLOCK_HEIGHT * 10 * 60};

    CBlock block{};
    block.vtx.reserve(1 + tx_count);

    // coinbase
    {
        CMutableTransaction cb;
        cb.vin = {CTxIn(COutPoint())};
        cb.vin[0].scriptSig = CScript() << GENERATED_BLOCK_HEIGHT << CScriptNum(rng.randrange(1'000'000)) << OP_0; // BIP-34
        cb.vout = {CTxOut(rng.randrange(GetBlockSubsidy(GENERATED_BLOCK_HEIGHT, params.GetConsensus())), CScript() << OP_TRUE)};
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
        const auto pub{ToByteVector(RandPub(rng))};
        return CScript{} << rand_signature() << pub;
    }};

    for (size_t i{0}; i < tx_count; ++i) {
        CMutableTransaction tx;
        tx.version = 2 + rng.randrange<int>(2); // 2 or 3
        tx.nLockTime = (rng.randrange<uint8_t>(100) < 90) ? 0
            : rng.randrange<uint32_t>(block_time);

        tx.vin.resize(geom_count());
        for (auto& tx_in : tx.vin) {
            tx_in.prevout = {Txid::FromUint256(rng.rand256()), uint32_t(geom_count())};
            tx_in.scriptSig = rand_unlock_script();

            if (rec.include_witness) {
                const size_t witness_count{geom_count()};
                tx_in.scriptWitness.stack.reserve(witness_count);
                for (size_t w{0}; w < witness_count; ++w) {
                    tx_in.scriptWitness.stack.emplace_back(rng.randbytes<uint8_t>(1 + rng.randrange(100)));
                }
            }
        }

        tx.vout.resize(geom_count());
        for (auto& tx_out : tx.vout) {
            tx_out.nValue = rng.randrange(geom_count() * COIN);
            tx_out.scriptPubKey = rand_lock_script();
        }

        block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }

    if (rec.include_witness) AddWitnessCommitment(block);

    block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    block.nTime = block_time;
    block.hashPrevBlock = genesis_block.GetHash();
    block.nBits = genesis_block.nBits;
    block.nNonce = rng.rand32();

    int64_t excess_weight{GetBlockWeight(block) - MAX_BLOCK_WEIGHT};
    bool trimmed{false};
    while (excess_weight > 0 && block.vtx.size() > 1) {
        excess_weight -= GetTransactionWeight(*block.vtx.back());
        block.vtx.pop_back();
        trimmed = true;
    }
    if (trimmed && rec.include_witness) AddWitnessCommitment(block);
    Assert(GetBlockWeight(block) <= MAX_BLOCK_WEIGHT);

    block.hashMerkleRoot = BlockMerkleRoot(block);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, params.GetConsensus())) {
        ++block.nNonce;
    }

    // Validate a copy, since validation caches its results in the checked block.
    const CBlock checked{block};
    Assert(!IsBlockMutated(checked, /*check_witness_root=*/true));
    BlockValidationState state;
    Assert(CheckBlock(checked, state, params.GetConsensus()));
    return block;
}
} // namespace benchmark
