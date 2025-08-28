// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license.

#include <addresstype.h>
#include <bench/block_generator.h>
#include <consensus/merkle.h>
#include <key.h>
#include <pow.h>
#include <primitives/block.h>
#include <random.h>
#include <script/script.h>
#include <script/solver.h>
#include <streams.h>
#include <test/util/transaction_utils.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <ranges>
#include <vector>

using namespace util::hex_literals;

namespace {
FastRandomContext& Rng()
{
    static FastRandomContext g{/*fDeterministic=*/true};
    return g;
}

size_t GeomCount(double thresh_prob)
{
    size_t n{1};
    while (Rng().randrange<uint8_t>(100) < thresh_prob * 100) {
        ++n;
    }
    return n;
}


CKey RandKey()
{
    CKey k;
    const auto s{Rng().rand256()};
    k.Set(s.begin(), s.end(), /*fCompressedIn=*/true);
    return k;
}

CPubKey RandPub()
{
    return RandKey().GetPubKey();
}

auto createScriptFactory(const benchmark::ScriptRecipe& rec)
{
    std::array<std::pair<double, std::function<CScript()>>, 11> table{
        std::pair{rec.anchor_prob, [&] { return GetScriptForDestination(PayToAnchor{}); }},
        std::pair{rec.multisig_prob, [&] {
            const size_t keys_count{1 + Rng().randrange<size_t>(15)};
            const size_t required{1 + Rng().randrange<size_t>(keys_count)};
            std::vector<CPubKey> keys;
            keys.reserve(keys_count);
            for (size_t i{}; i < keys_count; ++i) keys.emplace_back(RandPub());
            return GetScriptForMultisig(required, keys);
        }},
        {rec.null_data_prob, [&] {
            const auto len{1 + Rng().randrange<size_t>(90)}; // sometimes exceed policy rules
            return CScript() << OP_RETURN << Rng().randbytes<unsigned char>(len);
        }},
        std::pair{rec.pubkey_prob, [&] { return GetScriptForRawPubKey(RandPub()); }},
        std::pair{rec.pubkeyhash_prob, [&] { return GetScriptForDestination(PKHash(RandPub())); }},
        std::pair{rec.scripthash_prob, [&] { return GetScriptForDestination(ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.witness_v1_taproot_prob, [&] { return GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(RandPub()))); }},
        std::pair{rec.witness_v0_keyhash_prob, [&] { return GetScriptForDestination(WitnessV0KeyHash(RandPub())); }},
        std::pair{rec.witness_v0_scripthash_prob, [&] { return GetScriptForDestination(WitnessV0ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.witness_unknown_prob, [&] { return CScript() << OP_2 << Rng().randbytes<uint8_t>(32); }},
        std::pair{rec.nonstandard_prob, [&] { return CScript() << OP_TRUE; }},
    };

    double sum{};
    for (const auto& p : table | std::views::keys) sum += p;
    assert(sum <= 1);

    return table;
}

CBlock BuildBlock(const CChainParams& params, const benchmark::ScriptRecipe& rec)
{
    ECC_Context ecc_context{};

    assert(rec.geometric_base_prob >= 0 && rec.geometric_base_prob <= 1);
    const auto tx_count{rec.tx_count ? rec.tx_count : 1000 + Rng().randrange(2000)};

    CBlock block{};
    block.vtx.reserve(1 + tx_count);

    // coinbase
    {
        CMutableTransaction cb;
        cb.vin = {CTxIn(COutPoint())};
        cb.vin[0].scriptSig = CScript() << CScriptNum(Rng().randrange(1'000'000)) << OP_0;
        cb.vout = {CTxOut(Rng().randrange(50 * COIN), CScript() << OP_TRUE)};
        block.vtx.push_back(MakeTransactionRef(std::move(cb)));
    }

    auto scriptFactory{createScriptFactory(rec)};
    auto rand_script{[&] {
        double probability{Rng().rand64() * (1.0 / std::numeric_limits<uint64_t>::max())};
        for (const auto& [p, factory] : scriptFactory) {
            if (probability < p) return factory();
            probability -= p;
        }
        const auto raw{Rng().randbytes<uint8_t>(1 + Rng().randrange(100))};
        return CScript(raw.begin(), raw.end());
    }};

    for (size_t i{0}; i < tx_count; ++i) {
        CMutableTransaction tx;
        tx.version = 1 + Rng().randrange<int>(3);
        tx.nLockTime = (Rng().randrange<uint8_t>(100) < 90) ? 0 : Rng().rand32();

        const size_t in_count{GeomCount(rec.geometric_base_prob)};
        tx.vin.resize(in_count);
        for (size_t in{0}; in < in_count; ++in) {
            auto& tx_in{tx.vin[in]};
            tx_in.prevout = {Txid::FromUint256(Rng().rand256()), uint32_t(GeomCount(rec.geometric_base_prob))};
            tx_in.scriptSig = rand_script();

            const size_t witness_count{GeomCount(rec.geometric_base_prob)};
            tx_in.scriptWitness.stack.reserve(witness_count);
            for (size_t w{0}; w < witness_count; ++w) {
                tx_in.scriptWitness.stack.emplace_back(Rng().randbytes<uint8_t>(1 + Rng().randrange(100)));
            }

            tx_in.nSequence = (Rng().randrange<uint8_t>(100) < 99) ? CTxIn::SEQUENCE_FINAL : Rng().rand32();
        }

        const size_t out_count{GeomCount(rec.geometric_base_prob)};
        tx.vout.resize(out_count);
        for (size_t out{0}; out < out_count; ++out) {
            auto& tx_out{tx.vout[out]};
            tx_out.nValue = Rng().randrange(GeomCount(rec.geometric_base_prob) * COIN);
            tx_out.scriptPubKey = rand_script();
        }

        block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }

    block.nVersion = 1 + Rng().randrange<int>(VERSIONBITS_LAST_OLD_BLOCK_VERSION);
    block.nTime = params.GenesisBlock().nTime;
    block.hashPrevBlock.SetNull();
    block.nBits = UintToArith256(params.GetConsensus().powLimit).GetCompact(); // lowest difficulty
    block.nNonce = Rng().rand32();
    block.hashMerkleRoot = BlockMerkleRoot(block);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, params.GetConsensus())) {
        ++block.nNonce;
    }

    // Make sure we've generated a valid block
    {
        BlockValidationState validationState;
        const bool checked{CheckBlock(block, validationState, params.GetConsensus())};
        assert(checked);
    }

    return block;
}

DataStream SerializeBlock(const CBlock& blk)
{
    DataStream ds;
    ds << TX_WITH_WITNESS(blk);
    return ds;
}
} // namespace

namespace benchmark {
DataStream GetBlockData(const CChainParams& chain_params, const ScriptRecipe& recipe)
{
    return SerializeBlock(BuildBlock(chain_params, recipe));
}

CBlock GetBlock(const CChainParams& chain_params, const ScriptRecipe& recipe)
{
    return BuildBlock(chain_params, recipe);
}
} // namespace benchmark
