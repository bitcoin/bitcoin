// Copyright (c) 2023 The Bitcoin Core developers
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
#include <test/util/txmempool.h>
#include <util/hasher.h>
#include <util/rbf.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

namespace {
// What a peer may send proactively.
enum class PossibleMessage {
    /** Don't send anything */
    NOTHING,
    /** Announce the target transaction */
    INV_REAL,
    /** Announce another transaction */
    INV_DIFFERENT,
    /** Announce a random transaction. */
    INV_RANDOM,
    /** Send random transaction, perhaps unsolicited */
    TX_RANDOM,
    /** Send a notfound for a random hash*/
    NOTFOUND,
};

// What a peer may send in response to a getdata.
enum class PossibleResponse {
    /** Send the transaction requested */
    TX_REAL,
    /** Send a transaction whose txid matches the requested hash, with the witness stripped */
    TX_WITNESS_STRIPPED,
    /** Send a transaction whose txid matches the requested hash, with the witness malleated */
    TX_WITNESS_MALLEATED,
    /** Send a notfound for this getdata */
    NOTFOUND,
    /** Don't send anything (similar to becoming unresponsive and/or stalling) */
    NOTHING,
    /** Push to the back of the message queue (similar to stalling and/or responding to messages out of order) */
    DEFER,
};

// Possible states. Only possible state transitions are (start) NEVER_CONNECTED -> CONNECTED -> DISCONNECTED (terminal).
// Once a peer has disconnected, we won't reconnect them again within the test.
enum class ConnectionState {
    NEVER_CONNECTED = 0,
    CONNECTED,
    DISCONNECTED,
};

// Possible unsolicited messages from a non-good peer.
static PossibleMessage NON_GOOD_MESSAGES[] = {
    PossibleMessage::NOTHING,
    PossibleMessage::INV_REAL,
    PossibleMessage::INV_DIFFERENT,
    PossibleMessage::INV_RANDOM,
    PossibleMessage::TX_RANDOM,
    PossibleMessage::NOTFOUND,
};
// Possible responses from a non-good peer.
static PossibleResponse NON_GOOD_RESPONSES[] = {
    PossibleResponse::NOTHING,
    PossibleResponse::TX_WITNESS_STRIPPED,
    PossibleResponse::TX_WITNESS_MALLEATED,
    PossibleResponse::NOTFOUND,
    PossibleResponse::DEFER,
};

class TestPeer {
public:
    // Queue of getdatas to respond to: process from front, add to back.
    std::deque<GenTxid> m_getdata_queue;

    // Connection state
    ConnectionState m_connection_state{ConnectionState::NEVER_CONNECTED};

    // NodeId even if not connected.
    NodeId m_nodeid;

    // Whether this peer is "good"; the goal of this test is to verify that we can always download
    // a transaction as long as we have one such peer. We define a "good" peer as follows:
    // - only 2 possible unsolicited messages (NOTHING or INV_REAL exactly 1 time throughout the test)
    // - only 1 possible response to a getdata (TX_REAL)
    // - m_wtxid_relay=true
    // - never disconnects from us (at least while we're trying to download this transaction)
    //
    // A non-good peer can do everything the good peer might do, but also some other things.
    //
    // This definition is narrower than just being non-malicious in order to be useful. In real
    // life, an honest node may disconnect from us or forget a transaction (i.e. send a notfound).
    bool m_good;

    // for TxDownloadConnectionInfo
    // Note that this is used to indicate when somebody is an outbound peer.
    bool m_connection_preferred{false};
    bool m_relay_permissions{false};
    bool m_wtxid_relay{true};

    // Whether this peer has already sent announcement for the tx.
    bool m_sent_inv{false};

    // Decide what kind of unsolicited message to send. This is the only place where we might decide
    // to announce the target transaction. Updates m_sent_inv if this happens.
    // When force_announce=true, if this is a "good" peer and the transaction hasn't already been
    // announced, always return INV_REAL.
    PossibleMessage SendOneMessage(FuzzedDataProvider& fuzzed_data_provider, bool force_announce = false) {
        if (m_good) {
            // "Good" peers only send 1 unsolicited message: announcement of the tx.
            if (!m_sent_inv) {
                if (force_announce || fuzzed_data_provider.ConsumeBool()) {
                    m_sent_inv = true;
                    return PossibleMessage::INV_REAL;
                }
            }
            return PossibleMessage::NOTHING;
        }

        // No restrictions for non-good peers.
        auto message = fuzzed_data_provider.PickValueInArray(NON_GOOD_MESSAGES);
        if (message == PossibleMessage::INV_REAL) m_sent_inv = true;
        return message;
    }

