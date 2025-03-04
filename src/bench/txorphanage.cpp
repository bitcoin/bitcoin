// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <net.h>
#include <net_processing.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/sign.h>
#include <test/util/setup_common.h>
#include <txorphanage.h>
#include <util/check.h>

#include <cstdint>
#include <memory>

static constexpr unsigned int MAX_ORPHANS{3000};

// Creates a transaction spending outpoints (or 1 randomly generated input if none are given), with num_outputs outputs.
static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, unsigned int num_outputs, FastRandomContext& det_rand)
{
    CMutableTransaction tx;

    // Build vin
    // If no outpoints are given, create a random one.
    if (outpoints.empty()) {
        tx.vin.emplace_back(Txid::FromUint256(det_rand.rand256()), 0);
    } else {
        for (const auto& outpoint : outpoints) {
            tx.vin.emplace_back(outpoint);
        }
    }
    // Ensure txid != wtxid
    assert(tx.vin.size() > 0);
    tx.vin[0].scriptWitness.stack.push_back({1});

    // Build vout
    assert(num_outputs > 0);
    tx.vout.resize(num_outputs);
    for (unsigned int o = 0; o < num_outputs; ++o) {
        tx.vout[o].nValue = det_rand.randrange(100) * CENT;
        tx.vout[o].scriptPubKey = CScript() << CScriptNum(det_rand.randrange(o + 100)) << OP_EQUAL;
    }
    return MakeTransactionRef(tx);
}

static void OrphanageEraseForBlockSinglePeer(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    // Approx max number of inputs that can fit into a standard size tx
    unsigned int num_outputs{1700};
    unsigned int num_pairs{10};

    CBlock block;
    block.vtx.reserve(num_pairs);

    // Create parent+child pairs where the parent has a ton of outputs, all spent by a single child.
    // The children will go into the block. Also create 100 conflicting orphans also spending the same outputs.
    std::vector<CTransactionRef> orphans;

    for (int i{0}; i < 10; ++i) {
        // Create big parent with many outputs.
        auto block_parent = MakeTransactionSpending({}, /*num_outputs=*/num_outputs, det_rand);
        // Create outpoints vector with all outputs from this tx
        std::vector<COutPoint> outpoints;
        outpoints.reserve(block_parent->vout.size());
        for (unsigned int o = 0; o < block_parent->vout.size(); ++o) {
            outpoints.emplace_back(block_parent->GetHash(), o);
        }
        auto block_child = MakeTransactionSpending(outpoints, /*num_outputs=*/1, det_rand);
        block.vtx.emplace_back(block_child);

        // Create a batch of orphans that conflicts with this child
        for (int o{0}; o < 10; ++o) {
            std::shuffle(outpoints.begin(), outpoints.end(), det_rand);
            orphans.emplace_back(MakeTransactionSpending(outpoints, /*num_outputs=*/1, det_rand));
        }
    }

    Assert(std::accumulate(block.vtx.cbegin(), block.vtx.cend(), 0u, [](unsigned int sum, const auto& tx) {
        return sum + GetTransactionWeight(*tx);
    }) <= MAX_BLOCK_WEIGHT);

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        TxOrphanage orphanage;

        // Every orphan was provided by the same peer.
        // This part takes significantly longer than EraseForBlock.
        NodeId peer{2};
        for (const auto& orphan : orphans) {
            Assert(orphanage.AddTx(orphan, peer));
        }
        Assert(orphanage.Size() == 100);

        // Every orphan needs to be deleted because they all conflict with the block.
        orphanage.EraseForBlock(block);
    });
}

