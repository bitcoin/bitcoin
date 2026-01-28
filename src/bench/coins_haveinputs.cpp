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
#include <txdb.h>
#include <util/fs.h>
#include <util/time.h>

#include <cassert>
#include <ranges>
#include <system_error>

static void HaveInputsOnDisk(benchmark::Bench& bench)
{
    const fs::path path{fs::current_path() / strprintf("bench-%s-%lld", bench.name(), GetTime())};
    CCoinsViewDB db{{.path = path, .cache_bytes = 1 << 20, .memory_only = false, .wipe_data = true}, {}};

    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    CCoinsViewCache cache{&db};
    cache.SetBestBlock(uint256::ONE);
    for (auto& tx : block.vtx | std::views::drop(1)) {
        for (auto& txin : tx->vin) {
            cache.AddCoin(txin.prevout, {{COIN, CScript()}, /*nHeightIn=*/1, /*fCoinBaseIn=*/false}, /*possible_overwrite=*/false);
        }
    }
    cache.Flush();

    bench.batch(block.vtx.size() - 1).unit("tx").run([&] {
        CCoinsViewCache view{&db}; // Recreate to avoid caching
        for (auto& tx : block.vtx | std::views::drop(1)) {
            if (view.GetCacheSize() % 10 == 0) (void)view.HaveInputs(*tx); // Exercise the cache-hit path as well.
            assert(view.HaveInputs(*tx));
        }
    });

    std::error_code ec;
    fs::remove_all(path, ec);
}

BENCHMARK(HaveInputsOnDisk);
