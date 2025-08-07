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
#include <util/feefrac.h>
#include <util/time.h>

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
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

    LIMITED_WHILE(outpoints.size() < 200'000 && fuzzed_data_provider.ConsumeBool(), 1000)
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
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000)
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
                    bool have_tx_and_peer = orphanage->HaveTxFromPeer(wtxid, peer_id);
                    // AddTx should return false if tx is too big or already have it
                    // tx weight is unknown, we only check when tx is already in orphanage
                    {
                        bool add_tx = orphanage->AddTx(tx, peer_id);
                        // have_tx == true -> add_tx == false
                        Assert(!have_tx || !add_tx);
                        // have_tx_and_peer == true -> add_tx == false
                        Assert(!have_tx_and_peer || !add_tx);
                        // After AddTx, the orphanage may trim itself, so the peer's usage may have gone up or down.

                        if (add_tx) {
                            Assert(tx_weight <= MAX_STANDARD_TX_WEIGHT);
                        } else {
                            // Peer may have been added as an announcer.
                            if (orphanage->UsageByPeer(peer_id) > total_peer_bytes_start) {
                                Assert(orphanage->HaveTxFromPeer(wtxid, peer_id));
                            }

                            // If announcement was added, total bytes does not increase.
                            // However, if eviction was triggered, the value may decrease.
                            Assert(orphanage->TotalOrphanUsage() <= total_bytes_start);
                        }
                    }
                    // We are not guaranteed to have_tx after AddTx. There are a few possible reasons:
                    // - tx itself exceeds the per-peer memory usage limit, so LimitOrphans had to remove it immediately
                    // - tx itself exceeds the per-peer latency score limit, so LimitOrphans had to remove it immediately
                    // - the orphanage needed trim and all other announcements from this peer are reconsiderable
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

                        // If announcement was added, total bytes does not increase.
                        // However, if eviction was triggered, the value may decrease.
                        Assert(orphanage->TotalOrphanUsage() <= total_bytes_start);
                    }
                },
                [&] {
                    bool have_tx = orphanage->HaveTx(tx->GetWitnessHash());
                    bool have_tx_and_peer{orphanage->HaveTxFromPeer(wtxid, peer_id)};
                    // EraseTx should return 0 if m_orphans doesn't have the tx
                    {
                        auto bytes_from_peer_before{orphanage->UsageByPeer(peer_id)};
                        Assert(have_tx == orphanage->EraseTx(tx->GetWitnessHash()));
                        // After EraseTx, the orphanage may trim itself, so all peers' usage may have gone up or down.
                        if (have_tx) {
                            if (!have_tx_and_peer) {
                                Assert(orphanage->UsageByPeer(peer_id) == bytes_from_peer_before);
                            }
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
                }
            );
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
                            orphanage->LatencyScoreFromPeer(peer_id) + (tx->vin.size() / 10) + 1 > honest_latency_limit)) {
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
                }
            );
        }
    }

    orphanage->SanityCheck();
    // All of the honest peer's announcements are still present.
    for (const auto& wtxid : protected_wtxids) {
        Assert(orphanage->HaveTx(wtxid));
    }
}

