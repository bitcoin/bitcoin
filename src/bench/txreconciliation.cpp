// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <node/txreconciliation.h>


/* Benchmarks */

static void ShouldFanoutTo(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    CSipHasher hasher(frc.rand64(), frc.rand64());
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    // Register 120 inbound peers
    int num_peers{120};
    for (NodeId peer = 0; peer < num_peers; peer++) {
        tracker.PreRegisterPeer(peer);
        tracker.RegisterPeer(peer, /*is_peer_inbound=*/true, 1, 1);
    }
    FastRandomContext rc{/*fDeterministic=*/true};

    // The target function uses caching, so we want to mimic tx repetitions
    // of the real-world behavior.
    std::vector<Wtxid> txs;
    for (size_t i = 0; i < 1000; i++) {
        txs.push_back(Wtxid::FromUint256(rc.rand256()));
    }

    bench.run([&] {
        for (NodeId peer = 0; peer < num_peers; ++peer) {
            tracker.ShouldFanoutTo(txs[rand() % txs.size()], peer, /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/0);
        }
    });
}

BENCHMARK(ShouldFanoutTo, benchmark::PriorityLevel::HIGH);
