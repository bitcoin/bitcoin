// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <coins.h>
#include <coinsviewcacheasync.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <validation.h>

#include <cassert>
#include <ranges>
#include <utility>

static void CoinsViewCacheAsyncBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN, { .coins_db_in_memory = false })};
    Chainstate& chainstate{testing_setup->m_node.chainman->ActiveChainstate()};
    auto& coins_tip{WITH_LOCK(testing_setup->m_node.chainman->GetMutex(), return chainstate.CoinsTip();)};

    for (const auto& tx : block.vtx | std::views::drop(1)) {
        for (const auto& in : tx->vin) {
            Coin coin{};
            coin.out.nValue = 1;
            coins_tip.EmplaceCoinInternalDANGER(COutPoint{in.prevout}, std::move(coin));
        }
    }
    chainstate.ForceFlushStateToDisk();
    CoinsViewCacheAsync view{&coins_tip};

    bench.run([&] {
        const auto reset_guard{view.StartFetching(block)};
        for (const auto& tx : block.vtx | std::views::drop(1)) {
            for (const auto& in : tx->vin) {
                const auto have{view.HaveCoin(in.prevout)};
                assert(have);
            }
        }
    });
}

BENCHMARK(CoinsViewCacheAsyncBenchmark);