FUZZ_TARGET(txorphanage_sim)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    // This is a comprehensive simulation fuzz test, which runs through a scenario involving up to
    // 16 transactions (which may have simple or complex topology, and may have duplicate txids
    // with distinct wtxids, and up to 16 peers. The scenario is performed both on a real
    // TxOrphanage object and the behavior is compared with a naive reimplementation (just a vector
    // of announcements) where possible, and tested for desired properties where not possible.

    //
    // 1. Setup.
    //

    /** The total number of transactions this simulation uses (not all of which will necessarily
     *  be present in the orphanage at once). */
    static constexpr unsigned NUM_TX = 16;
    /** The number of peers this simulation uses (not all of which will necessarily be present in
     *  the orphanage at once). */
    static constexpr unsigned NUM_PEERS = 16;
    /** The maximum number of announcements this simulation uses (which may be higher than the
     *  number permitted inside the orphanage). */
    static constexpr unsigned MAX_ANN = 64;

    FuzzedDataProvider provider(buffer.data(), buffer.size());
    /** Local RNG. Only used for topology/sizes of the transaction set, the order of transactions
     *  in EraseForBlock, and for the randomized passed to AddChildrenToWorkSet. */
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    //
    // 2. Construct an interesting set of 16 transactions.
    //

    // - Pick a topological order among the transactions.
    std::vector<unsigned> txorder(NUM_TX);
    std::iota(txorder.begin(), txorder.end(), unsigned{0});
    std::shuffle(txorder.begin(), txorder.end(), rng);
    // - Pick a set of dependencies (pair<child_index, parent_index>).
    std::vector<std::pair<unsigned, unsigned>> deps;
    deps.reserve((NUM_TX * (NUM_TX - 1)) / 2);
    for (unsigned p = 0; p < NUM_TX - 1; ++p) {
        for (unsigned c = p + 1; c < NUM_TX; ++c) {
            deps.emplace_back(c, p);
        }
    }
    std::shuffle(deps.begin(), deps.end(), rng);
    deps.resize(provider.ConsumeIntegralInRange<unsigned>(0, NUM_TX * 4 - 1));
    // - Construct the actual transactions.
    std::set<Wtxid> wtxids;
    std::vector<CTransactionRef> txn(NUM_TX);
    node::TxOrphanage::Usage total_usage{0};
    for (unsigned t = 0; t < NUM_TX; ++t) {
        CMutableTransaction tx;
        if (t > 0 && rng.randrange(4) == 0) {
            // Occasionally duplicate the previous transaction, so that repetitions of the same
            // txid are possible (with different wtxid).
            tx = CMutableTransaction(*txn[txorder[t - 1]]);
        } else {
            tx.version = 1;
            tx.nLockTime = 0xffffffff;
            // Construct 1 to 16 outputs.
            auto num_outputs = rng.randrange<unsigned>(1 << rng.randrange<unsigned>(5)) + 1;
            for (unsigned output = 0; output < num_outputs; ++output) {
                CScript scriptpubkey;
                scriptpubkey.resize(provider.ConsumeIntegralInRange<unsigned>(20, 34));
                tx.vout.emplace_back(CAmount{0}, std::move(scriptpubkey));
            }
            // Construct inputs (one for each dependency).
            for (auto& [child, parent] : deps) {
                if (child == t) {
                    auto& partx = txn[txorder[parent]];
                    assert(partx->version == 1);
                    COutPoint outpoint(partx->GetHash(), rng.randrange<size_t>(partx->vout.size()));
                    tx.vin.emplace_back(outpoint);
                    tx.vin.back().scriptSig.resize(provider.ConsumeIntegralInRange<unsigned>(16, 200));
                }
            }
            // Construct fallback input in case there are no dependencies.
            if (tx.vin.empty()) {
                COutPoint outpoint(Txid::FromUint256(rng.rand256()), rng.randrange<size_t>(16));
                tx.vin.emplace_back(outpoint);
                tx.vin.back().scriptSig.resize(provider.ConsumeIntegralInRange<unsigned>(16, 200));
            }
        }
        // Optionally modify the witness (allowing wtxid != txid), and certainly when the wtxid
        // already exists.
        while (wtxids.contains(CTransaction(tx).GetWitnessHash()) || rng.randrange(4) == 0) {
            auto& input = tx.vin[rng.randrange(tx.vin.size())];
            if (rng.randbool()) {
                input.scriptWitness.stack.resize(1);
                input.scriptWitness.stack[0].resize(rng.randrange(100));
            } else {
                input.scriptWitness.stack.resize(0);
            }
        }
        // Convert to CTransactionRef.
        txn[txorder[t]] = MakeTransactionRef(std::move(tx));
        wtxids.insert(txn[txorder[t]]->GetWitnessHash());
        auto weight = GetTransactionWeight(*txn[txorder[t]]);
        assert(weight < MAX_STANDARD_TX_WEIGHT);
        total_usage += GetTransactionWeight(*txn[txorder[t]]);
    }

    //
    // 3. Initialize real orphanage
    //

    auto max_global_latency_score = provider.ConsumeIntegralInRange<node::TxOrphanage::Count>(NUM_PEERS, MAX_ANN);
    auto reserved_peer_usage = provider.ConsumeIntegralInRange<node::TxOrphanage::Usage>(1, total_usage);
    auto real = node::MakeTxOrphanage(max_global_latency_score, reserved_peer_usage);

    //
    // 4. Functions and data structures for the simulation.
    //

    /** Data structure representing one announcement (pair of (tx, peer), plus whether it's
     *  reconsiderable or not. */
    struct SimAnnouncement
    {
        unsigned tx;
        NodeId announcer;
        bool reconsider{false};
        SimAnnouncement(unsigned tx_in, NodeId announcer_in, bool reconsider_in) noexcept :
            tx(tx_in), announcer(announcer_in), reconsider(reconsider_in) {}
    };
    /** The entire simulated orphanage is represented by this list of announcements, in
     *  announcement order (unlike TxOrphanageImpl which uses a sequence number to represent
     *  announcement order). New announcements are added to the back. */
    std::vector<SimAnnouncement> sim_announcements;

    /** Consume a transaction (index into txn) from provider. */
    auto read_tx_fn = [&]() -> unsigned { return provider.ConsumeIntegralInRange<unsigned>(0, NUM_TX - 1); };
    /** Consume a NodeId from provider. */
    auto read_peer_fn = [&]() -> NodeId { return provider.ConsumeIntegralInRange<unsigned>(0, NUM_PEERS - 1); };
    /** Consume both a transaction (index into txn) and a NodeId from provider. */
    auto read_tx_peer_fn = [&]() -> std::pair<unsigned, NodeId> {
        auto code = provider.ConsumeIntegralInRange<unsigned>(0, NUM_TX * NUM_PEERS - 1);
        return {code % NUM_TX, code / NUM_TX};
    };
    /** Determine if we have any announcements of the given transaction in the simulation. */
    auto have_tx_fn = [&](unsigned tx) -> bool {
        for (auto& ann : sim_announcements) {
            if (ann.tx == tx) return true;
        }
        return false;
    };
    /** Count the number of peers in the simulation. */
    auto count_peers_fn = [&]() -> unsigned {
        std::bitset<NUM_PEERS> mask;
        for (auto& ann : sim_announcements) {
            mask.set(ann.announcer);
        }
        return mask.count();
    };
    /** Determine if we have any reconsiderable announcements of a given transaction. */
    auto have_reconsiderable_fn = [&](unsigned tx) -> bool {
        for (auto& ann : sim_announcements) {
            if (ann.reconsider && ann.tx == tx) return true;
        }
        return false;
    };
    /** Determine if a peer has any transactions to reconsider. */
    auto have_reconsider_fn = [&](NodeId peer) -> bool {
        for (auto& ann : sim_announcements) {
            if (ann.reconsider && ann.announcer == peer) return true;
        }
        return false;
    };
    /** Get an iterator to an existing (wtxid, peer) pair in the simulation. */
    auto find_announce_wtxid_fn = [&](const Wtxid& wtxid, NodeId peer) -> std::vector<SimAnnouncement>::iterator {
        for (auto it = sim_announcements.begin(); it != sim_announcements.end(); ++it) {
            if (txn[it->tx]->GetWitnessHash() == wtxid && it->announcer == peer) return it;
        }
        return sim_announcements.end();
    };
    /** Get an iterator to an existing (tx, peer) pair in the simulation. */
    auto find_announce_fn = [&](unsigned tx, NodeId peer) {
        for (auto it = sim_announcements.begin(); it != sim_announcements.end(); ++it) {
            if (it->tx == tx && it->announcer == peer) return it;
        }
        return sim_announcements.end();
    };
    /** Compute a peer's DoS score according to simulation data. */
    auto dos_score_fn = [&](NodeId peer, int32_t max_count, int32_t max_usage) -> FeeFrac {
        int64_t count{0};
        int64_t usage{0};
        for (auto& ann : sim_announcements) {
            if (ann.announcer != peer) continue;
            count += 1 + (txn[ann.tx]->vin.size() / 10);
            usage += GetTransactionWeight(*txn[ann.tx]);
        }
        return std::max(FeeFrac{count, max_count}, FeeFrac{usage, max_usage});
    };

    //
    // 5. Run through a scenario of mutators on both real and simulated orphanage.
    //

    LIMITED_WHILE(provider.remaining_bytes() > 0, 200) {
        int command = provider.ConsumeIntegralInRange<uint8_t>(0, 15);
        while (true) {
            if (sim_announcements.size() < MAX_ANN && command-- == 0) {
                // AddTx
                auto [tx, peer] = read_tx_peer_fn();
                bool added = real->AddTx(txn[tx], peer);
                bool sim_have_tx = have_tx_fn(tx);
                assert(added == !sim_have_tx);
                if (find_announce_fn(tx, peer) == sim_announcements.end()) {
                    sim_announcements.emplace_back(tx, peer, false);
                }
                break;
            } else if (sim_announcements.size() < MAX_ANN && command-- == 0) {
                // AddAnnouncer
                auto [tx, peer] = read_tx_peer_fn();
                bool added = real->AddAnnouncer(txn[tx]->GetWitnessHash(), peer);
                bool sim_have_tx = have_tx_fn(tx);
                auto sim_it = find_announce_fn(tx, peer);
                assert(added == (sim_it == sim_announcements.end() && sim_have_tx));
                if (added) {
                    sim_announcements.emplace_back(tx, peer, false);
                }
                break;
            } else if (command-- == 0) {
                // EraseTx
                auto tx = read_tx_fn();
                bool erased = real->EraseTx(txn[tx]->GetWitnessHash());
                bool sim_have = have_tx_fn(tx);
                assert(erased == sim_have);
                std::erase_if(sim_announcements, [&](auto& ann) { return ann.tx == tx; });
                break;
           } else if (command-- == 0) {
                // EraseForPeer
                auto peer = read_peer_fn();
                real->EraseForPeer(peer);
                std::erase_if(sim_announcements, [&](auto& ann) { return ann.announcer == peer; });
                break;
            } else if (command-- == 0) {
                // EraseForBlock
                auto pattern = provider.ConsumeIntegralInRange<uint64_t>(0, (uint64_t{1} << NUM_TX) - 1);
                CBlock block;
                std::set<COutPoint> spent;
                for (unsigned tx = 0; tx < NUM_TX; ++tx) {
                    if ((pattern >> tx) & 1) {
                        block.vtx.emplace_back(txn[tx]);
                        for (auto& txin : block.vtx.back()->vin) {
                            spent.insert(txin.prevout);
                        }
                    }
                }
                std::shuffle(block.vtx.begin(), block.vtx.end(), rng);
                real->EraseForBlock(block);
                std::erase_if(sim_announcements, [&](auto& ann) {
                    for (auto& txin : txn[ann.tx]->vin) {
                        if (spent.count(txin.prevout)) return true;
                    }
                    return false;
                });
                break;
            } else if (command-- == 0) {
                // AddChildrenToWorkSet
                auto tx = read_tx_fn();
                FastRandomContext rand_ctx(rng.rand256());
                auto added = real->AddChildrenToWorkSet(*txn[tx], rand_ctx);
                /** Set of not-already-reconsiderable child wtxids. */
                std::set<Wtxid> child_wtxids;
                for (unsigned child_tx = 0; child_tx < NUM_TX; ++child_tx) {
                    if (!have_tx_fn(child_tx)) continue;
                    if (have_reconsiderable_fn(child_tx)) continue;
                    bool child_of = false;
                    for (auto& txin : txn[child_tx]->vin) {
                        if (txin.prevout.hash == txn[tx]->GetHash()) {
                            child_of = true;
                            break;
                        }
                    }
                    if (child_of) {
                        child_wtxids.insert(txn[child_tx]->GetWitnessHash());
                    }
                }
                for (auto& [wtxid, peer] : added) {
                    // Wtxid must be a child of tx that is not yet reconsiderable.
                    auto child_wtxid_it = child_wtxids.find(wtxid);
                    assert(child_wtxid_it != child_wtxids.end());
                    // Announcement must exist.
                    auto sim_ann_it = find_announce_wtxid_fn(wtxid, peer);
                    assert(sim_ann_it != sim_announcements.end());
                    // Announcement must not yet be reconsiderable.
                    assert(sim_ann_it->reconsider == false);
                    // Make reconsiderable.
                    sim_ann_it->reconsider = true;
                    // Remove from child_wtxids map, to disallow it being reported a second time in added.
                    child_wtxids.erase(wtxid);
                }
                // Verify that AddChildrenToWorkSet does not select announcements that were already reconsiderable:
                // Check all child wtxids which did not occur at least once in the result were already reconsiderable
                // due to a previous AddChildrenToWorkSet.
                assert(child_wtxids.empty());
                break;
            } else if (command-- == 0) {
                // GetTxToReconsider.
                auto peer = read_peer_fn();
                auto result = real->GetTxToReconsider(peer);
                if (result) {
                    // A transaction was found. It must have a corresponding reconsiderable
                    // announcement from peer.
                    auto sim_ann_it = find_announce_wtxid_fn(result->GetWitnessHash(), peer);
                    assert(sim_ann_it != sim_announcements.end());
                    assert(sim_ann_it->announcer == peer);
                    assert(sim_ann_it->reconsider);
                    // Make it non-reconsiderable.
                    sim_ann_it->reconsider = false;
                } else {
                    // No reconsiderable transaction was found from peer. Verify that it does not
                    // have any.
                    assert(!have_reconsider_fn(peer));
                }
                break;
            }
        }
        // Always trim after each command if needed.
        const auto max_ann = max_global_latency_score / std::max<unsigned>(1, count_peers_fn());
        const auto max_mem = reserved_peer_usage;
        while (true) {
            // Count global usage and number of peers.
            node::TxOrphanage::Usage total_usage{0};
            node::TxOrphanage::Count total_latency_score = sim_announcements.size();
            for (unsigned tx = 0; tx < NUM_TX; ++tx) {
                if (have_tx_fn(tx)) {
                    total_usage += GetTransactionWeight(*txn[tx]);
                    total_latency_score += txn[tx]->vin.size() / 10;
                }
            }
            auto num_peers = count_peers_fn();
            bool oversized = (total_usage > reserved_peer_usage * num_peers) ||
                                (total_latency_score > real->MaxGlobalLatencyScore());
            if (!oversized) break;
            // Find worst peer.
            FeeFrac worst_dos_score{0, 1};
            unsigned worst_peer = unsigned(-1);
            for (unsigned peer = 0; peer < NUM_PEERS; ++peer) {
                auto dos_score = dos_score_fn(peer, max_ann, max_mem);
                // Use >= so that the more recent peer (higher NodeId) wins in case of
                // ties.
                if (dos_score >= worst_dos_score) {
                    worst_dos_score = dos_score;
                    worst_peer = peer;
                }
            }
            assert(worst_peer != unsigned(-1));
            assert(worst_dos_score >> FeeFrac(1, 1));
            // Find oldest announcement from worst_peer, preferring non-reconsiderable ones.
            bool done{false};
            for (int reconsider = 0; reconsider < 2; ++reconsider) {
                for (auto it = sim_announcements.begin(); it != sim_announcements.end(); ++it) {
                    if (it->announcer != worst_peer || it->reconsider != reconsider) continue;
                    sim_announcements.erase(it);
                    done = true;
                    break;
                }
                if (done) break;
            }
            assert(done);
        }
        // We must now be within limits, otherwise LimitOrphans should have continued further.
        // We don't check the contents of the orphanage until the end to make fuzz runs faster.
        assert(real->TotalLatencyScore() <= real->MaxGlobalLatencyScore());
        assert(real->TotalOrphanUsage() <= real->MaxGlobalUsage());
    }

    //
    // 6. Perform a full comparison between the real orphanage's inspectors and the simulation.
    //

    real->SanityCheck();


    auto all_orphans = real->GetOrphanTransactions();
    node::TxOrphanage::Usage orphan_usage{0};
    std::vector<node::TxOrphanage::Usage> usage_by_peer(NUM_PEERS);
    node::TxOrphanage::Count unique_orphans{0};
    std::vector<node::TxOrphanage::Count> count_by_peer(NUM_PEERS);
    node::TxOrphanage::Count total_latency_score = sim_announcements.size();
    for (unsigned tx = 0; tx < NUM_TX; ++tx) {
        bool sim_have_tx = have_tx_fn(tx);
        if (sim_have_tx) {
            orphan_usage += GetTransactionWeight(*txn[tx]);
            total_latency_score += txn[tx]->vin.size() / 10;
        }
        unique_orphans += sim_have_tx;
        auto orphans_it = std::find_if(all_orphans.begin(), all_orphans.end(), [&](auto& orph) { return orph.tx->GetWitnessHash() == txn[tx]->GetWitnessHash(); });
        // GetOrphanTransactions (OrphanBase existence)
        assert((orphans_it != all_orphans.end()) == sim_have_tx);
        // HaveTx
        bool have_tx = real->HaveTx(txn[tx]->GetWitnessHash());
        assert(have_tx == sim_have_tx);
        // GetTx
        auto txref = real->GetTx(txn[tx]->GetWitnessHash());
        assert(!!txref == sim_have_tx);
        if (sim_have_tx) assert(txref->GetWitnessHash() == txn[tx]->GetWitnessHash());

        for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
            auto it_sim_ann = find_announce_fn(tx, peer);
            bool sim_have_ann = it_sim_ann != sim_announcements.end();
            if (sim_have_ann) usage_by_peer[peer] += GetTransactionWeight(*txn[tx]);
            count_by_peer[peer] += sim_have_ann;
            // GetOrphanTransactions (announcers presence)
            if (sim_have_ann) assert(sim_have_tx);
            if (sim_have_tx) assert(orphans_it->announcers.count(peer) == sim_have_ann);
            // HaveTxFromPeer
            bool have_ann = real->HaveTxFromPeer(txn[tx]->GetWitnessHash(), peer);
            assert(sim_have_ann == have_ann);
            // GetChildrenFromSamePeer
            auto children_from_peer = real->GetChildrenFromSamePeer(txn[tx], peer);
            auto it = children_from_peer.rbegin();
            for (int phase = 0; phase < 2; ++phase) {
                // First expect all children which have reconsiderable announcement from peer, then the others.
                for (auto& ann : sim_announcements) {
                    if (ann.announcer != peer) continue;
                    if (ann.reconsider != (phase == 1)) continue;
                    bool matching_parent{false};
                    for (const auto& vin : txn[ann.tx]->vin) {
                        if (vin.prevout.hash == txn[tx]->GetHash()) matching_parent = true;
                    }
                    if (!matching_parent) continue;
                    // Found an announcement from peer which is a child of txn[tx].
                    assert(it != children_from_peer.rend());
                    assert((*it)->GetWitnessHash() == txn[ann.tx]->GetWitnessHash());
                    ++it;
                }
            }
            assert(it == children_from_peer.rend());
        }
    }
    // TotalOrphanUsage
    assert(orphan_usage == real->TotalOrphanUsage());
    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        bool sim_have_reconsider = have_reconsider_fn(peer);
        // HaveTxToReconsider
        bool have_reconsider = real->HaveTxToReconsider(peer);
        assert(have_reconsider == sim_have_reconsider);
        // UsageByPeer
        assert(usage_by_peer[peer] == real->UsageByPeer(peer));
        // AnnouncementsFromPeer
        assert(count_by_peer[peer] == real->AnnouncementsFromPeer(peer));
    }
    // CountAnnouncements
    assert(sim_announcements.size() == real->CountAnnouncements());
    // CountUniqueOrphans
    assert(unique_orphans == real->CountUniqueOrphans());
    // MaxGlobalLatencyScore
    assert(max_global_latency_score == real->MaxGlobalLatencyScore());
    // ReservedPeerUsage
    assert(reserved_peer_usage == real->ReservedPeerUsage());
    // MaxPeerLatencyScore
    auto present_peers = count_peers_fn();
    assert(max_global_latency_score / std::max<unsigned>(1, present_peers) == real->MaxPeerLatencyScore());
    // MaxGlobalUsage
    assert(reserved_peer_usage * std::max<unsigned>(1, present_peers) == real->MaxGlobalUsage());
    // TotalLatencyScore.
    assert(real->TotalLatencyScore() == total_latency_score);
}
