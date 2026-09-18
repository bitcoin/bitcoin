// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <node/miner.h>
#include <node/txdownloadman.h>
#include <node/txdownloadman_impl.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/hasher.h>
#include <util/rbf.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace {

const TestingSetup* g_setup;

constexpr size_t NUM_COINS{50};
COutPoint COINS[NUM_COINS];

static TxValidationResult TESTED_TX_RESULTS[] = {
    // Skip TX_RESULT_UNSET
    TxValidationResult::TX_CONSENSUS,
    TxValidationResult::TX_INPUTS_NOT_STANDARD,
    TxValidationResult::TX_NOT_STANDARD,
    TxValidationResult::TX_MISSING_INPUTS,
    TxValidationResult::TX_PREMATURE_SPEND,
    TxValidationResult::TX_WITNESS_MUTATED,
    TxValidationResult::TX_WITNESS_STRIPPED,
    TxValidationResult::TX_CONFLICT,
    TxValidationResult::TX_MEMPOOL_POLICY,
    // Skip TX_NO_MEMPOOL
    TxValidationResult::TX_RECONSIDERABLE,
    TxValidationResult::TX_UNKNOWN,
};

// Precomputed transactions. Some may conflict with each other.
std::vector<CTransactionRef> TRANSACTIONS;
// COINS plus the outputs of TRANSACTIONS, so that randomly generated transactions can be children
// of the premade ones.
std::vector<COutPoint> OUTPOINTS;

// Limit the total number of peers because we don't expect coverage to change much with lots more peers.
constexpr int NUM_PEERS = 16;

// Precomputed random durations (positive and negative, each ~exponentially distributed).
std::chrono::microseconds TIME_SKIPS[128];

static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, size_t num_outputs, bool add_witness)
{
    CMutableTransaction tx;
    // If no outpoints are given, create a random one.
    for (const auto& outpoint : outpoints) {
        tx.vin.emplace_back(outpoint);
    }
    if (add_witness) {
        tx.vin[0].scriptWitness.stack.push_back({1});
    }
    for (size_t o = 0; o < num_outputs; ++o) tx.vout.emplace_back(CENT, P2WSH_OP_TRUE);
    return MakeTransactionRef(tx);
}
static std::vector<COutPoint> PickCoins(FuzzedDataProvider& fuzzed_data_provider)
{
    std::vector<COutPoint> ret;
    ret.push_back(PickValue(fuzzed_data_provider, OUTPOINTS));
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10) {
        ret.push_back(PickValue(fuzzed_data_provider, OUTPOINTS));
    }
    return ret;
}

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    for (uint32_t i = 0; i < uint32_t{NUM_COINS}; ++i) {
        COINS[i] = COutPoint{Txid::FromUint256((HashWriter() << i).GetHash()), i};
    }
    size_t outpoints_index = 0;
    // 2 transactions same txid different witness
    {
        auto tx1{MakeTransactionSpending({COINS[outpoints_index]}, /*num_outputs=*/5, /*add_witness=*/false)};
        auto tx2{MakeTransactionSpending({COINS[outpoints_index]}, /*num_outputs=*/5, /*add_witness=*/true)};
        Assert(tx1->GetHash() == tx2->GetHash());
        TRANSACTIONS.emplace_back(tx1);
        TRANSACTIONS.emplace_back(tx2);
        outpoints_index += 1;
    }
    // 2 parents 1 child
    {
        auto tx_parent_1{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/1, /*add_witness=*/true)};
        TRANSACTIONS.emplace_back(tx_parent_1);
        auto tx_parent_2{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/1, /*add_witness=*/false)};
        TRANSACTIONS.emplace_back(tx_parent_2);
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent_1->GetHash(), 0}, COutPoint{tx_parent_2->GetHash(), 0}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
    }
    // 1 parent 2 children
    {
        auto tx_parent{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/2, /*add_witness=*/true)};
        TRANSACTIONS.emplace_back(tx_parent);
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent->GetHash(), 0}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent->GetHash(), 1}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
    }
    // chain of 5 segwit
    {
        COutPoint& last_outpoint = COINS[outpoints_index++];
        for (auto i{0}; i < 5; ++i) {
            auto tx{MakeTransactionSpending({last_outpoint}, /*num_outputs=*/1, /*add_witness=*/true)};
            TRANSACTIONS.emplace_back(tx);
            last_outpoint = COutPoint{tx->GetHash(), 0};
        }
    }
    // chain of 5 non-segwit
    {
        COutPoint& last_outpoint = COINS[outpoints_index++];
        for (auto i{0}; i < 5; ++i) {
            auto tx{MakeTransactionSpending({last_outpoint}, /*num_outputs=*/1, /*add_witness=*/false)};
            TRANSACTIONS.emplace_back(tx);
            last_outpoint = COutPoint{tx->GetHash(), 0};
        }
    }
    // Also create a loose tx for each outpoint. Some of these transactions conflict with the above
    // or have the same txid.
    for (const auto& outpoint : COINS) {
        TRANSACTIONS.emplace_back(MakeTransactionSpending({outpoint}, /*num_outputs=*/1, /*add_witness=*/true));
    }

    OUTPOINTS.assign(std::begin(COINS), std::end(COINS));
    for (const auto& tx : TRANSACTIONS) {
        for (uint32_t o = 0; o < tx->vout.size(); ++o) OUTPOINTS.emplace_back(tx->GetHash(), o);
    }

    // Create random-looking time jumps
    int i = 0;
    // TIME_SKIPS[N] for N=0..15 is just N microseconds.
    for (; i < 16; ++i) {
        TIME_SKIPS[i] = std::chrono::microseconds{i};
    }
    // TIME_SKIPS[N] for N=16..127 has randomly-looking but roughly exponentially increasing values up to
    // 198.416453 seconds.
    for (; i < 128; ++i) {
        int diff_bits = ((i - 10) * 2) / 9;
        uint64_t diff = 1 + (CSipHasher(0, 0).Write(i).Finalize() >> (64 - diff_bits));
        TIME_SKIPS[i] = TIME_SKIPS[i - 1] + std::chrono::microseconds{diff};
    }
}