    // Process up to one message from the front of m_getdata_queue. The size of m_getdata_queue may
    // not change, as DEFER may just push it to the back again.
    std::pair<PossibleResponse, std::optional<GenTxid>> ProcessOneMessage(FuzzedDataProvider& fuzzed_data_provider) {
        // Nothing to do if no messages to process
        if (m_getdata_queue.empty()) return {PossibleResponse::NOTHING, std::nullopt};

        auto request = m_getdata_queue.front();
        auto response{PossibleResponse::TX_REAL};
        if (!m_good) response = fuzzed_data_provider.PickValueInArray(NON_GOOD_RESPONSES);

        // If DEFER, pop the request to the back of the queue and return nothing.
        // This simulates responding out of order and/or stalling.
        if (response == PossibleResponse::DEFER) {
            m_getdata_queue.push_back(request);
            m_getdata_queue.pop_front();
            return {PossibleResponse::NOTHING, std::nullopt};
        }

        m_getdata_queue.pop_front();
        return {response, request};
    }
};

TestPeer RandomPeer(FuzzedDataProvider& fuzzed_data_provider, NodeId nodeid, bool preferred, bool force_good)
{
    TestPeer peer;
    peer.m_nodeid = nodeid;
    peer.m_good = force_good || fuzzed_data_provider.ConsumeBool();
    peer.m_connection_preferred = preferred;
    peer.m_relay_permissions = fuzzed_data_provider.ConsumeBool();
    // good peer should be at least WTXID_RELAY_VERSION
    peer.m_wtxid_relay = force_good || fuzzed_data_provider.ConsumeBool();
    return peer;
}

static TxValidationResult TESTED_TX_RESULTS[] = {
    // This can lead to orphan resolution
    TxValidationResult::TX_MISSING_INPUTS,
    // Reconsiderable failures - can lead to package validation.
    TxValidationResult::TX_RECONSIDERABLE,
    // Represents all non-reconsiderable failures - it's not necessary to distinguish between policy
    // and consensus here.
    TxValidationResult::TX_MEMPOOL_POLICY,
};


const TestingSetup* g_setup;
constexpr size_t NUM_COINS{100};
// map from uint256 (can be txid or wtxid) to 2 transactions of the same txid.
// If the key is a wtxid, the first tx in the pair has that wtxid, and the second doesn't.
std::map<uint256, std::pair<CTransactionRef, CTransactionRef>> TRANSACTIONS;

static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, size_t num_outputs, bool add_witness)
{
    CMutableTransaction tx;
    for (const auto& outpoint : outpoints) {
        tx.vin.emplace_back(outpoint);
    }
    if (add_witness) {
        tx.vin[0].scriptWitness.stack.push_back({1});
    }
    tx.vout.emplace_back(CENT, P2WSH_OP_TRUE);
    return MakeTransactionRef(tx);
}

static CTransactionRef Malleate(const CTransactionRef& ptx)
{
    CMutableTransaction tx(*ptx);
    tx.vin[0].scriptWitness.stack.push_back({5});
    auto mutated_tx = MakeTransactionRef(tx);
    assert(ptx->GetHash() == mutated_tx->GetHash());
    return mutated_tx;
}

static CTransactionRef StripWitness(const CTransactionRef& ptx)
{
    CMutableTransaction tx(*ptx);
    tx.vin[0].scriptWitness.stack.clear();
    auto stripped_tx = MakeTransactionRef(tx);
    assert(ptx->GetHash() == stripped_tx->GetHash());
    return stripped_tx;
}

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    for (uint32_t i = 0; i < uint32_t{NUM_COINS}; ++i) {
        COutPoint outpoint{Txid::FromUint256((HashWriter() << i).GetHash()), i};

        auto correct_tx = MakeTransactionSpending({outpoint}, /*num_outputs=*/2, /*add_witness=*/true);
        auto malleated_tx = Malleate(correct_tx);
        auto stripped_tx = StripWitness(correct_tx);

        const auto& target_txid = correct_tx->GetHash().ToUint256();
        const auto& target_wtxid = correct_tx->GetWitnessHash().ToUint256();
        const auto& malleated_wtxid = malleated_tx->GetWitnessHash().ToUint256();

        // Provide a way to query each transaction by wtxid or by txid. Each pair of transactions
        // have identical txids. Additionally, the correct_tx only ever appears as the first
        // transaction of the pair. This ensures that sending a malleated version of a tx does not
        // accidentally cause the node to receive correct_tx early.
        TRANSACTIONS.try_emplace(target_txid, correct_tx, correct_tx);
        TRANSACTIONS.try_emplace(target_wtxid, correct_tx, malleated_tx);
        TRANSACTIONS.try_emplace(malleated_wtxid, malleated_tx, stripped_tx);
    }
}

