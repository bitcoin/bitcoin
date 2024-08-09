// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <node/utxo_snapshot.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <validation.h>
#include <validationinterface.h>

using node::SnapshotMetadata;

namespace {

const std::vector<std::shared_ptr<CBlock>>* g_chain;

void initialize_chain()
{
    const auto params{CreateChainParams(ArgsManager{}, ChainType::REGTEST)};
    static const auto chain{CreateBlockChain(2 * COINBASE_MATURITY, *params)};
    g_chain = &chain;
}

FUZZ_TARGET(utxo_snapshot, .init = initialize_chain)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::unique_ptr<const TestingSetup> setup{
        MakeNoLogFileContext<const TestingSetup>(
            ChainType::REGTEST,
            TestOpts{
                .setup_net = false,
                .setup_validation_interface = false,
            })};
    const auto& node = setup->m_node;
    auto& chainman{*node.chainman};

    const auto snapshot_path = gArgs.GetDataDirNet() / "fuzzed_snapshot.dat";

    Assert(!chainman.SnapshotBlockhash());

    {
        AutoFile outfile{fsbridge::fopen(snapshot_path, "wb")};
        // Metadata
        if (fuzzed_data_provider.ConsumeBool()) {
            std::vector<uint8_t> metadata{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
            outfile << Span{metadata};
        } else {
            DataStream data_stream{};
            auto msg_start = chainman.GetParams().MessageStart();
            int base_blockheight{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 2 * COINBASE_MATURITY)};
            uint256 base_blockhash{g_chain->at(base_blockheight - 1)->GetHash()};
            uint64_t m_coins_count{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(1, 3 * COINBASE_MATURITY)};
            SnapshotMetadata metadata{msg_start, base_blockhash, m_coins_count};
            outfile << metadata;
        }
        // Coins
        if (fuzzed_data_provider.ConsumeBool()) {
            std::vector<uint8_t> file_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
            outfile << Span{file_data};
        } else {
            int height{0};
            for (const auto& block : *g_chain) {
                auto coinbase{block->vtx.at(0)};
                outfile << coinbase->GetHash();
                WriteCompactSize(outfile, 1); // number of coins for the hash
                WriteCompactSize(outfile, 0); // index of coin
                outfile << Coin(coinbase->vout[0], height, /*fCoinBaseIn=*/1);
                height++;
            }
        }
    }

    const auto ActivateFuzzedSnapshot{[&] {
        AutoFile infile{fsbridge::fopen(snapshot_path, "rb")};
        auto msg_start = chainman.GetParams().MessageStart();
        SnapshotMetadata metadata{msg_start};
        try {
            infile >> metadata;
        } catch (const std::ios_base::failure&) {
            return false;
        }
        return !!chainman.ActivateSnapshot(infile, metadata, /*in_memory=*/true);
    }};

    if (fuzzed_data_provider.ConsumeBool()) {
        for (const auto& block : *g_chain) {
            BlockValidationState dummy;
            bool processed{chainman.ProcessNewBlockHeaders({*block}, true, dummy)};
            Assert(processed);
            const auto* index{WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(block->GetHash()))};
            Assert(index);
        }
    }

    if (ActivateFuzzedSnapshot()) {
        LOCK(::cs_main);
        Assert(!chainman.ActiveChainstate().m_from_snapshot_blockhash->IsNull());
        Assert(*chainman.ActiveChainstate().m_from_snapshot_blockhash ==
               *chainman.SnapshotBlockhash());
        const auto& coinscache{chainman.ActiveChainstate().CoinsTip()};
        for (const auto& block : *g_chain) {
            Assert(coinscache.HaveCoin(COutPoint{block->vtx.at(0)->GetHash(), 0}));
            const auto* index{chainman.m_blockman.LookupBlockIndex(block->GetHash())};
            Assert(index);
            Assert(index->nTx == 0);
            if (index->nHeight == chainman.GetSnapshotBaseHeight()) {
                auto params{chainman.GetParams().AssumeutxoForHeight(index->nHeight)};
                Assert(params.has_value());
                Assert(params.value().m_chain_tx_count == index->m_chain_tx_count);
            } else {
                Assert(index->m_chain_tx_count == 0);
            }
        }
        Assert(g_chain->size() == coinscache.GetCacheSize());
    } else {
        Assert(!chainman.SnapshotBlockhash());
        Assert(!chainman.ActiveChainstate().m_from_snapshot_blockhash);
    }
    // Snapshot should refuse to load a second time regardless of validity
    Assert(!ActivateFuzzedSnapshot());
}
} // namespace
