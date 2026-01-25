// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <coins.h>
#include <consensus/amount.h>
#include <primitives/block.h>
#include <script/script.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <util/fs.h>

#include <cassert>
#include <ranges>

static void HaveInputsOnDisk(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const BasicTestingSetup>(ChainType::REGTEST)};
    const auto path{testing_setup->m_path_root / fs::PathFromString(bench.name())};
    CCoinsViewDB db{{.path = path, .cache_bytes = 1_MiB, .memory_only = false, .wipe_data = true}, {}};

    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    const uint256 best_block{block.GetHash()};

    CCoinsViewCache cache{&db};
    cache.SetBestBlock(best_block);
    for (auto& tx : block.vtx | std::views::drop(1)) {
        for (auto& txin : tx->vin) {
            cache.AddCoin(txin.prevout, {{COIN, CScript()}, /*nHeightIn=*/1, /*fCoinBaseIn=*/false}, /*possible_overwrite=*/false);
        }
    }
    cache.Flush();

    bench.batch(block.vtx.size() - 1).unit("tx").run([&] {
        CCoinsViewCache view{&db}; // Recreate to avoid caching
        view.SetBestBlock(best_block);
        for (auto& tx : block.vtx | std::views::drop(1)) {
            assert(view.HaveInputs(*tx));
            assert(view.HaveInputs(*tx)); // Exercise the cache-hit path as well.
        }
    });
}

BENCHMARK(HaveInputsOnDisk);