void CheckPackageToValidate(const node::PackageToValidate& package_to_validate, NodeId peer)
{
    Assert(package_to_validate.m_senders.size() == 2);
    Assert(package_to_validate.m_senders.front() == peer);
    Assert(package_to_validate.m_senders.back() < NUM_PEERS);

    // Package is a 1p1c
    const auto& package = package_to_validate.m_txns;
    Assert(IsChildWithParents(package));
    Assert(package.size() == 2);
}

FUZZ_TARGET(txdownloadman, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    // Initialize txdownloadman
    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    node::TxDownloadManager txdownloadman{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};

    std::chrono::microseconds time{244466666};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 500) {
        NodeId rand_peer = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, NUM_PEERS - 1);

        // Transaction can be one of the premade ones or a randomly generated one
        auto rand_tx = fuzzed_data_provider.ConsumeBool() ?
            MakeTransactionSpending(PickCoins(fuzzed_data_provider),
                                    /*num_outputs=*/fuzzed_data_provider.ConsumeIntegralInRange(1, 500),
                                    /*add_witness=*/fuzzed_data_provider.ConsumeBool()) :
            TRANSACTIONS.at(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, TRANSACTIONS.size() - 1));

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node::TxDownloadConnectionInfo info{
                    .m_preferred = fuzzed_data_provider.ConsumeBool(),
                    .m_relay_permissions = fuzzed_data_provider.ConsumeBool(),
                    .m_wtxid_relay = fuzzed_data_provider.ConsumeBool()
                };
                txdownloadman.ConnectedPeer(rand_peer, info);
            },
            [&] {
                txdownloadman.DisconnectedPeer(rand_peer);
                txdownloadman.CheckIsEmpty(rand_peer);
            },
            [&] {
                txdownloadman.ActiveTipChange();
            },
            [&] {
                CBlock block;
                block.vtx.push_back(rand_tx);
                txdownloadman.BlockConnected(std::make_shared<CBlock>(block));
            },
            [&] {
                txdownloadman.BlockDisconnected();
            },
            [&] {
                txdownloadman.MempoolAcceptedTx(rand_tx);
            },
            [&] {
                TxValidationState state;
                state.Invalid(fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                bool first_time_failure{fuzzed_data_provider.ConsumeBool()};

                node::RejectedTxTodo todo = txdownloadman.MempoolRejectedTx(rand_tx, state, rand_peer, first_time_failure);
                Assert(first_time_failure || !todo.m_should_add_extra_compact_tx);
            },
            [&] {
                auto gtxid = fuzzed_data_provider.ConsumeBool() ?
                             GenTxid{rand_tx->GetHash()} :
                             GenTxid{rand_tx->GetWitnessHash()};
                txdownloadman.AddTxAnnouncement(rand_peer, gtxid, time);
            },
            [&] {
                txdownloadman.GetRequestsToSend(rand_peer, time);
            },
            [&] {
                txdownloadman.ReceivedTx(rand_peer, rand_tx);
                const auto& [should_validate, maybe_package] = txdownloadman.ReceivedTx(rand_peer, rand_tx);
                // The only possible results should be:
                // - Don't validate the tx, no package.
                // - Don't validate the tx, package.
                // - Validate the tx, no package.
                // The only combination that doesn't make sense is validate both tx and package.
                Assert(!(should_validate && maybe_package.has_value()));
                if (maybe_package.has_value()) CheckPackageToValidate(*maybe_package, rand_peer);
            },
            [&] {
                txdownloadman.ReceivedNotFound(rand_peer, {rand_tx->GetWitnessHash()});
            },
            [&] {
                const bool expect_work{txdownloadman.HaveMoreWork(rand_peer)};
                const auto ptx = txdownloadman.GetTxToReconsider(rand_peer);
                // expect_work=true doesn't necessarily mean the next item from the workset isn't a
                // nullptr, as the transaction could have been removed from orphanage without being
                // removed from the peer's workset.
                if (ptx) {
                    // However, if there was a non-null tx in the workset, HaveMoreWork should have
                    // returned true.
                    Assert(expect_work);
                }
            });
        // Jump forwards or backwards
        auto time_skip = fuzzed_data_provider.PickValueInArray(TIME_SKIPS);
        if (fuzzed_data_provider.ConsumeBool()) time_skip *= -1;
        time += time_skip;
    }
    // Disconnect everybody, check that all data structures are empty.
    for (NodeId nodeid = 0; nodeid < NUM_PEERS; ++nodeid) {
        txdownloadman.DisconnectedPeer(nodeid);
        txdownloadman.CheckIsEmpty(nodeid);
    }
    txdownloadman.CheckIsEmpty();
}

