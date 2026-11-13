// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <uint256.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

/** The shape of a synthetic block. */
struct BlockShape {
    /** The number of transactions in the block (all identical). */
    size_t num_txn = 1;
    /** The number of inputs in each transaction. */
    size_t num_txin = 1;
    /** The size of each scriptSig. */
    size_t size_scriptsig = 0;
    /** The number of outputs in each transaction. */
    size_t num_txout = 1;
    /** The size of each scriptPubKey. */
    size_t size_spk = 0;
    /** The number of items in each input's witness stack. */
    size_t num_wit_items = 0;
    /** The size of each witness stack item. */
    size_t size_wit_item = 0;
};

/** Write the serialization of a transaction specified by shape. */
void WriteTx(VectorWriter& s, const BlockShape& shape)
{
    const bool have_witness = (shape.num_wit_items > 0);
    // Version.
    s << CTransaction::CURRENT_VERSION;
    // Extended serialization flag and marker, if needed.
    if (have_witness) s << uint8_t{0} << uint8_t{1};
    // Inputs.
    WriteCompactSize(s, shape.num_txin);
    for (size_t i = 0; i < shape.num_txin; ++i) {
        // Prevout.
        s << uint256::ONE << uint32_t(i);
        // scriptSig (all OP_1).
        s << std::vector<unsigned char>(shape.size_scriptsig, 0x51);
        // nSequence.
        s << CTxIn::SEQUENCE_FINAL;
    }
    // Outputs.
    WriteCompactSize(s, shape.num_txout);
    for (size_t i = 0; i < shape.num_txout; ++i) {
        // nValue (1 BTC).
        s << COIN;
        // scriptPubKey (all OP_2).
        s << std::vector<unsigned char>(shape.size_spk, 0x52);
    }
    // Witnesses.
    if (have_witness) {
        for (size_t i = 0; i < shape.num_txin; ++i) {
            // Number of witness items.
            WriteCompactSize(s, shape.num_wit_items);
            for (size_t j = 0; j < shape.num_wit_items; ++j) {
                // Witness items (all 0x03 bytes).
                s << std::vector<unsigned char>(shape.size_wit_item, 0x03);
            }
        }
    }
    // nLockTime.
    s << uint32_t{0};
}

/** Benchmark deserializing a block of the specified shape. */
void DeserializeBlock(benchmark::Bench& bench, const BlockShape& shape)
{
    // Construct one serialized transaction.
    std::vector<unsigned char> tx;
    {
        VectorWriter s{tx, 0};
        WriteTx(s, shape);
    }
    // Construct serialized block.
    std::vector<unsigned char> data;
    {
        VectorWriter s{data, 0};
        // Header.
        s << CBlockHeader{};
        // Transactions.
        WriteCompactSize(s, shape.num_txn);
        for (size_t i = 0; i < shape.num_txn; ++i) s.write(MakeByteSpan(tx));
    }
    // Verify block deserialization succeeds and has valid weight.
    {
        CBlock block;
        SpanReader{data} >> TX_WITH_WITNESS(block);
        assert(GetBlockWeight(block) <= MAX_BLOCK_WEIGHT);
    }
    // Run benchmark.
    bench.unit("block").run([&] {
        CBlock block;
        SpanReader{data} >> TX_WITH_WITNESS(block);
        assert(!block.vtx.empty());
    });
}

} // namespace

/** Spends of P2WPKH outputs to P2WPKH outputs: a typical segwit block. */
static void DeserializeBlockP2WPKH(benchmark::Bench& bench)
{
    DeserializeBlock(bench, {.num_txn = 4385,
                             .num_txin = 2,
                             .num_txout = 2, .size_spk = 22,
                             .num_wit_items = 2, .size_wit_item = 72});
}

/** The smallest segwit transactions: the most transactions a block can have. */
static void DeserializeBlockMinimalSegwitTxs(benchmark::Bench& bench)
{
    DeserializeBlock(bench, {.num_txn = 16392, .num_wit_items = 1});
}

/** One transaction with as many inputs (each with a single-element witness) as fit. */
static void DeserializeBlockManyInputs(benchmark::Bench& bench)
{
    DeserializeBlock(bench, {.num_txin = 24093, .num_wit_items = 1});
}

/** One transaction with as many outputs as fit. */
static void DeserializeBlockManyOutputs(benchmark::Bench& bench)
{
    DeserializeBlock(bench, {.num_txout = 111096});
}

/** One transaction whose witness has as many 1-byte elements as fit. */
static void DeserializeBlockManyWitnessElements(benchmark::Bench& bench)
{
    DeserializeBlock(bench, {.num_wit_items = 1999714, .size_wit_item = 1});
}

BENCHMARK(DeserializeBlockP2WPKH);
BENCHMARK(DeserializeBlockMinimalSegwitTxs);
BENCHMARK(DeserializeBlockManyInputs);
BENCHMARK(DeserializeBlockManyOutputs);
BENCHMARK(DeserializeBlockManyWitnessElements);
