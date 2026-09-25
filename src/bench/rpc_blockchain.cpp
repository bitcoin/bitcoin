// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <bench/data/block413567_undo.raw.h>
#include <chain.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <kernel/chainparams.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <undo.h>
#include <univalue.h>
#include <util/check.h>
#include <validation.h>

#include <memory>
#include <span>
#include <string>

namespace {

struct TestBlockAndIndex {
    const std::unique_ptr<const TestingSetup> testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    CBlock block{};
    uint256 blockHash{};
    CBlockIndex prev_blockindex{};
    CBlockIndex blockindex{};

    TestBlockAndIndex()
    {
        SpanReader stream{benchmark::data::block413567};
        stream >> TX_WITH_WITNESS(block);

        blockHash = block.GetHash();
        blockindex.phashBlock = &blockHash;
        blockindex.nBits = 403014710;
        // The undo data checksum commits to the previous block's hash.
        prev_blockindex.phashBlock = &block.hashPrevBlock;
        blockindex.pprev = &prev_blockindex;

        // Store the block's undo data so that blockToJSON can read it back.
        // Without it, no fee or prevout is included in the output and
        // TxVerbosity::SHOW_DETAILS_AND_PREVOUT does the same work as
        // TxVerbosity::SHOW_DETAILS.
        CBlockUndo block_undo;
        SpanReader{benchmark::data::block413567_undo} >> block_undo;
        BlockValidationState state;
        LOCK(::cs_main);
        Assert(testing_setup->m_node.chainman->m_blockman.WriteBlockUndo(block_undo, state, blockindex));
    }
};

} // namespace

static void BlockToJson(benchmark::Bench& bench, TxVerbosity verbosity)
{
    TestBlockAndIndex data;
    const uint256 pow_limit{data.testing_setup->m_node.chainman->GetParams().GetConsensus().powLimit};
    bench.run([&] {
        auto univalue = blockToJSON(data.testing_setup->m_node.chainman->m_blockman, data.block, data.blockindex, data.blockindex, verbosity, pow_limit);
        ankerl::nanobench::doNotOptimizeAway(univalue);
    });
}

static void BlockToJsonVerbosity1(benchmark::Bench& bench)
{
    BlockToJson(bench, TxVerbosity::SHOW_TXID);
}

static void BlockToJsonVerbosity2(benchmark::Bench& bench)
{
    BlockToJson(bench, TxVerbosity::SHOW_DETAILS);
}

static void BlockToJsonVerbosity3(benchmark::Bench& bench)
{
    BlockToJson(bench, TxVerbosity::SHOW_DETAILS_AND_PREVOUT);
}

BENCHMARK(BlockToJsonVerbosity1);
BENCHMARK(BlockToJsonVerbosity2);
BENCHMARK(BlockToJsonVerbosity3);

static void BlockToJsonVerboseWrite(benchmark::Bench& bench)
{
    TestBlockAndIndex data;
    const uint256 pow_limit{data.testing_setup->m_node.chainman->GetParams().GetConsensus().powLimit};
    auto univalue = blockToJSON(data.testing_setup->m_node.chainman->m_blockman, data.block, data.blockindex, data.blockindex, TxVerbosity::SHOW_DETAILS_AND_PREVOUT, pow_limit);
    bench.run([&] {
        auto str = univalue.write();
        ankerl::nanobench::doNotOptimizeAway(str);
    });
}

BENCHMARK(BlockToJsonVerboseWrite);