// Give node 0 relay permissions, and nobody else. This helps us remember who is a RelayPermissions
// peer without tracking anything (this is only for the txdownload_impl target).
static bool HasRelayPermissions(NodeId peer) { return peer == 0; }

/** Non-completed announcements per txhash, for the given hashes. */
using RequestSnapshot = std::map<uint256, std::vector<NodeId>>;
static RequestSnapshot SnapshotRequests(const TxRequestTracker& txrequest, const std::set<uint256>& hashes)
{
    RequestSnapshot ret;
    for (const auto& hash : hashes) {
        std::vector<NodeId> peers;
        txrequest.GetCandidatePeers(hash, peers);
        if (!peers.empty()) ret.emplace(hash, std::move(peers));
    }
    return ret;
}
/** Every peer's announcement present before must still be present, unless its hash was allowed to be forgotten. */
static void CheckRequestsKept(const RequestSnapshot& before, const TxRequestTracker& txrequest,
                              const std::set<uint256>& allowed_hashes, std::optional<NodeId> allowed_peer = std::nullopt)
{
    for (const auto& [hash, peers_before] : before) {
        if (allowed_hashes.contains(hash)) continue;
        std::vector<NodeId> peers_after;
        txrequest.GetCandidatePeers(hash, peers_after);
        for (const NodeId peer : peers_before) {
            if (peer == allowed_peer) continue;
            Assert(std::ranges::find(peers_after, peer) != peers_after.end());
        }
    }
}