static CTransactionRef GetCorrectTx(const std::map<uint256, std::pair<CTransactionRef, CTransactionRef>> txmap, const GenTxid& gtxid)
{
    auto tx_it = txmap.find(gtxid.GetHash());
    if (tx_it != txmap.end()) {
        return tx_it->second.first;
    }
    return nullptr;
}
static CTransactionRef GetStrippedTx(const std::map<uint256, std::pair<CTransactionRef, CTransactionRef>> txmap, const GenTxid& gtxid)
{
    auto tx_it = txmap.find(gtxid.GetHash());
    if (tx_it != txmap.end()) {
        return StripWitness(tx_it->second.first);
    }
    return nullptr;
}
static CTransactionRef GetMalleatedTx(const std::map<uint256, std::pair<CTransactionRef, CTransactionRef>> txmap, const GenTxid& gtxid)
{
    auto tx_it = txmap.find(gtxid.GetHash());
    if (tx_it != txmap.end()) {
        return tx_it->second.second;
    }
    return nullptr;
}
static CTransactionRef GetRandomTx(const std::map<uint256, std::pair<CTransactionRef, CTransactionRef>> txmap, FuzzedDataProvider& fuzzed_data_provider, bool correct_only = false)
{
    const auto jumps{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, txmap.size() - 1)};
    auto it = std::next(txmap.begin(), jumps);

    if (it == txmap.end()) return nullptr;
    return correct_only || fuzzed_data_provider.ConsumeBool() ? it->second.first : it->second.second;
}

