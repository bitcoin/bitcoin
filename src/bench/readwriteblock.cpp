// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <flatfile.h>
#include <net.h>
#include <netmessagemaker.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

static CBlock CreateTestBlock()
{
    CBlock block;
    SpanReader{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    return block;
}

static void WriteBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const CBlock block{CreateTestBlock()};
    bench.run([&] {
        LOCK(::cs_main);
        const auto pos{blockman.WriteBlock(block, 413'567)};
        assert(!pos.IsNull());
    });
}

static void ReadBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto& test_block{CreateTestBlock()};
    const auto& expected_hash{test_block.GetHash()};
    const auto& pos{WITH_LOCK(::cs_main, return blockman.WriteBlock(test_block, 413'567))};
    bench.run([&] {
        CBlock block;
        const auto success{blockman.ReadBlock(block, pos, expected_hash)};
        assert(success);
    });
}

static void ReadRawBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto pos{WITH_LOCK(::cs_main, return blockman.WriteBlock(CreateTestBlock(), 413'567))};
    bench.run([&] {
        const auto res{blockman.ReadRawBlock(pos)};
        assert(res);
    });
}

BENCHMARK(WriteBlockBench);
BENCHMARK(ReadBlockBench);
BENCHMARK(ReadRawBlockBench);

static CSerializedNetMsg ReadBlockMessageWithoutWitness(const node::BlockManager& blockman, const FlatFilePos& pos, const uint256& hash)
{
    CBlock block;
    Assert(blockman.ReadBlock(block, pos, hash));
    return NetMsg::Make(NetMsgType::BLOCK, TX_NO_WITNESS(block));
}

static void BlockWithoutWitnessRead(benchmark::Bench& bench, bool witness)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    CBlock fixture{CreateTestBlock()};
    if (witness) {
        for (auto& transaction : fixture.vtx) {
            CMutableTransaction tx{*transaction};
            for (auto& input : tx.vin) input.scriptWitness.stack = {std::vector<uint8_t>(512, 0x42), {}};
            transaction = MakeTransactionRef(std::move(tx));
        }
    }
    assert(std::ranges::all_of(fixture.vtx, [witness](const auto& tx) { return tx->HasWitness() == witness; }));
    const auto hash{fixture.GetHash()};
    const auto pos{WITH_LOCK(::cs_main, return blockman.WriteBlock(fixture, /*nHeight=*/0))};
    const auto expected_size{GetSerializeSize(TX_NO_WITNESS(fixture))};
    bench.unit("block").run([&] {
        const auto message{ReadBlockMessageWithoutWitness(blockman, pos, hash)};
        assert(message.data.size() == expected_size);
    });
}

static void BlockWithoutWitnessRead_Legacy(benchmark::Bench& bench)
{
    BlockWithoutWitnessRead(bench, /*witness=*/false);
}

static void BlockWithoutWitnessRead_Witness(benchmark::Bench& bench)
{
    BlockWithoutWitnessRead(bench, /*witness=*/true);
}

BENCHMARK(BlockWithoutWitnessRead_Legacy);
BENCHMARK(BlockWithoutWitnessRead_Witness);
