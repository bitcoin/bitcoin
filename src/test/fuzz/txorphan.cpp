// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <net_processing.h>
#include <node/eviction.h>
#include <node/txorphanage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <bitset>
#include <cmath>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

void initialize_orphanage()
{
    static const auto testing_setup = MakeNoLogFileContext();
}

FUZZ_TARGET(txorphan, .init = initialize_orphanage)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FastRandomContext orphanage_rng{ConsumeUInt256(fuzzed_data_provider)};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    auto orphanage = node::MakeTxOrphanage();
    std::vector<COutPoint> outpoints; // Duplicates are tolerated
    outpoints.reserve(200'000);

    // initial outpoints used to construct transactions later
    for (uint8_t i = 0; i < 4; i++) {
        outpoints.emplace_back(Txid::FromUint256(uint256{i}), 0);
    }

    CTransactionRef ptx_potential_parent = nullptr;

    std::vector<CTransactionRef> tx_history;

    LIMITED_WHILE(outpoints.size() < 200'000 && fuzzed_data_provider.ConsumeBool(), 10 * node::DEFAULT_MAX_ORPHAN_TRANSACTIONS)
    {
        // construct transaction
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, outpoints.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 256);
            // pick outpoints from outpoints as input. We allow input duplicates on purpose, given we are not
            // running any transaction validation logic before adding transactions to the orphanage
            tx_mut.vin.reserve(num_in);
            for (uint32_t i = 0; i < num_in; i++) {
                auto& prevout = PickValue(fuzzed_data_provider, outpoints);
                // try making transactions unique by setting a random nSequence, but allow duplicate transactions if they happen
                tx_mut.vin.emplace_back(prevout, CScript{}, fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, CTxIn::SEQUENCE_FINAL));
            }
            // output amount will not affect txorphanage
            tx_mut.vout.reserve(num_out);
            for (uint32_t i = 0; i < num_out; i++) {
                tx_mut.vout.emplace_back(CAmount{0}, CScript{});
            }
            auto new_tx = MakeTransactionRef(tx_mut);
            // add newly constructed outpoints to the coin pool
            for (uint32_t i = 0; i < num_out; i++) {
                outpoints.emplace_back(new_tx->GetHash(), i);
            }
            return new_tx;
        }();

        tx_history.push_back(tx);

        const auto wtxid{tx->GetWitnessHash()};

        // Trigger orphanage functions that are called using parents. ptx_potential_parent is a tx we constructed in a
        // previous loop and potentially the parent of this tx.
        if (ptx_potential_parent) {
            // Set up future GetTxToReconsider call.
            orphanage->AddChildrenToWorkSet(*ptx_potential_parent, orphanage_rng);

            // Check that all txns returned from GetChildrenFrom* are indeed a direct child of this tx.
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegral<NodeId>();
            for (const auto& child : orphanage->GetChildrenFromSamePeer(ptx_potential_parent, peer_id)) {
                assert(std::any_of(child->vin.cbegin(), child->vin.cend(), [&](const auto& input) {
                    return input.prevout.hash == ptx_potential_parent->GetHash();
                }));
            }
        }

        // trigger orphanage functions
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10 * node::DEFAULT_MAX_ORPHAN_TRANSACTIONS)
        {
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegral<NodeId>();
            const auto total_bytes_start{orphanage->TotalOrphanUsage()};
            const auto total_peer_bytes_start{orphanage->UsageByPeer(peer_id)};
            const auto tx_weight{GetTransactionWeight(*tx)};

            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    {
                        CTransactionRef ref = orphanage->GetTxToReconsider(peer_id);
                        if (ref) {
                            Assert(orphanage->HaveTx(ref->GetWitnessHash()));
                        }
                    }
                },
                [&] {
                    bool have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    // AddTx should return false if tx is too big or already have it
                    // tx weight is unknown, we only check when tx is already in orphanage
                    {
                        bool add_tx = orphanage->AddTx(tx, peer_id);
                        // have_tx == true -> add_tx == false
                        Assert(!have_tx || !add_tx);

                        if (add_tx) {
                            Assert(orphanage->UsageByPeer(peer_id) == tx_weight + total_peer_bytes_start);
                            Assert(orphanage->TotalOrphanUsage() == tx_weight + total_bytes_start);
                            Assert(tx_weight <= MAX_STANDARD_TX_WEIGHT);
                        } else {
                            // Peer may have been added as an announcer.
                            if (orphanage->UsageByPeer(peer_id) == tx_weight + total_peer_bytes_start) {
                                Assert(orphanage->HaveTxFromPeer(wtxid, peer_id));
                            } else {
                                // Otherwise, there must not be any change to the peer byte count.
                                Assert(orphanage->UsageByPeer(peer_id) == total_peer_bytes_start);
                            }

                            // Regardless, total bytes should not have changed.
                            Assert(orphanage->TotalOrphanUsage() == total_bytes_start);
                        }
                    }
                    have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    {
                        bool add_tx = orphanage->AddTx(tx, peer_id);
                        // if have_tx is still false, it must be too big
                        Assert(!have_tx == (tx_weight > MAX_STANDARD_TX_WEIGHT));
                        Assert(!have_tx || !add_tx);
                    }
                },
                [&] {
                    bool have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    bool have_tx_and_peer = orphanage->HaveTxFromPeer(tx->GetWitnessHash(), peer_id);
                    // AddAnnouncer should return false if tx doesn't exist or we already HaveTxFromPeer.
                    {
                        bool added_announcer = orphanage->AddAnnouncer(tx->GetWitnessHash(), peer_id);
                        // have_tx == false -> added_announcer == false
                        Assert(have_tx || !added_announcer);
                        // have_tx_and_peer == true -> added_announcer == false
                        Assert(!have_tx_and_peer || !added_announcer);

                        // Total bytes should not have changed. If peer was added as announcer, byte
                        // accounting must have been updated.
                        Assert(orphanage->TotalOrphanUsage() == total_bytes_start);
                        if (added_announcer) {
                            Assert(orphanage->UsageByPeer(peer_id) == tx_weight + total_peer_bytes_start);
                        } else {
                            Assert(orphanage->UsageByPeer(peer_id) == total_peer_bytes_start);
                        }
                    }
                },
                [&] {
                    bool have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    bool have_tx_and_peer{orphanage->HaveTxFromPeer(wtxid, peer_id)};
                    // EraseTx should return 0 if m_orphans doesn't have the tx
                    {
                        auto bytes_from_peer_before{orphanage->UsageByPeer(peer_id)};
                        Assert(have_tx == orphanage->EraseTx(tx->GetWitnessHash()));
                        if (have_tx) {
                            Assert(orphanage->TotalOrphanUsage() == total_bytes_start - tx_weight);
                            if (have_tx_and_peer) {
                                Assert(orphanage->UsageByPeer(peer_id) == bytes_from_peer_before - tx_weight);
                            } else {
                                Assert(orphanage->UsageByPeer(peer_id) == bytes_from_peer_before);
                            }
                        } else {
                            Assert(orphanage->TotalOrphanUsage() == total_bytes_start);
                        }
                    }
                    have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    have_tx_and_peer = orphanage->HaveTxFromPeer(wtxid, peer_id);
                    // have_tx should be false and EraseTx should fail
                    {
                        Assert(!have_tx && !have_tx_and_peer && !orphanage->EraseTx(wtxid));
                    }
                },
                [&] {
                    orphanage->EraseForPeer(peer_id);
                    Assert(!orphanage->HaveTxFromPeer(tx->GetWitnessHash(), peer_id));
                    Assert(orphanage->UsageByPeer(peer_id) == 0);
                },
                [&] {
                    // Make a block out of txs and then EraseForBlock
                    CBlock block;
                    int num_txs = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1000);
                    for (int i{0}; i < num_txs; ++i) {
                        auto& tx_to_remove = PickValue(fuzzed_data_provider, tx_history);
                        block.vtx.push_back(tx_to_remove);
                    }
                    orphanage->EraseForBlock(block);
                    for (const auto& tx_removed : block.vtx) {
                        Assert(!orphanage->HaveTx(tx_removed->GetWitnessHash()));
                        Assert(!orphanage->HaveTxFromPeer(tx_removed->GetWitnessHash(), peer_id));
                    }
                },
                [&] {
                    // test mocktime and expiry
                    SetMockTime(ConsumeTime(fuzzed_data_provider));
                    orphanage->LimitOrphans();
                    Assert(orphanage->Size() <= node::DEFAULT_MAX_ORPHAN_TRANSACTIONS);
                });

        }

        // Set tx as potential parent to be used for future GetChildren() calls.
        if (!ptx_potential_parent || fuzzed_data_provider.ConsumeBool()) {
            ptx_potential_parent = tx;
        }

        const bool have_tx{orphanage->HaveTx(tx->GetWitnessHash())};
        const bool get_tx_nonnull{orphanage->GetTx(tx->GetWitnessHash()) != nullptr};
        Assert(have_tx == get_tx_nonnull);
    }
    orphanage->SanityCheck();
}

