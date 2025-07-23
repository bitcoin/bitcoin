// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <chain.h>
#include <core_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <univalue.h>
#include <validation.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace {

struct TestBlockAndIndex {
    const std::unique_ptr<const TestingSetup> testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    CBlock block{};
    uint256 blockHash{};
    CBlockIndex blockindex{};

    TestBlockAndIndex()
    {
        DataStream stream{benchmark::data::block413567};
        std::byte a{0};
        stream.write({&a, 1}); // Prevent compaction

        stream >> TX_WITH_WITNESS(block);

        blockHash = block.GetHash();
        blockindex.phashBlock = &blockHash;
        blockindex.nBits = 403014710;
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

BENCHMARK(BlockToJsonVerbosity1, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockToJsonVerbosity2, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockToJsonVerbosity3, benchmark::PriorityLevel::HIGH);

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

BENCHMARK(BlockToJsonVerboseWrite, benchmark::PriorityLevel::HIGH);
