// Copyright (c) 2022 The Bitcoin Core developers
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
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    TxOrphanage orphanage;
    std::vector<COutPoint> outpoints;
    // initial outpoints used to construct transactions later
    for (uint8_t i = 0; i < 4; i++) {
        outpoints.emplace_back(uint256{i}, 0);
    }
    // if true, allow duplicate input when constructing tx
    const bool duplicate_input = fuzzed_data_provider.ConsumeBool();

    LIMITED_WHILE(outpoints.size() < 200'000 && fuzzed_data_provider.ConsumeBool(), 10 * DEFAULT_MAX_ORPHAN_TRANSACTIONS)
    {
        // construct transaction
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, outpoints.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, outpoints.size());
            // pick unique outpoints from outpoints as input
            for (uint32_t i = 0; i < num_in; i++) {
                auto& prevout = PickValue(fuzzed_data_provider, outpoints);
                tx_mut.vin.emplace_back(prevout);
                // pop the picked outpoint if duplicate input is not allowed
                if (!duplicate_input) {
                    std::swap(prevout, outpoints.back());
                    outpoints.pop_back();
                }
            }
            // output amount will not affect txorphanage
            for (uint32_t i = 0; i < num_out; i++) {
                tx_mut.vout.emplace_back(CAmount{0}, CScript{});
            }
            // restore previously popped outpoints
            for (auto& in : tx_mut.vin) {
                outpoints.push_back(in.prevout);
            }
            auto new_tx = MakeTransactionRef(tx_mut);
            // add newly constructed transaction to outpoints
            for (uint32_t i = 0; i < num_out; i++) {
                outpoints.emplace_back(new_tx->GetHash(), i);
            }
            return new_tx;
        }();

        // trigger orphanage functions
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10 * DEFAULT_MAX_ORPHAN_TRANSACTIONS)
        {
            // Limit possible peers to hit more multiple-announcer behavior.
            NodeId peer_id = fuzzed_data_provider.ConsumeIntegralInRange<NodeId>(0, 5);
            const auto& wtxid = tx->GetWitnessHash();

            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    FastRandomContext rng{true};
                    orphanage.AddChildrenToWorkSet(*tx);
                },
                [&] {
                    {
                        CTransactionRef ref = orphanage.GetTxToReconsider(peer_id);
                        if (ref) {
                            Assert(orphanage.HaveTx(GenTxid::Txid(ref->GetHash())) ||
                                   orphanage.HaveTx(GenTxid::Wtxid(ref->GetHash())));
                            Assert(orphanage.HaveTxAndPeer(GenTxid::Txid(ref->GetHash()), peer_id) ||
                                   orphanage.HaveTxAndPeer(GenTxid::Wtxid(ref->GetHash()), peer_id));
                            Assert(orphanage.BytesFromPeer(peer_id) >= ref->GetTotalSize());
                        }
                    }
                },
                [&] {
                    bool have_tx = orphanage.HaveTx(GenTxid::Txid(tx->GetHash())) || orphanage.HaveTx(GenTxid::Wtxid(tx->GetHash()));
                    // AddTx should return false if tx is too big or already have it
                    // tx weight is unknown, we only check when tx is already in orphanage
                    {
                        auto total_bytes_before = orphanage.TotalOrphanBytes();
                        auto peer_bytes_before = orphanage.BytesFromPeer(peer_id);
                        bool add_tx = orphanage.AddTx(tx, peer_id, {});
                        // have_tx == true -> add_tx == false
                        Assert(!have_tx || !add_tx);
                        if (!have_tx && add_tx) {
                            Assert(orphanage.BytesFromPeer(peer_id) == peer_bytes_before + tx->GetTotalSize());
                            Assert(orphanage.TotalOrphanBytes() == total_bytes_before + tx->GetTotalSize());
                        }
                    }
                    have_tx = orphanage.HaveTx(GenTxid::Txid(tx->GetHash())) || orphanage.HaveTx(GenTxid::Wtxid(tx->GetHash()));
                    {
                        bool add_tx = orphanage.AddTx(tx, peer_id, {});
                        // if have_tx is still false, it must be too big
                        Assert(!have_tx == (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT));
                        Assert(!have_tx || !add_tx);
                    }
                },
                [&] {
                    bool have_tx = orphanage.HaveTx(GenTxid::Txid(tx->GetHash())) || orphanage.HaveTx(GenTxid::Wtxid(tx->GetHash()));
                    // EraseTx should return 0 if m_orphans doesn't have the tx
                    {
                        Assert(have_tx == orphanage.EraseTx(tx->GetWitnessHash()));
                    }
                    have_tx = orphanage.HaveTx(GenTxid::Txid(tx->GetHash())) || orphanage.HaveTx(GenTxid::Wtxid(tx->GetHash()));
                    // have_tx should be false and EraseTx should fail
                    {
                        Assert(!have_tx && !orphanage.EraseTx(tx->GetWitnessHash()));
                    }
                },
                [&] {
                    orphanage.EraseForPeer(peer_id);
                    Assert(orphanage.BytesFromPeer(peer_id) == 0);
                },
                [&] {
                    // test mocktime and expiry
                    SetMockTime(ConsumeTime(fuzzed_data_provider));
                    auto count_limit = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
                    auto size_limit = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
                    orphanage.LimitOrphans(count_limit, size_limit);
                    Assert(orphanage.Size() <= count_limit);
                    Assert(orphanage.TotalOrphanBytes() <= size_limit);
                },
                [&] {
                    // check EraseForBlock
                    CBlock block;
                    block.vtx.emplace_back(tx);
                    orphanage.EraseForBlock(block);
                    Assert(!orphanage.HaveTx(GenTxid::Wtxid(wtxid)));
                },
                [&] {
                    // EraseOrphanOfPeer
                    orphanage.EraseOrphanOfPeer(wtxid, peer_id);
                    Assert(!orphanage.HaveTxAndPeer(GenTxid::Wtxid(wtxid), peer_id));
                });
            if (orphanage.Size() == 0) Assert(orphanage.TotalOrphanBytes() == 0);
            if (orphanage.HaveTx(GenTxid::Wtxid(wtxid))) {
                Assert(orphanage.HaveTx(GenTxid::Txid(tx->GetHash())));
                Assert(orphanage.GetParentTxids(wtxid).has_value());
            }
        }
    }
}
