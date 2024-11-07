// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <flatfile.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

static FlatFilePos WriteBlockToDisk(ChainstateManager& chainman)
{
    DataStream stream{benchmark::data::block413567};
    CBlock block;
    stream >> TX_WITH_WITNESS(block);

    return chainman.m_blockman.SaveBlockToDisk(block, 0);
}

static void ReadBlockFromDiskTest(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    ChainstateManager& chainman{*testing_setup->m_node.chainman};

    CBlock block;
    const auto pos{WriteBlockToDisk(chainman)};

    bench.run([&] {
        const auto success{chainman.m_blockman.ReadBlockFromDisk(block, pos)};
        assert(success);
    });
}

static void ReadRawBlockFromDiskTest(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    ChainstateManager& chainman{*testing_setup->m_node.chainman};

    std::vector<uint8_t> block_data;
    const auto pos{WriteBlockToDisk(chainman)};

    bench.run([&] {
        const auto success{chainman.m_blockman.ReadRawBlockFromDisk(block_data, pos)};
        assert(success);
    });
}

BENCHMARK(ReadBlockFromDiskTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(ReadRawBlockFromDiskTest, benchmark::PriorityLevel::HIGH);