/** Announcers per orphan. */
using AnnouncerSnapshot = std::map<Wtxid, std::set<NodeId>>;
static AnnouncerSnapshot SnapshotAnnouncers(const node::TxOrphanage& orphanage)
{
    AnnouncerSnapshot ret;
    for (const auto& info : orphanage.GetOrphanTransactions()) {
        ret.emplace(info.tx->GetWitnessHash(), info.announcers);
    }
    return ret;
}
/** Every orphan announcement present before must still be present, unless the orphan was allowed to be erased. */
static void CheckAnnouncersKept(const AnnouncerSnapshot& before, const node::TxOrphanage& orphanage,
                                const std::set<Wtxid>& allowed_erased, std::optional<NodeId> allowed_peer = std::nullopt)
{
    const auto after{SnapshotAnnouncers(orphanage)};
    for (const auto& [wtxid, announcers_before] : before) {
        if (allowed_erased.contains(wtxid)) continue;
        const auto it{after.find(wtxid)};
        for (const NodeId peer : announcers_before) {
            if (peer == allowed_peer) continue;
            Assert(it != after.end() && it->second.contains(peer));
        }
    }
}
/** Whether erasing exactly the given orphans is guaranteed not to trigger trimming: the global capacity
 * shrinks by one peer's reservation for every peer left without any announcement. */
static bool NoTrimmingAfterErase(const node::TxOrphanage& orphanage, const AnnouncerSnapshot& before, const std::set<Wtxid>& erased)
{
    std::set<NodeId> all_peers, peers_keeping_some;
    for (const auto& [wtxid, announcers] : before) {
        for (const NodeId peer : announcers) {
            all_peers.insert(peer);
            if (!erased.contains(wtxid)) peers_keeping_some.insert(peer);
        }
    }
    const int64_t leaving{static_cast<int64_t>(all_peers.size() - peers_keeping_some.size())};
    return orphanage.TotalOrphanUsage() <= orphanage.MaxGlobalUsage() - leaving * orphanage.ReservedPeerUsage();
}
/** Whether adding tx as an orphan for up to NUM_PEERS announcers is guaranteed not to trigger trimming. */
static bool NoTrimmingPossible(const node::TxOrphanage& orphanage, const CTransaction& tx)
{
    const auto usage{NUM_PEERS * GetTransactionWeight(tx)};
    const auto latency{NUM_PEERS * (1 + tx.vin.size() / 10)};
    return orphanage.TotalOrphanUsage() + usage <= orphanage.MaxGlobalUsage() &&
           orphanage.TotalLatencyScore() + latency <= orphanage.MaxGlobalLatencyScore();
}

static void CheckInvariants(const node::TxDownloadManagerImpl& txdownload_impl)
{
    txdownload_impl.m_orphanage->SanityCheck();
    // We should never have more than the maximum in-flight requests out for a peer.
    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        if (!HasRelayPermissions(peer)) {
            Assert(txdownload_impl.m_txrequest.Count(peer) <= node::MAX_PEER_TX_ANNOUNCEMENTS);
        }
    }
    txdownload_impl.m_txrequest.SanityCheck();
}

