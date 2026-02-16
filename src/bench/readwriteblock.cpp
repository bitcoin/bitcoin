// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/block_generator.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cassert>
#include <memory>

static void WriteBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::REGTEST)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto& params{testing_setup->m_node.chainman->GetParams()};
    const auto test_block{benchmark::GenerateBlock(params)};
    bench.run([&] {
        const auto pos{blockman.WriteBlock(test_block, /*nHeight=*/1)};
        assert(!pos.IsNull());
    });
}

static void ReadBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::REGTEST)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto& params{testing_setup->m_node.chainman->GetParams()};
    const auto test_block{benchmark::GenerateBlock(params)};
    const auto& expected_hash{test_block.GetHash()};
    const auto& pos{blockman.WriteBlock(test_block, /*nHeight=*/1)};
    bench.run([&] {
        CBlock block;
        const auto success{blockman.ReadBlock(block, pos, expected_hash)};
        assert(success);
    });
}

static void ReadRawBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::REGTEST)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto& params{testing_setup->m_node.chainman->GetParams()};
    const auto test_block{benchmark::GenerateBlock(params)};
    const auto pos{blockman.WriteBlock(test_block, /*nHeight=*/1)};
    bench.run([&] {
        const auto res{blockman.ReadRawBlock(pos)};
        assert(res);
    });
}

BENCHMARK(WriteBlockBench);
BENCHMARK(ReadBlockBench);
BENCHMARK(ReadRawBlockBench);