FUZZ_TARGET(txorphan_protected, .init = initialize_orphanage)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FastRandomContext orphanage_rng{ConsumeUInt256(fuzzed_data_provider)};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    // We have num_peers peers. Some subset of them will never exceed their reserved weight or announcement count, and
    // should therefore never have any orphans evicted.
    const unsigned int MAX_PEERS = 125;
    const unsigned int num_peers = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, MAX_PEERS);
    // Generate a vector of bools for whether each peer is protected from eviction
    std::bitset<MAX_PEERS> protected_peers;
    for (unsigned int i = 0; i < num_peers; i++) {
        protected_peers.set(i, fuzzed_data_provider.ConsumeBool());
    }

    // Params for orphanage.
    const unsigned int global_latency_score_limit = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(num_peers, 6'000);
    const int64_t per_peer_weight_reservation = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 4'040'000);
    auto orphanage = node::MakeTxOrphanage(global_latency_score_limit, per_peer_weight_reservation);

    // The actual limit, MaxPeerLatencyScore(), may be higher, since TxOrphanage only counts peers
    // that have announced an orphan. The honest peer will not experience evictions if it never
    // exceeds this.
    const unsigned int honest_latency_limit = global_latency_score_limit / num_peers;
    // Honest peer will not experience evictions if it never exceeds this.
    const int64_t honest_mem_limit = per_peer_weight_reservation;

    std::vector<COutPoint> outpoints; // Duplicates are tolerated
    outpoints.reserve(400);

    // initial outpoints used to construct transactions later
    for (uint8_t i = 0; i < 4; i++) {
        outpoints.emplace_back(Txid::FromUint256(uint256{i}), 0);
    }

    // These are honest peer's live announcements. We expect them to be protected from eviction.
    std::set<Wtxid> protected_wtxids;

    LIMITED_WHILE(outpoints.size() < 400 && fuzzed_data_provider.ConsumeBool(), 1000)
    {
        // construct transaction
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, outpoints.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 256);
            // pick outpoints from outpoints as input. We allow input duplicates on purpose, given we are not
            // running any transaction validation logic before adding transactions to the orphanage
            tx_mut.vin.reserve(num_in);
            for (uint32_t i = 0; i < num_in; i++) {
                auto& prevout = PickValue(fuzzed_data_provider, outpoints);
                // try making transactions unique by setting a random nSequence, but allow duplicate transactions if they happen
                tx_mut.vin.emplace_back(prevout, CScript{}, fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, CTxIn::SEQUENCE_FINAL));
            }
            // output amount or spendability will not affect txorphanage
            tx_mut.vout.reserve(num_out);
            for (uint32_t i = 0; i < num_out; i++) {
                const auto payload_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 100000);
                if (payload_size) {
                    tx_mut.vout.emplace_back(0, CScript() << OP_RETURN << std::vector<unsigned char>(payload_size));
                } else {
                    tx_mut.vout.emplace_back(0, CScript{});
                }
            }
            auto new_tx = MakeTransactionRef(tx_mut);
            // add newly constructed outpoints to the coin pool
            for (uint32_t i = 0; i < num_out; i++) {
                outpoints.emplace_back(new_tx->GetHash(), i);
            }
            return new_tx;
        }();

        const auto wtxid{tx->GetWitnessHash()};

        // orphanage functions
        LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 10 * global_latency_score_limit)
        {
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegralInRange<NodeId>(0, num_peers - 1);
            const auto tx_weight{GetTransactionWeight(*tx)};

            // This protected peer will never send orphans that would
            // exceed their own personal allotment, so is never evicted.
            const bool peer_is_protected{protected_peers[peer_id]};

            CallOneOf(
                fuzzed_data_provider,
                [&] { // AddTx
                    bool have_tx_and_peer = orphanage->HaveTxFromPeer(wtxid, peer_id);
                    if (peer_is_protected && !have_tx_and_peer &&
                        (orphanage->UsageByPeer(peer_id) + tx_weight > honest_mem_limit ||
                        orphanage->LatencyScoreFromPeer(peer_id) + (tx->vin.size() / 10) + 1 > honest_latency_limit)) {
                        // We never want our protected peer oversized or over-announced
                    } else {
                        orphanage->AddTx(tx, peer_id);
                        if (peer_is_protected && orphanage->HaveTxFromPeer(wtxid, peer_id)) {
                            protected_wtxids.insert(wtxid);
                        }
                    }
                },
                [&] { // AddAnnouncer
                    bool have_tx_and_peer = orphanage->HaveTxFromPeer(tx->GetWitnessHash(), peer_id);
                    // AddAnnouncer should return false if tx doesn't exist or we already HaveTxFromPeer.
                    {
                        if (peer_is_protected && !have_tx_and_peer &&
                            (orphanage->UsageByPeer(peer_id) + tx_weight > honest_mem_limit ||
                            orphanage->LatencyScoreFromPeer(peer_id) + (tx->vin.size()) + 1 > honest_latency_limit)) {
                            // We never want our protected peer oversized
                        } else {
                            orphanage->AddAnnouncer(tx->GetWitnessHash(), peer_id);
                            if (peer_is_protected && orphanage->HaveTxFromPeer(wtxid, peer_id)) {
                                protected_wtxids.insert(wtxid);
                            }
                        }
                    }
                },
                [&] { // EraseTx
                    if (protected_wtxids.count(tx->GetWitnessHash())) {
                        protected_wtxids.erase(wtxid);
                    }
                    orphanage->EraseTx(wtxid);
                    Assert(!orphanage->HaveTx(wtxid));
                },
                [&] { // EraseForPeer
                    if (!protected_peers[peer_id]) {
                        orphanage->EraseForPeer(peer_id);
                        Assert(orphanage->UsageByPeer(peer_id) == 0);
                        Assert(orphanage->LatencyScoreFromPeer(peer_id) == 0);
                        Assert(orphanage->AnnouncementsFromPeer(peer_id) == 0);
                    }
                },
                [&] { // LimitOrphans
                    // Assert that protected peers are never affected by LimitOrphans.
                    unsigned int protected_count = 0;
                    unsigned int protected_bytes = 0;
                    for (unsigned int peer = 0; peer < num_peers; ++peer) {
                        if (protected_peers[peer]) {
                            protected_count += orphanage->LatencyScoreFromPeer(peer);
                            protected_bytes += orphanage->UsageByPeer(peer);
                        }
                    }
                    orphanage->LimitOrphans();
                    Assert(orphanage->TotalLatencyScore() <= global_latency_score_limit);
                    Assert(orphanage->TotalOrphanUsage() <= per_peer_weight_reservation * num_peers);

                    // Number of announcements and usage should never differ before and after since
                    // we've never exceeded the per-peer reservations.
                    for (unsigned int peer = 0; peer < num_peers; ++peer) {
                        if (protected_peers[peer]) {
                            protected_count -= orphanage->LatencyScoreFromPeer(peer);
                            protected_bytes -= orphanage->UsageByPeer(peer);
                        }
                    }
                    Assert(protected_count == 0);
                    Assert(protected_bytes == 0);
                });

        }
    }

    orphanage->SanityCheck();
    // All of the honest peer's announcements are still present.
    for (const auto& wtxid : protected_wtxids) {
        Assert(orphanage->HaveTx(wtxid));
    }
}