FUZZ_TARGET(txdownloadman_impl, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    // Initialize a TxDownloadManagerImpl
    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};

    std::chrono::microseconds time{244466666};
    // All txhashes that may have entered m_txrequest: announced txids/wtxids and parent txids of orphans.
    std::set<uint256> seen_hashes;

    // Start with all peers connected so that multi-step interactions (e.g. storing an orphan for one
    // peer, then rejecting its parent from another) are reachable; peers may still (dis)connect later.
    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        txdownload_impl.ConnectedPeer(peer, node::TxDownloadConnectionInfo{
            .m_preferred = fuzzed_data_provider.ConsumeBool(),
            .m_relay_permissions = HasRelayPermissions(peer),
            .m_wtxid_relay = fuzzed_data_provider.ConsumeBool()});
    }

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 500) {
        NodeId rand_peer = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, NUM_PEERS - 1);

        // Transaction can be one of the premade ones or a randomly generated one. Sometimes pick a
        // premade transaction that is the missing parent of a current orphan, so that orphan
        // resolution (parent arriving after child) is exercised often.
        auto rand_tx = fuzzed_data_provider.ConsumeBool() ?
            MakeTransactionSpending(PickCoins(fuzzed_data_provider),
                                    /*num_outputs=*/fuzzed_data_provider.ConsumeIntegralInRange(1, 500),
                                    /*add_witness=*/fuzzed_data_provider.ConsumeBool()) :
            TRANSACTIONS.at(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, TRANSACTIONS.size() - 1));
        if (fuzzed_data_provider.ConsumeBool()) {
            std::vector<CTransactionRef> orphan_parents;
            for (const auto& tx : TRANSACTIONS) {
                if (txdownload_impl.m_orphanage->HaveChildren(*tx)) orphan_parents.push_back(tx);
            }
            if (!orphan_parents.empty()) rand_tx = PickValue(fuzzed_data_provider, orphan_parents);
        }
        seen_hashes.insert(rand_tx->GetHash().ToUint256());
        seen_hashes.insert(rand_tx->GetWitnessHash().ToUint256());
        for (const auto& input : rand_tx->vin) seen_hashes.insert(input.prevout.hash.ToUint256());
        const std::set<uint256> rand_tx_hashes{rand_tx->GetHash().ToUint256(), rand_tx->GetWitnessHash().ToUint256()};

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node::TxDownloadConnectionInfo info{
                    .m_preferred = fuzzed_data_provider.ConsumeBool(),
                    .m_relay_permissions = HasRelayPermissions(rand_peer),
                    .m_wtxid_relay = fuzzed_data_provider.ConsumeBool()
                };
                txdownload_impl.ConnectedPeer(rand_peer, info);
            },
            [&] {
                const auto requests_before{SnapshotRequests(txdownload_impl.m_txrequest, seen_hashes)};
                txdownload_impl.DisconnectedPeer(rand_peer);
                txdownload_impl.CheckIsEmpty(rand_peer);
                // Only this peer's requests go away.
                CheckRequestsKept(requests_before, txdownload_impl.m_txrequest, /*allowed_hashes=*/{}, rand_peer);
            },
            [&] {
                const auto requests_before{SnapshotRequests(txdownload_impl.m_txrequest, seen_hashes)};
                const auto announcers_before{SnapshotAnnouncers(*txdownload_impl.m_orphanage)};
                txdownload_impl.ActiveTipChange();
                // A tip change only resets the rejection filters.
                CheckRequestsKept(requests_before, txdownload_impl.m_txrequest, /*allowed_hashes=*/{});
                CheckAnnouncersKept(announcers_before, *txdownload_impl.m_orphanage, /*allowed_erased=*/{});
                // After a block update, nothing should be in the rejection caches
                for (const auto& tx : TRANSACTIONS) {
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(tx->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(tx->GetHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(tx->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(tx->GetHash().ToUint256()));
                }
            },
            [&] {
                CBlock block;
                block.vtx.push_back(rand_tx);
                const auto requests_before{SnapshotRequests(txdownload_impl.m_txrequest, seen_hashes)};
                const auto announcers_before{SnapshotAnnouncers(*txdownload_impl.m_orphanage)};
                // Orphans included in or conflicting with the block may be erased.
                std::set<Wtxid> allowed_erased{rand_tx->GetWitnessHash()};
                for (const auto& [wtxid, _] : announcers_before) {
                    const auto orphan{txdownload_impl.m_orphanage->GetTx(wtxid)};
                    if (std::ranges::any_of(orphan->vin, [&](const auto& in) {
                            return std::ranges::any_of(rand_tx->vin, [&](const auto& block_in) { return in.prevout == block_in.prevout; });
                        })) {
                        allowed_erased.insert(wtxid);
                    }
                }
                const bool no_trimming{NoTrimmingAfterErase(*txdownload_impl.m_orphanage, announcers_before, allowed_erased)};
                txdownload_impl.BlockConnected(std::make_shared<CBlock>(block));
                // Block transactions must be removed from orphanage
                Assert(!txdownload_impl.m_orphanage->HaveTx(rand_tx->GetWitnessHash()));
                CheckRequestsKept(requests_before, txdownload_impl.m_txrequest, rand_tx_hashes);
                if (no_trimming) CheckAnnouncersKept(announcers_before, *txdownload_impl.m_orphanage, allowed_erased);
            },
            [&] {
                txdownload_impl.BlockDisconnected();
                Assert(!txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetWitnessHash().ToUint256()));
                Assert(!txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetHash().ToUint256()));
            },
            [&] {
                const auto requests_before{SnapshotRequests(txdownload_impl.m_txrequest, seen_hashes)};
                const auto announcers_before{SnapshotAnnouncers(*txdownload_impl.m_orphanage)};
                const bool no_trimming{NoTrimmingAfterErase(*txdownload_impl.m_orphanage, announcers_before, {rand_tx->GetWitnessHash()})};
                txdownload_impl.MempoolAcceptedTx(rand_tx);
                // Only this tx's requests and orphan entry go away.
                CheckRequestsKept(requests_before, txdownload_impl.m_txrequest, rand_tx_hashes);
                if (no_trimming) CheckAnnouncersKept(announcers_before, *txdownload_impl.m_orphanage, {rand_tx->GetWitnessHash()});
            },
            [&] {
                TxValidationState state;
                // Bias towards the results that drive orphan storage and package evaluation.
                state.Invalid(fuzzed_data_provider.ConsumeBool() ?
                              (fuzzed_data_provider.ConsumeBool() ? TxValidationResult::TX_MISSING_INPUTS : TxValidationResult::TX_RECONSIDERABLE) :
                              fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                bool first_time_failure{fuzzed_data_provider.ConsumeBool()};

                bool reject_contains_wtxid{txdownload_impl.RecentRejectsFilter().contains(rand_tx->GetWitnessHash().ToUint256())};
                const auto requests_before{SnapshotRequests(txdownload_impl.m_txrequest, seen_hashes)};
                const auto announcers_before{SnapshotAnnouncers(*txdownload_impl.m_orphanage)};

                // Which of this tx's hashes a rejection may forget in m_txrequest, for all peers.
                std::set<uint256> allowed_hashes;
                const bool witnessless{!rand_tx->HasWitness()};
                switch (state.GetResult()) {
                case TxValidationResult::TX_MISSING_INPUTS:
                    // Stored as orphan, or rejected along with its parents: both hashes.
                    allowed_hashes = rand_tx_hashes;
                    break;
                case TxValidationResult::TX_WITNESS_STRIPPED:
                    break;
                case TxValidationResult::TX_INPUTS_NOT_STANDARD:
                    // Witness-independent failure: both hashes.
                    allowed_hashes = rand_tx_hashes;
                    break;
                case TxValidationResult::TX_RECONSIDERABLE:
                    // A witnessless tx that is the missing parent of an orphan keeps the (txid) orphan
                    // resolution requests of other peers.
                    if (!(witnessless && txdownload_impl.m_orphanage->HaveChildren(*rand_tx))) {
                        allowed_hashes.insert(rand_tx->GetWitnessHash().ToUint256());
                    }
                    break;
                default:
                    allowed_hashes.insert(rand_tx->GetWitnessHash().ToUint256());
                }
                // Only the rejected tx's own orphan entry may be erased. Storing it as a new orphan, or
                // erasing it, may trim other peers' announcements when the orphanage is near its limits.
                const bool no_trimming{state.GetResult() == TxValidationResult::TX_MISSING_INPUTS ?
                                       NoTrimmingPossible(*txdownload_impl.m_orphanage, *rand_tx) :
                                       NoTrimmingAfterErase(*txdownload_impl.m_orphanage, announcers_before, {rand_tx->GetWitnessHash()})};

                node::RejectedTxTodo todo = txdownload_impl.MempoolRejectedTx(rand_tx, state, rand_peer, first_time_failure);
                Assert(first_time_failure || !todo.m_should_add_extra_compact_tx);
                if (!reject_contains_wtxid) Assert(todo.m_unique_parents.size() <= rand_tx->vin.size());
                CheckRequestsKept(requests_before, txdownload_impl.m_txrequest, allowed_hashes);
                if (no_trimming) CheckAnnouncersKept(announcers_before, *txdownload_impl.m_orphanage, {rand_tx->GetWitnessHash()});
            },
            [&] {
                auto gtxid = fuzzed_data_provider.ConsumeBool() ?
                             GenTxid{rand_tx->GetHash()} :
                             GenTxid{rand_tx->GetWitnessHash()};
                txdownload_impl.AddTxAnnouncement(rand_peer, gtxid, time);
            },
            [&] {
                const auto getdata_requests = txdownload_impl.GetRequestsToSend(rand_peer, time);
                // TxDownloadManager should not be telling us to request things we already have.
                // Exclude m_lazy_recent_rejects_reconsiderable because it may request low-feerate parent of orphan.
                for (const auto& gtxid : getdata_requests) {
                    Assert(!txdownload_impl.AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false));
                }
            },
            [&] {
                const auto& [should_validate, maybe_package] = txdownload_impl.ReceivedTx(rand_peer, rand_tx);
                // The only possible results should be:
                // - Don't validate the tx, no package.
                // - Don't validate the tx, package.
                // - Validate the tx, no package.
                // The only combination that doesn't make sense is validate both tx and package.
                Assert(!(should_validate && maybe_package.has_value()));
                if (should_validate) {
                    Assert(!txdownload_impl.AlreadyHaveTx(rand_tx->GetWitnessHash(), /*include_reconsiderable=*/true));
                }
                if (maybe_package.has_value()) {
                    CheckPackageToValidate(*maybe_package, rand_peer);

                    const auto& package = maybe_package->m_txns;
                    // Parent is in m_lazy_recent_rejects_reconsiderable and child is in m_orphanage
                    Assert(txdownload_impl.RecentRejectsReconsiderableFilter().contains(rand_tx->GetWitnessHash().ToUint256()));
                    Assert(txdownload_impl.m_orphanage->HaveTx(maybe_package->m_txns.back()->GetWitnessHash()));
                    // Package has not been rejected
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(GetPackageHash(package)));
                    // Neither is in m_lazy_recent_rejects
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(package.front()->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(package.back()->GetWitnessHash().ToUint256()));
                }
            },
            [&] {
                txdownload_impl.ReceivedNotFound(rand_peer, {rand_tx->GetWitnessHash()});
            },
            [&] {
                const bool expect_work{txdownload_impl.HaveMoreWork(rand_peer)};
                const auto ptx{txdownload_impl.GetTxToReconsider(rand_peer)};
                // expect_work=true doesn't necessarily mean the next item from the workset isn't a
                // nullptr, as the transaction could have been removed from orphanage without being
                // removed from the peer's workset.
                if (ptx) {
                    // However, if there was a non-null tx in the workset, HaveMoreWork should have
                    // returned true.
                    Assert(expect_work);
                    Assert(txdownload_impl.AlreadyHaveTx(ptx->GetWitnessHash(), /*include_reconsiderable=*/false));
                    // Presumably we have validated this tx. Use "missing inputs" to keep it in the
                    // orphanage longer. Later iterations might call MempoolAcceptedTx or
                    // MempoolRejectedTx with a different error.
                    TxValidationState state_missing_inputs;
                    state_missing_inputs.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
                    txdownload_impl.MempoolRejectedTx(ptx, state_missing_inputs, rand_peer, fuzzed_data_provider.ConsumeBool());
                }
            });

        auto time_skip = fuzzed_data_provider.PickValueInArray(TIME_SKIPS);
        if (fuzzed_data_provider.ConsumeBool()) time_skip *= -1;
        time += time_skip;
    }
    CheckInvariants(txdownload_impl);
    // Disconnect everybody, check that all data structures are empty.
    for (NodeId nodeid = 0; nodeid < NUM_PEERS; ++nodeid) {
        txdownload_impl.DisconnectedPeer(nodeid);
        txdownload_impl.CheckIsEmpty(nodeid);
    }
    txdownload_impl.CheckIsEmpty();
}

} // namespace
