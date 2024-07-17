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

void initialize_orphanage()
{
    static const auto testing_setup = MakeNoLogFileContext();
}

FUZZ_TARGET(txorphan, .init = initialize_orphanage)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FastRandomContext limit_orphans_rng{/*fDeterministic=*/true};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    TxOrphanage orphanage;
    std::vector<COutPoint> outpoints; // Duplicates are tolerated

    // initial outpoints used to construct transactions later
    for (uint8_t i = 0; i < 4; i++) {
        outpoints.emplace_back(Txid::FromUint256(uint256{i}), 0);
    }

    CTransactionRef ptx_potential_parent = nullptr;

    LIMITED_WHILE(outpoints.size() < 200'000 && fuzzed_data_provider.ConsumeBool(), 10 * DEFAULT_MAX_ORPHAN_TRANSACTIONS)
    {
        // construct transaction
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, outpoints.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 256);
            // pick outpoints from outpoints as input. We allow input duplicates on purpose, given we are not
            // running any transaction validation logic before adding transactions to the orphanage
            for (uint32_t i = 0; i < num_in; i++) {
                auto& prevout = PickValue(fuzzed_data_provider, outpoints);
                // try making transactions unique by setting a random nSequence, but allow duplicate transactions if they happen
                tx_mut.vin.emplace_back(prevout, CScript{}, fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, CTxIn::SEQUENCE_FINAL));
            }
            // output amount will not affect txorphanage
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

        // Trigger orphanage functions that are called using parents. ptx_potential_parent is a tx we constructed in a
        // previous loop and potentially the parent of this tx.
        if (ptx_potential_parent) {
            // Set up future GetTxToReconsider call.
            orphanage.AddChildrenToWorkSet(*ptx_potential_parent);

            // Check that all txns returned from GetChildrenFrom* are indeed a direct child of this tx.
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegral<NodeId>();
            for (const auto& child : orphanage.GetChildrenFromSamePeer(ptx_potential_parent, peer_id)) {
                assert(std::any_of(child->vin.cbegin(), child->vin.cend(), [&](const auto& input) {
                    return input.prevout.hash == ptx_potential_parent->GetHash();
                }));
            }
            for (const auto& [child, peer] : orphanage.GetChildrenFromDifferentPeer(ptx_potential_parent, peer_id)) {
                assert(std::any_of(child->vin.cbegin(), child->vin.cend(), [&](const auto& input) {
                    return input.prevout.hash == ptx_potential_parent->GetHash();
                }));
                assert(peer != peer_id);
            }
        }

        // trigger orphanage functions
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10 * DEFAULT_MAX_ORPHAN_TRANSACTIONS)
        {
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegral<NodeId>();

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
                    bool have_tx = orphanage.HaveTx(tx->GetWitnessHash());
                    // AddTx should return false if tx is too big or already have it
                    // tx weight is unknown, we only check when tx is already in orphanage
                    {
                        bool add_tx = orphanage.AddTx(tx, peer_id);
                        // have_tx == true -> add_tx == false
                        Assert(!have_tx || !add_tx);
                    }
                    have_tx = orphanage.HaveTx(tx->GetWitnessHash());
                    {
                        bool add_tx = orphanage.AddTx(tx, peer_id);
                        // if have_tx is still false, it must be too big
                        Assert(!have_tx == (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT));
                        Assert(!have_tx || !add_tx);
                    }
                },
                [&] {
                    bool have_tx = orphanage.HaveTx(tx->GetWitnessHash());
                    // EraseTx should return 0 if m_orphans doesn't have the tx
                    {
                        Assert(have_tx == orphanage.EraseTx(tx->GetWitnessHash()));
                    }
                    have_tx = orphanage.HaveTx(tx->GetWitnessHash());
                    // have_tx should be false and EraseTx should fail
                    {
                        Assert(!have_tx && !orphanage.EraseTx(tx->GetWitnessHash()));
                    }
                },
                [&] {
                    orphanage.EraseForPeer(peer_id);
                },
                [&] {
                    // test mocktime and expiry
                    SetMockTime(ConsumeTime(fuzzed_data_provider));
                    auto limit = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
                    orphanage.LimitOrphans(limit, limit_orphans_rng);
                    Assert(orphanage.Size() <= limit);
                });

        }
        // Set tx as potential parent to be used for future GetChildren() calls.
        if (!ptx_potential_parent || fuzzed_data_provider.ConsumeBool()) {
            ptx_potential_parent = tx;
        }

    }
}
