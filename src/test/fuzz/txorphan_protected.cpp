// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <net_processing.h>
#include <node/eviction.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <txorphanage.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

void initialize_protected_orphanage()
{
    static const auto testing_setup = MakeNoLogFileContext();
}

FUZZ_TARGET(txorphan_protected, .init = initialize_protected_orphanage)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FastRandomContext orphanage_rng{/*fDeterministic=*/true};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    // Peer that must have orphans protected from eviction
    NodeId protect_peer{0};

    // We have NUM_PEERS, of which Peer==0 is the "honest" one
    // who will never exceed their reserved weight of announcement
    // count, and should therefore never be evicted.
    const unsigned int NUM_PEERS = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 125);
    const unsigned int global_announcement_limit = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(NUM_PEERS, 1'000);
    // This must match announcement limit (or exceed) otherwise "honest" peer can be evicted
    const unsigned int global_tx_limit = global_announcement_limit;
    const unsigned int per_peer_announcements = global_announcement_limit / NUM_PEERS;
    const unsigned int per_peer_weight_reservation = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 4'000);

    // As long as peer never exceeds per_peer_weight_reservation weight units or per_peer_announcements, peer should
    // not be evicted
    TxOrphanage orphanage{per_peer_weight_reservation, global_announcement_limit};

    std::vector<COutPoint> outpoints; // Duplicates are tolerated
    outpoints.reserve(200'000);

    // initial outpoints used to construct transactions later
    for (uint8_t i = 0; i < 4; i++) {
        outpoints.emplace_back(Txid::FromUint256(uint256{i}), 0);
    }

    CTransactionRef ptx_potential_parent = nullptr;

    LIMITED_WHILE(outpoints.size() < 200'000 && fuzzed_data_provider.ConsumeBool(), 10 * global_tx_limit)
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

        const auto wtxid{tx->GetWitnessHash()};

        // Trigger orphanage functions that are called using parents. ptx_potential_parent is a tx we constructed in a
        // previous loop and potentially the parent of this tx.
        if (ptx_potential_parent) {
            // Set up future GetTxToReconsider call.
            orphanage.AddChildrenToWorkSet(*ptx_potential_parent, orphanage_rng);

            // Check that all txns returned from GetChildrenFrom* are indeed a direct child of this tx.
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegral<NodeId>();
            for (const auto& child : orphanage.GetChildrenFromSamePeer(ptx_potential_parent, peer_id)) {
                assert(std::any_of(child->vin.cbegin(), child->vin.cend(), [&](const auto& input) {
                    return input.prevout.hash == ptx_potential_parent->GetHash();
                }));
            }
        }

        // trigger orphanage functions
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10 * global_tx_limit)
        {
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegralInRange<NodeId>(0, NUM_PEERS - 1);
            const auto tx_weight{GetTransactionWeight(*tx)};

            // This protected peer will never send orphans that would
            // exceed their own personal allotment, so excluding
            // time bumps, his orphans will never be evicted.
            bool peer_is_protected{peer_id == protect_peer};

            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    {
                        CTransactionRef ref = orphanage.GetTxToReconsider(peer_id);
                        if (ref) {
                            Assert(orphanage.HaveTx(ref->GetWitnessHash()));
                        }
                    }
                },
                [&] {
                    bool have_tx_and_peer = orphanage.HaveTxFromPeer(wtxid, peer_id);
                    if (peer_is_protected && !have_tx_and_peer &&
                        (orphanage.UsageByPeer(peer_id) + tx_weight > per_peer_weight_reservation ||
                        orphanage.AnnouncementsByPeer(peer_id) + 1 > per_peer_announcements)) {
                        // We never want our protected peer oversized or over-announced
                    } else {
                        orphanage.AddTx(tx, peer_id);
                    }
                },
                [&] {
                    bool have_tx_and_peer = orphanage.HaveTxFromPeer(tx->GetWitnessHash(), peer_id);
                    // AddAnnouncer should return false if tx doesn't exist or we already HaveTxFromPeer.
                    {
                        if (peer_is_protected && !have_tx_and_peer &&
                            (orphanage.UsageByPeer(peer_id) + tx_weight > per_peer_weight_reservation ||
                            orphanage.AnnouncementsByPeer(peer_id) + 1 > per_peer_announcements)) {
                            // We never want our protected peer oversized
                        } else {
                            orphanage.AddAnnouncer(tx->GetWitnessHash(), peer_id);
                        }
                    }
                },
                [&] {
                    if (peer_id != protect_peer) {
                        orphanage.EraseForPeer(peer_id);
                    }
                },
                [&] {
                    // Assert that protected peer is never affected by LimitOrphans,
                    // excluding expiry.
                    const auto protected_bytes{orphanage.UsageByPeer(protect_peer)};
                    orphanage.LimitOrphans(/*max_orphans=*/global_tx_limit, orphanage_rng);
                    Assert(orphanage.Size() <= global_tx_limit);
                    // This should never differ before and after since we aren't allowing
                    // expiries and we've never exceeded the per-peer reservations.
                    Assert(protected_bytes == orphanage.UsageByPeer(protect_peer));
                });

        }
        // Set tx as potential parent to be used for future GetChildren() calls.
        if (!ptx_potential_parent || fuzzed_data_provider.ConsumeBool()) {
            ptx_potential_parent = tx;
        }
    }

    orphanage.SanityCheck();
}
