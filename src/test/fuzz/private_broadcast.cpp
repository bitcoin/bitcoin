// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <private_broadcast.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/hasher.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace {
CTransactionRef MakeDummyTx(uint32_t id, uint32_t witness_variant)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].nSequence = id;
    if (witness_variant > 0) {
        mtx.vin[0].scriptWitness.stack.resize(witness_variant);
    }
    return MakeTransactionRef(mtx);
}
} // namespace

void initialize_private_broadcast()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(private_broadcast, .init = initialize_private_broadcast)
{
    // Required because `model` below is keyed with SaltedWtxidHasher, whose
    // salt is drawn from the global PRNG; seeding it deterministically keeps
    // the target reproducible (enforced by the CheckGlobals harness).
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    NodeClockContext clock_ctx{ConsumeTime(fdp)};
    PrivateBroadcast pb;

    // A simple reference model of `pb` that the harness maintains independently
    // and cross-checks against. It maps each wtxid currently in the queue to the
    // number of times that tx has been picked for sending (i.e. how many
    // send-statuses it should hold). We can model this exactly -- unlike the
    // confirmation state and tie-breaking, which depend on `pb`'s internal
    // unordered_map iteration order and so are only checked via looser bounds.
    std::unordered_map<Wtxid, size_t, SaltedWtxidHasher> model;

    // Small pool of stable txs the fuzzer can re-pick across iterations.
    // Reusing the same txs is what lets Add() hit its already-present branch
    // and Remove() hit its present branch (a fresh tx every time would never
    // reach those). The witness variants give several distinct wtxids per
    // txid (PrivateBroadcast keys only on wtxid), so many entries coexist.
    std::vector<CTransactionRef> pool;
    pool.reserve(32);
    for (uint32_t id{0}; id < 8; ++id) {
        for (uint32_t w{0}; w < 4; ++w) {
            pool.push_back(MakeDummyTx(id, w));
        }
    }

    LIMITED_WHILE(fdp.ConsumeBool(), 500)
    {
        CallOneOf(
            fdp,
            [&] {
                const auto& tx{pool.at(fdp.ConsumeIntegralInRange<size_t>(0, pool.size() - 1))};
                if (pb.Add(tx)) model.emplace(tx->GetWitnessHash(), 0);
            },
            [&] {
                // Mint a fresh tx so the queue can grow past the static pool
                // and `Priority` orderings span more entries.
                const auto tx{MakeDummyTx(fdp.ConsumeIntegral<uint32_t>(),
                                          fdp.ConsumeIntegralInRange<uint32_t>(0, 3))};
                if (pb.Add(tx)) model.emplace(tx->GetWitnessHash(), 0);
            },
            [&] {
                const auto& tx{pool.at(fdp.ConsumeIntegralInRange<size_t>(0, pool.size() - 1))};
                if (pb.Remove(tx).has_value()) model.erase(tx->GetWitnessHash());
            },
            [&] {
                // Advance the mock clock so GetStale()'s time thresholds
                // actually transition.
                clock_ctx.set(ConsumeTime(fdp));
            },
            [&] {
                const NodeId id{fdp.ConsumeIntegralInRange<NodeId>(0, 16)};
                const CService addr{ConsumeService(fdp)};
                // num_picked (== send count) is the primary priority key, so
                // whatever tx is picked must currently have the fewest sends of
                // any queued tx. (Ties are broken by state we don't model, so
                // we only check the primary key, not the exact tx.)
                size_t min_sends{std::numeric_limits<size_t>::max()};
                for (const auto& [wtxid, sends] : model) min_sends = std::min(min_sends, sends);

                const auto picked{pb.PickTxForSend(id, addr)};
                // A tx is returned iff the queue is non-empty (picking does not
                // change membership).
                assert(picked.has_value() == !model.empty());
                if (picked) {
                    const auto it{model.find((*picked)->GetWitnessHash())};
                    assert(it != model.end());     // picked tx is a live member
                    assert(it->second == min_sends); // ... with minimal send count
                    ++it->second;                  // pick records exactly one send
                }
            },
            [&] {
                const NodeId id{fdp.ConsumeIntegralInRange<NodeId>(0, 16)};
                // We can't assert *which* tx comes back: GetSendStatusByNode()
                // returns the first tx in map iteration order with a send
                // status for this node, not necessarily the last one picked.
                // But any tx returned must still be a live queue member --
                // Remove() drops a tx's node associations along with it.
                if (const auto tx{pb.GetTxForNode(id)}) {
                    assert(model.contains((*tx)->GetWitnessHash()));
                }
            },
            [&] {
                const NodeId id{fdp.ConsumeIntegralInRange<NodeId>(0, 16)};
                pb.NodeConfirmedReception(id);
                // The node reads back as confirmed iff it had a tx picked for
                // it (i.e. a send status for the confirmation to attach to).
                // Both queries resolve the node via the same first-match.
                assert(pb.DidNodeConfirmReception(id) == pb.GetTxForNode(id).has_value());
            },
            [&] {
                const NodeId id{fdp.ConsumeIntegralInRange<NodeId>(0, 16)};
                pb.DidNodeConfirmReception(id);
            },
            [&] {
                assert(pb.HavePendingTransactions() == !model.empty());
            },
            [&] {
                // GetBroadcastInfo() returns exactly the txs we added, each
                // carrying exactly the sends we recorded against it.
                const auto infos{pb.GetBroadcastInfo()};
                assert(infos.size() == model.size());
                for (const auto& info : infos) {
                    const auto it{model.find(info.tx->GetWitnessHash())};
                    assert(it != model.end());
                    assert(info.peers.size() == it->second);
                }
            },
            [&] {
                // Stale txs are a subset of the queue, so each must be a member.
                const auto stale{pb.GetStale()};
                assert(stale.size() <= model.size());
                for (const auto& tx : stale) {
                    assert(model.contains(tx->GetWitnessHash()));
                }
            });
    }
}
