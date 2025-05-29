// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <kernel/coinstats.h>
#include <node/blockstorage.h>
#include <node/utxo_snapshot.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/result.h>
#include <util/time.h>
#include <validation.h>

#include <cstdint>
#include <functional>
#include <ios>
#include <memory>
#include <optional>
#include <vector>

using node::SnapshotMetadata;

namespace {

const std::vector<std::shared_ptr<CBlock>>* g_chain;
TestingSetup* g_setup{nullptr};

/** Sanity check the assumeutxo values hardcoded in chainparams for the fuzz target. */
void sanity_check_snapshot()
{
    Assert(g_chain && g_setup == nullptr);

    // Create a temporary chainstate manager to connect the chain to.
    const auto tmp_setup{MakeNoLogFileContext<TestingSetup>(ChainType::REGTEST, TestOpts{.setup_net = false})};
    const auto& node{tmp_setup->m_node};
    for (auto& block: *g_chain) {
        ProcessBlock(node, block);
    }

    // Connect the chain to the tmp chainman and sanity check the chainparams snapshot values.
    LOCK(cs_main);
    auto& cs{node.chainman->ActiveChainstate()};
    cs.ForceFlushStateToDisk();
    const auto stats{*Assert(kernel::ComputeUTXOStats(kernel::CoinStatsHashType::HASH_SERIALIZED, &cs.CoinsDB(), node.chainman->m_blockman))};
    const auto cp_au_data{*Assert(node.chainman->GetParams().AssumeutxoForHeight(2 * COINBASE_MATURITY))};
    Assert(stats.nHeight == cp_au_data.height);
    Assert(stats.nTransactions + 1 == cp_au_data.m_chain_tx_count); // +1 for the genesis tx.
    Assert(stats.hashBlock == cp_au_data.blockhash);
    Assert(AssumeutxoHash{stats.hashSerialized} == cp_au_data.hash_serialized);
}

template <bool INVALID>
void initialize_chain()
{
    const auto params{CreateChainParams(ArgsManager{}, ChainType::REGTEST)};
    static const auto chain{CreateBlockChain(2 * COINBASE_MATURITY, *params)};
    g_chain = &chain;

    // Make sure we can generate a valid snapshot.
    sanity_check_snapshot();

    static const auto setup{
        MakeNoLogFileContext<TestingSetup>(ChainType::REGTEST,
                                           TestOpts{
                                               .setup_net = false,
                                               .setup_validation_interface = false,
                                               .min_validation_cache = true,
                                           }),
    };
    if constexpr (INVALID) {
        auto& chainman{*setup->m_node.chainman};
        for (const auto& block : chain) {
            BlockValidationState dummy;
            bool processed{chainman.ProcessNewBlockHeaders({{block->GetBlockHeader()}}, true, dummy)};
            Assert(processed);
            const auto* index{WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(block->GetHash()))};
            Assert(index);
        }
    }
    g_setup = setup.get();
}

template <bool INVALID>
void utxo_snapshot_fuzz(FuzzBufferType buffer)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider, /*min=*/1296688602)); // regtest genesis block timestamp
    auto& setup{*g_setup};
    bool dirty_chainman{false}; // Reuse the global chainman, but reset it when it is dirty
    auto& chainman{*setup.m_node.chainman};

    const auto snapshot_path = gArgs.GetDataDirNet() / "fuzzed_snapshot.dat";

    Assert(!chainman.SnapshotBlockhash());

    {
        AutoFile outfile{fsbridge::fopen(snapshot_path, "wb")};
        // Metadata
        if (fuzzed_data_provider.ConsumeBool()) {
            std::vector<uint8_t> metadata{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
            outfile << std::span{metadata};
        } else {
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
            outfile << std::span{file_data};
        } else {
            int height{1};
            for (const auto& block : *g_chain) {
                auto coinbase{block->vtx.at(0)};
                outfile << coinbase->GetHash();
                WriteCompactSize(outfile, 1); // number of coins for the hash
                WriteCompactSize(outfile, 0); // index of coin
                outfile << Coin(coinbase->vout[0], height, /*fCoinBaseIn=*/1);
                height++;
            }
        }
        if constexpr (INVALID) {
            // Append an invalid coin to ensure invalidity. This error will be
            // detected late in PopulateAndValidateSnapshot, and allows the
            // INVALID fuzz target to reach more potential code coverage.
            const auto& coinbase{g_chain->back()->vtx.back()};
            outfile << coinbase->GetHash();
            WriteCompactSize(outfile, 1);   // number of coins for the hash
            WriteCompactSize(outfile, 999); // index of coin
            outfile << Coin{coinbase->vout[0], /*nHeightIn=*/999, /*fCoinBaseIn=*/0};
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
        // Consume the bool, but skip the code for the INVALID fuzz target
        if constexpr (!INVALID) {
            for (const auto& block : *g_chain) {
                BlockValidationState dummy;
                bool processed{chainman.ProcessNewBlockHeaders({{block->GetBlockHeader()}}, true, dummy)};
                Assert(processed);
                const auto* index{WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(block->GetHash()))};
                Assert(index);
            }
            dirty_chainman = true;
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
        dirty_chainman = true;
    } else {
        Assert(!chainman.SnapshotBlockhash());
        Assert(!chainman.ActiveChainstate().m_from_snapshot_blockhash);
    }
    // Snapshot should refuse to load a second time regardless of validity
    Assert(!ActivateFuzzedSnapshot());
    if constexpr (INVALID) {
        // Activating the snapshot, or any other action that makes the chainman
        // "dirty" can and must not happen for the INVALID fuzz target
        Assert(!dirty_chainman);
    }
    if (dirty_chainman) {
        setup.m_node.chainman.reset();
        setup.m_make_chainman();
        setup.LoadVerifyActivateChainstate();
    }
}

// There are two fuzz targets:
//
// The target 'utxo_snapshot', which allows valid snapshots, but is slow,
// because it has to reset the chainstate manager on almost all fuzz inputs.
// Otherwise, a dirty header tree or dirty chainstate could leak from one fuzz
// input execution into the next, which makes execution non-deterministic.
//
// The target 'utxo_snapshot_invalid', which is fast and does not require any
// expensive state to be reset.
FUZZ_TARGET(utxo_snapshot /*valid*/, .init = initialize_chain<false>) { utxo_snapshot_fuzz<false>(buffer); }
FUZZ_TARGET(utxo_snapshot_invalid, .init = initialize_chain<true>) { utxo_snapshot_fuzz<true>(buffer); }

} // namespace