// This test asserts that, as long as we have 1 good outbound peer, even if all other peers are
// malicious, we'll always be able to download a transaction.
// Connect to 8 outbounds and 0-120 inbound peers. 1 outbound is guaranteed to be good (see m_good
// above for definition), while the rest may or may not be.
// A non-good node may stall a getdata, send a malleated or witness stripped tx/inv, send a notfound, etc.
FUZZ_TARGET(txdownloadman_one_honest_peer, .init = initialize)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // Initialize txdownloadman
    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    const auto max_orphan_count = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 300);
    FastRandomContext det_rand{true};

    node::TxDownloadManager txdownloadman{node::TxDownloadOptions{pool, det_rand, max_orphan_count, true}};
    std::chrono::microseconds time{244466666};

    // Decide the target transaction. Disallow selecting a witness-stripped or non-segwit
    // transaction because it can lead to false negatives. This test may mark a same-txid
    // transaction as invalid, and we don't want that to cause rejection of the target tx.
    const auto& target_tx = GetRandomTx(TRANSACTIONS, fuzzed_data_provider, /*correct_only=*/true);
    Assert(target_tx != nullptr);
    Assert(target_tx->HasWitness());

    const auto& target_txid = target_tx->GetHash();
    const auto& target_wtxid = target_tx->GetWitnessHash();

    // Decide what the peers are.
    std::vector<TestPeer> all_peers;
    // NodeId of the "designated good peer"
    // Other TestPeers may be "good" as well, but not guaranteed.
    NodeId honestid = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 7);
    // 8 outbound peers. NodeId = o. We indicate outbound using preferred=true
    for (unsigned int o = 0; o < 8; ++o) {
        bool good = (o == honestid) || fuzzed_data_provider.ConsumeBool();
        all_peers.emplace_back(RandomPeer(fuzzed_data_provider, o, /*preferred=*/true, /*force_good=*/good));
    }
    // 0-120 inbound peers. NodeId = i. We indicate inbound using preferred=false
    auto num_inbound_peers = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 120);
    for (unsigned int i = 8; i < num_inbound_peers + 8; ++i) {
        bool good = fuzzed_data_provider.ConsumeBool();
        all_peers.emplace_back(RandomPeer(fuzzed_data_provider, i, /*preferred=*/false, /*force_good=*/good));
    }

    // Indexes for random peer iteration
    std::vector<size_t> indexes(all_peers.size());
    std::iota(indexes.begin(), indexes.end(), 0);

    // We are trying to ensure that we always download the tx as long as we have 1 good outbound
    // peer who is able to send it to us.
    bool got_tx{false};

    // Keep track of how many iterations the loop has done to make sure the target transaction is
    // announced within the first 1000 rounds. Note that the good peer is guaranteed to connect
    // within the first round and will never be disconnected.
    int counter = 0;

    LIMITED_WHILE(!got_tx, 100000)
    {
        FastRandomContext rng{ConsumeUInt256(fuzzed_data_provider)};
        std::shuffle(indexes.begin(), indexes.end(), rng);
        // For each peer in random order, mock a ProcessMessages + SendMessages
        for (const auto index : indexes) {
            auto& peer = all_peers.at(index);
            NodeId nodeid = peer.m_nodeid;

            // Maybe connect the peer if we haven't already. Always connect if it's the good peer.
            if (nodeid == honestid ||
                (peer.m_connection_state == ConnectionState::NEVER_CONNECTED && fuzzed_data_provider.ConsumeBool())) {
                node::TxDownloadConnectionInfo info{peer.m_connection_preferred, peer.m_relay_permissions, peer.m_wtxid_relay};
                txdownloadman.ConnectedPeer(nodeid, info);
                peer.m_connection_state = ConnectionState::CONNECTED;
            }

            if (peer.m_connection_state == ConnectionState::CONNECTED) {
                // Peer will do its own ProcessMessages, i.e. respond to the next item in its message queue
                // If the message queue is empty or it decides to send nothing, maybe_gtxid is
                // std::nullopt. Otherwise, maybe_gtxid must contain the gtxid we are responding to.
                const auto& [peer_response, maybe_gtxid] = peer.ProcessOneMessage(fuzzed_data_provider);

                // Simulate a ProcessMessage.
                switch (peer_response) {
                    case PossibleResponse::TX_REAL:
                    {
                        // If the transaction exists, use ReceivedTx. Otherwise, maybe send notfound.
                        if (auto ptx{GetCorrectTx(TRANSACTIONS, maybe_gtxid.value())}) {
                            const auto& [should_validate, package_to_validate] = txdownloadman.ReceivedTx(nodeid, ptx);
                            if (should_validate || package_to_validate.has_value()) {
                                if (ptx->GetWitnessHash() == target_wtxid) {
                                    // We received the target tx and decided to validate it! Win!
                                    got_tx = true;
                                } else {
                                    // If not the target tx, pick some failure reason.
                                    TxValidationState state;
                                    state.Invalid(fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                                    txdownloadman.MempoolRejectedTx(ptx, state, nodeid, true);
                                }
                            } else {
                            }
                        } else {
                            Assert(!peer.m_good);
                            if (fuzzed_data_provider.ConsumeBool()) {
                                txdownloadman.ReceivedNotFound(nodeid, {maybe_gtxid.value().GetHash()});
                            }
                        }
                        break;
                    }
                    case PossibleResponse::TX_WITNESS_STRIPPED:
                    {
                        // Call ReceivedTx. If txdownloadman instructs us to validate the
                        // transaction, say the result was TX_WITNESS_STRIPPED.
                        if (auto stripped_tx{GetStrippedTx(TRANSACTIONS, maybe_gtxid.value())}) {
                            const auto& [should_validate, package] = txdownloadman.ReceivedTx(nodeid, stripped_tx);
                            if (should_validate) {
                                TxValidationState state;
                                state.Invalid(TxValidationResult::TX_WITNESS_STRIPPED, "");
                                txdownloadman.MempoolRejectedTx(stripped_tx, state, nodeid, true);
                            }
                        }
                        break;
                    }
                    case PossibleResponse::TX_WITNESS_MALLEATED:
                    {
                        // Call ReceivedTx. If txdownloadman instructs us to validate the
                        // transaction, return some error.
                        if (auto malleated_tx{GetMalleatedTx(TRANSACTIONS, maybe_gtxid.value())}) {

                            // Prevent accidental lose conditions.
                            if (malleated_tx == target_tx) break;

                            const auto& [should_validate, package_to_validate] = txdownloadman.ReceivedTx(nodeid, malleated_tx);
                            if (should_validate) {
                                TxValidationState state;
                                state.Invalid(fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                                txdownloadman.MempoolRejectedTx(malleated_tx, state, nodeid, true);
                            }
                        }
                        break;
                    }
                    case PossibleResponse::NOTFOUND:
                    {
                        txdownloadman.ReceivedNotFound(nodeid, {maybe_gtxid.value().GetHash()});
                        break;
                    }
                    case PossibleResponse::NOTHING:
                    {
                        break;
                    }
                    case PossibleResponse::DEFER:
                    {
                        // bug
                        Assert(false);
                        break;
                    }
                }

                // The peer may also send some unsolicited message. Simulate a ProcessMessage.
                // Ensure the good peer sends the inv within the first 1000 rounds.
                auto peer_unsolicited_message = peer.SendOneMessage(fuzzed_data_provider, /*force_announce=*/counter >= 1000);

                switch (peer_unsolicited_message) {
                    case PossibleMessage::NOTHING:
                    {
                        break;
                    }
                    case PossibleMessage::INV_REAL:
                    {
                        if (peer.m_wtxid_relay) {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Wtxid(target_wtxid), time, /*p2p_inv=*/true);
                        } else {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Txid(target_txid), time, /*p2p_inv=*/true);
                        }
                        break;
                    }
                    case PossibleMessage::INV_DIFFERENT:
                    {
                        // This may or may not be target_tx
                        if (auto random_tx{GetRandomTx(TRANSACTIONS, fuzzed_data_provider)}) {
                            if (peer.m_wtxid_relay) {
                                txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Txid(random_tx->GetHash()), time, /*p2p_inv=*/true);
                            } else {
                                txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Wtxid(random_tx->GetWitnessHash()), time, /*p2p_inv=*/true);
                            }
                        }
                        break;
                    }
                    case PossibleMessage::INV_RANDOM:
                    {
                        auto random_txhash = ConsumeUInt256(fuzzed_data_provider);
                        if (peer.m_wtxid_relay) {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Wtxid(random_txhash), time, /*p2p_inv=*/true);
                        } else {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Txid(random_txhash), time, /*p2p_inv=*/true);
                        }
                        break;
                    }
                    case PossibleMessage::TX_RANDOM:
                    {
                        if (auto random_tx{GetRandomTx(TRANSACTIONS, fuzzed_data_provider)}) {
                            // Disallow winning this way.
                            if (random_tx == target_tx) break;
                            const auto& [should_validate, package_to_validate] = txdownloadman.ReceivedTx(nodeid, random_tx);
                        }
                        break;
                    }
                    case PossibleMessage::NOTFOUND:
                    {
                        if (auto random_tx{GetRandomTx(TRANSACTIONS, fuzzed_data_provider)}) {
                            const auto txhash{fuzzed_data_provider.ConsumeBool() ? random_tx->GetHash().ToUint256() : random_tx->GetWitnessHash().ToUint256()};
                            txdownloadman.ReceivedNotFound(nodeid, {txhash});
                        }
                        break;
                    }
                }

                // Simulate a SendMessages, which includes sending getdata to request transactions.
                for (const auto& gtxid : txdownloadman.GetRequestsToSend(nodeid, time)) {
                    peer.m_getdata_queue.push_back(gtxid);
                }

                // Maybe disconnect the peer if we haven't already. Never disconnect the good peer.
                if (peer.m_connection_state == ConnectionState::CONNECTED && nodeid != honestid && fuzzed_data_provider.ConsumeBool()) {
                    peer.m_connection_state = ConnectionState::DISCONNECTED;
                    txdownloadman.DisconnectedPeer(nodeid);
                    txdownloadman.CheckIsEmpty(nodeid);
                }
                ++counter;
            }
        }

        time += std::chrono::milliseconds(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(100, 300));
    }
    // Disconnect everybody, check that all data structures are empty.
    for (const auto& peer : all_peers) {
        if (peer.m_nodeid == honestid) {
            // The honest peer only tells us about INV_REAL and then responds to the getdata, so
            // should have no state afterwards.
            txdownloadman.CheckIsEmpty(peer.m_nodeid);
        }
        txdownloadman.DisconnectedPeer(peer.m_nodeid);
        txdownloadman.CheckIsEmpty(peer.m_nodeid);
    }
    txdownloadman.CheckIsEmpty();

    // We must have received the transaction by now.
    Assert(got_tx);
}
} // namespace