static void OrphanageEvictionManyPeers(benchmark::Bench& bench)
{
    NodeId NUM_PEERS{125};
    unsigned int NUM_TRANSACTIONS(MAX_ORPHANS / NUM_PEERS);

    FastRandomContext det_rand{true};

    // Construct transactions to submit to orphanage: 1-in-1-out tiny transactions
    std::vector<CTransactionRef> txns;
    txns.reserve(NUM_TRANSACTIONS);
    for (unsigned int i{0}; i < NUM_TRANSACTIONS; ++i) {
        txns.emplace_back(MakeTransactionSpending({}, /*num_outputs=*/1, det_rand));
    }

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        TxOrphanage orphanage;
        // Worst case: each orphan is announced by every peer. The eviction loop can delete many
        // announcements before deleting a whole orphan.
        for (unsigned int i{0}; i < NUM_TRANSACTIONS; ++i) {
            Assert(orphanage.AddTx(txns.at(i), i % NUM_PEERS));
        }
        Assert(orphanage.Size() == NUM_TRANSACTIONS);
        orphanage.LimitOrphans(0, det_rand);
    });
}

static void OrphanageWorksetManyPeers(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};

    // Create many orphans spending the same output from 1 transaction.
    auto ptx_parent = MakeTransactionSpending({}, /*num_outputs=*/1, det_rand);
    unsigned int num_orphans{MAX_ORPHANS};
    std::vector<CTransactionRef> child_txns;
    child_txns.reserve(num_orphans);
    for (unsigned int c = 0; c < num_orphans; ++c) {
        child_txns.emplace_back(MakeTransactionSpending({{ptx_parent->GetHash(), c}}, /*num_outputs=*/1, det_rand));
    }

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        TxOrphanage orphanage;

        // Number of announcements == number of orphans.
        // There is only 1 peer who provided all of them.
        NodeId peer{1};
        for (const auto& orphan : child_txns) {
            Assert(orphanage.AddTx(orphan, peer));
        }
        Assert(orphanage.Size() == num_orphans);

        // Every orphan spends ptx_parent, so they all need to be added to the peer's workset
        orphanage.AddChildrenToWorkSet(*ptx_parent, det_rand);
    });
}

static void OrphanageWorksetSinglePeer(benchmark::Bench& bench)
{
    NodeId NUM_PEERS{120};

    FastRandomContext det_rand{true};

    // Create big parent with many outputs.
    unsigned int num_outputs = 500;
    auto ptx_parent = MakeTransactionSpending({}, num_outputs, det_rand);
    // Create outpoints vector with all outputs from this tx
    std::vector<COutPoint> outpoints;
    outpoints.reserve(ptx_parent->vout.size());
    for (unsigned int o = 0; o < ptx_parent->vout.size(); ++o) {
        outpoints.emplace_back(ptx_parent->GetHash(), o);
    }

    unsigned int num_orphans = MAX_ORPHANS / NUM_PEERS;
    std::vector<CTransactionRef> child_txns;
    child_txns.reserve(num_orphans);
    for (unsigned int c = 0; c < num_orphans; ++c) {
        // Guarantee that every tx has a different txid
        outpoints.pop_back();
        std::shuffle(outpoints.begin(), outpoints.end(), det_rand);
        child_txns.emplace_back(MakeTransactionSpending(outpoints, /*num_outputs=*/1, det_rand));
    }

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        TxOrphanage orphanage;

        // Every orphan was provided by every peer.
        for (const auto& orphan : child_txns) {
            for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
                orphanage.AddTx(orphan, peer);
            }
        }
        Assert(orphanage.Size() == num_orphans);

        // Every orphan spends ptx_parent, so they all need to be added to some peer's workset.
        orphanage.AddChildrenToWorkSet(*ptx_parent, det_rand);
    });
}

BENCHMARK(OrphanageEraseForBlockSinglePeer, benchmark::PriorityLevel::HIGH);
BENCHMARK(OrphanageEvictionManyPeers, benchmark::PriorityLevel::HIGH);
BENCHMARK(OrphanageWorksetManyPeers, benchmark::PriorityLevel::HIGH);
BENCHMARK(OrphanageWorksetSinglePeer, benchmark::PriorityLevel::HIGH);
