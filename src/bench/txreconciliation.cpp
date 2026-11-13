// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <bench/bench.h>
#include <minisketch.h>
#include <node/minisketchwrapper.h>
#include <node/txreconciliation_impl.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace node;

// Reconciliation cost is dominated by sketch decoding, whose complexity is quadratic in the sketch
// capacity (see src/minisketch/README.md). The capacity is chosen by the sending peer, so these
// benchmarks bound the work a single message can impose on the message processing thread. Capacities
// span from a typical round up to MAX_SKETCH_CAPACITY and the MAX_SKETCH_CAPACITY * 2 an extension may reach
static void ReconcileSketchDecode(benchmark::Bench& bench, uint32_t capacity)
{
    // Worst case: the symmetric difference exactly fills the sketch, so decode does the full amount
    // of work and succeeds (failing would be cheaper)
    Minisketch remote{MakeMinisketch32(capacity)};
    const std::vector<uint8_t> remote_bytes{remote.Serialize()};

    // The most a sketch with "capacity" can decode within the protocol's false-positive margin
    const size_t max_elements{minisketch_compute_max_elements(RECON_FIELD_SIZE, capacity, RECON_FALSE_POSITIVE_COEF)};

    bench.batch(max_elements).unit("element").run([&] {
        Minisketch remote_sketch{MakeMinisketch32(capacity).Deserialize(remote_bytes)};
        Minisketch local_sketch{MakeMinisketch32(capacity)};
        for (uint64_t i = 0; i < max_elements; ++i) local_sketch.Add(i + 1);
        std::vector<uint64_t> differences(max_elements);
        local_sketch.Merge(remote_sketch).Decode(differences);
    });
}

// A typical round: at ~7 tx/s over the 30s reconciliation interval both sets hold ~210 txs, and
// EstimateSketchCapacity with q=0.25 yields ceil(0.25 * 210) + 1 ~= 54.
static void ReconcileSketchDecodeTypical(benchmark::Bench& bench) { ReconcileSketchDecode(bench, 54); }
static void ReconcileSketchDecodeReconSetMax(benchmark::Bench& bench) { ReconcileSketchDecode(bench, MAX_RECONSET_SIZE + 1); }
static void ReconcileSketchDecodeMaxCapacity(benchmark::Bench& bench) { ReconcileSketchDecode(bench, MAX_SKETCH_CAPACITY); }
static void ReconcileSketchDecodeExtensionMax(benchmark::Bench& bench) { ReconcileSketchDecode(bench, MAX_SKETCH_CAPACITY * 2); }

// Responder cost: building a sketch (linear in capacity per element)
static void ReconcileSketchConstruct(benchmark::Bench& bench, uint32_t capacity)
{
    bench.batch(capacity).unit("element").run([&] {
        Minisketch sketch{MakeMinisketch32(capacity)};
        for (uint64_t i = 0; i < capacity; ++i) sketch.Add(2 * i + 1);
        ankerl::nanobench::doNotOptimizeAway(sketch.Serialize());
    });
}

static void ReconcileSketchConstructReconSetMax(benchmark::Bench& bench) { ReconcileSketchConstruct(bench, MAX_RECONSET_SIZE + 1); }

BENCHMARK(ReconcileSketchDecodeTypical);
BENCHMARK(ReconcileSketchDecodeReconSetMax);
BENCHMARK(ReconcileSketchDecodeMaxCapacity);
BENCHMARK(ReconcileSketchDecodeExtensionMax);
BENCHMARK(ReconcileSketchConstructReconSetMax);
