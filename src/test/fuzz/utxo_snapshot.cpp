// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <validationinterface.h>

namespace {

const std::vector<std::shared_ptr<CBlock>>* g_chain;

void initialize_chain()
{
    const auto params{CreateChainParams(ArgsManager{}, CBaseChainParams::REGTEST)};
    static const auto chain{CreateBlockChain(2 * COINBASE_MATURITY, *params)};
    g_chain = &chain;
}

FUZZ_TARGET_INIT(utxo_snapshot, initialize_chain)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::unique_ptr<const TestingSetup> setup{MakeNoLogFileContext<const TestingSetup>()};
    const auto& node = setup->m_node;
    auto& chainman{*node.chainman};

    const auto snapshot_path = GetDataDir() / "fuzzed_snapshot.dat";

    Assert(!chainman.SnapshotBlockhash());

    {
        CAutoFile outfile{fsbridge::fopen(snapshot_path, "wb"), SER_DISK, CLIENT_VERSION};
        const auto file_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        outfile << Span<const uint8_t>{file_data};
    }

    const auto ActivateFuzzedSnapshot{[&] {
        CAutoFile infile{fsbridge::fopen(snapshot_path, "rb"), SER_DISK, CLIENT_VERSION};
        SnapshotMetadata metadata;
        try {
            infile >> metadata;
        } catch (const std::ios_base::failure&) {
            return false;
        }
        return chainman.ActivateSnapshot(infile, metadata, /* in_memory */ true);
    }};

    if (fuzzed_data_provider.ConsumeBool()) {
        for (const auto& block : *g_chain) {
            BlockValidationState dummy;
            bool processed{chainman.ProcessNewBlockHeaders({*block}, dummy, ::Params())};
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
        int64_t chain_tx{};
        for (const auto& block : *g_chain) {
            Assert(coinscache.HaveCoin(COutPoint{block->vtx.at(0)->GetHash(), 0}));
            const auto* index{chainman.m_blockman.LookupBlockIndex(block->GetHash())};
            const auto num_tx{Assert(index)->nTx};
            Assert(num_tx == 1);
            chain_tx += num_tx;
        }
        Assert(g_chain->size() == coinscache.GetCacheSize());
        Assert(chain_tx == chainman.ActiveTip()->nChainTx);
    } else {
        Assert(!chainman.SnapshotBlockhash());
        Assert(!chainman.ActiveChainstate().m_from_snapshot_blockhash);
    }
    // Snapshot should refuse to load a second time regardless of validity
    Assert(!ActivateFuzzedSnapshot());
}
} // namespace
