// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <node/miner.h>
#include <node/txdownloadman.h>
#include <node/txdownload_impl.h>
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
    /** Announce a transaction with the same txid but different witness. May be announced by txid
     * depending on the peer's m_wtxid_relay. */
    INV_SAME_TXID_DIFF_WITNESS,
    /** Send the target transaction, perhaps unsolicited */
    TX_REAL,
    /** Send a transaction with the witness stripped, perhaps unsolicited */
    TX_WITNESS_STRIPPED,
    /** Send a transaction with the witness malleated, perhaps unsolicited */
    TX_WITNESS_MALLEATED,
    /** Send a notfound for something that may or may not have been requested */
    NOTFOUND,
};

// What a peer may send in response to a getdata.
enum class PossibleResponse {
    /** Send the target transaction, in response to getdata */
    TX_REAL,
    /** Send a transaction with the witness stripped */
    TX_WITNESS_STRIPPED,
    /** Send a transaction with the witness malleated */
    TX_WITNESS_MALLEATED,
    /** Send a notfound for this getdata */
    NOTFOUND,
    /** Don't send anything (similar becoming unresponsive and/or stalling) */
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

static PossibleMessage ALL_MESSAGES[] = {
    PossibleMessage::NOTHING,
    PossibleMessage::INV_REAL,
    PossibleMessage::INV_SAME_TXID_DIFF_WITNESS,
    PossibleMessage::TX_REAL,
    PossibleMessage::TX_WITNESS_STRIPPED,
    PossibleMessage::TX_WITNESS_MALLEATED,
    PossibleMessage::NOTFOUND,
};
static PossibleResponse ALL_RESPONSES[] = {
    PossibleResponse::TX_REAL,
    PossibleResponse::TX_WITNESS_STRIPPED,
    PossibleResponse::TX_WITNESS_MALLEATED,
    PossibleResponse::NOTFOUND,
    PossibleResponse::NOTHING,
    PossibleResponse::DEFER,
};

class TestPeer {
public:
    // Queue of getdats to respond to: process from front, add to back.
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
        // "Good" peers send the inv at most once, and otherwise will not send an unsolicited message.
        if (m_good) {
            if (!m_sent_inv && (force_announce || fuzzed_data_provider.ConsumeBool())) {
                m_sent_inv = true;
                return PossibleMessage::INV_REAL;
            }
            return PossibleMessage::NOTHING;
        }

        // Non-good peers might send anything!
        auto message = fuzzed_data_provider.PickValueInArray(ALL_MESSAGES);
        if (message == PossibleMessage::INV_REAL) m_sent_inv = true;
        return message;
    }

    // Process up to one message from the front of m_getdata_queue. The size of m_getdata_queue may
    // not change, as DEFER may just push it to the back again.
    PossibleResponse ProcessOneMessage(FuzzedDataProvider& fuzzed_data_provider) {
        // Nothing to do if no messages to process
        if (m_getdata_queue.empty()) return PossibleResponse::NOTHING;

        auto request = m_getdata_queue.front();
        auto response{PossibleResponse::TX_REAL};
        if (!m_good) response = fuzzed_data_provider.PickValueInArray(ALL_RESPONSES);

        // If DEFER, pop the request to the back of the queue and return nothing.
        // This simulates responding out of order and/or stalling.
        if (response == PossibleResponse::DEFER) {
            m_getdata_queue.push_back(request);
            m_getdata_queue.pop_front();
            return PossibleResponse::NOTHING;
        }

        m_getdata_queue.pop_front();
        return response;
    }
};

TestPeer RandomPeer(FuzzedDataProvider& fuzzed_data_provider, NodeId nodeid, bool preferred, bool force_good)
{
    TestPeer peer;
    peer.m_nodeid = nodeid;
    peer.m_good = force_good || fuzzed_data_provider.ConsumeBool();
    peer.m_connection_preferred = preferred;
    peer.m_relay_permissions = false;
    peer.m_wtxid_relay = force_good || fuzzed_data_provider.ConsumeBool();
    return peer;
}

const TestingSetup* g_setup;
constexpr size_t NUM_COINS{10};
COutPoint COINS[NUM_COINS];

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
        COINS[i] = COutPoint{Txid::FromUint256((HashWriter() << i).GetHash()), i};
    }
}

// This test asserts that, as long as we have 1 good outbound peer, even if all other peers are
// malicious, we'll always be able to download a transaction.
// Connect to 8 outbounds and 0-120 inbound peers. 1 outbound is guaranteed to be good (see m_good
// above for definition), while the rest may or may not be.
// A non-good node has a superset of the actions available to a good node: it may stall a getdata,
// send a malleated or witness stripped tx/inv, send a notfound, etc.
FUZZ_TARGET(tx_download, .init = initialize)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // Initialize txdownloadman
    const auto& node = g_setup->m_node;
    CTxMemPool pool{MemPoolOptionsForTest(node)};
    const auto max_orphan_count = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 300);
    FastRandomContext det_rand{true};

    // Make target transaction, malleated, and stripped
    auto target_tx{MakeTransactionSpending(
        {fuzzed_data_provider.PickValueInArray(COINS)},
        /*num_outputs=*/2,
        /*add_witness=*/fuzzed_data_provider.ConsumeBool())
    };
    auto malleated_tx = Malleate(target_tx);
    auto stripped_tx = StripWitness(target_tx);
    const auto& target_txid = target_tx->GetHash().ToUint256();
    const auto& target_wtxid = target_tx->GetWitnessHash().ToUint256();
    const auto& malleated_wtxid = malleated_tx->GetWitnessHash().ToUint256();

    node::TxDownloadManager txdownloadman{node::TxDownloadOptions{pool, det_rand, max_orphan_count}};
    std::chrono::microseconds time{244466666};

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
    std::vector<size_t> indexes;
    indexes.resize(all_peers.size());
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
        Shuffle(indexes.begin(), indexes.end(), det_rand);
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
                auto maybe_notfound = peer.m_getdata_queue.empty() ? uint256::ZERO : peer.m_getdata_queue.front().GetHash();
                auto peer_response = peer.ProcessOneMessage(fuzzed_data_provider);

                // Simulate a ProcessMessage.
                switch (peer_response) {
                    case PossibleResponse::TX_REAL:
                    {
                        const auto& [should_validate, package_to_validate] = txdownloadman.ReceivedTx(nodeid, target_tx);
                        if (should_validate || package_to_validate.has_value()) {
                            got_tx = true;
                        }
                        break;
                    }
                    case PossibleResponse::TX_WITNESS_STRIPPED:
                    {
                        const auto& [should_validate, package] = txdownloadman.ReceivedTx(nodeid, stripped_tx);
                        if (should_validate) {
                            TxValidationState state;
                            state.Invalid(TxValidationResult::TX_WITNESS_STRIPPED, "");
                            txdownloadman.MempoolRejectedTx(stripped_tx, state, nodeid, true);
                        }
                        break;
                    }
                    case PossibleResponse::TX_WITNESS_MALLEATED:
                    {
                        const auto& [should_validate, package] = txdownloadman.ReceivedTx(nodeid, malleated_tx);
                        break;
                    }
                    case PossibleResponse::NOTFOUND:
                    {
                        txdownloadman.ReceivedNotFound(nodeid, {maybe_notfound});
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
                auto peer_unsolicited_message = peer.SendOneMessage(fuzzed_data_provider, /*force_announce=*/counter > 1000);

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
                    case PossibleMessage::INV_SAME_TXID_DIFF_WITNESS:
                    {
                        if (peer.m_wtxid_relay) {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Wtxid(malleated_wtxid), time, /*p2p_inv=*/true);
                        } else {
                            txdownloadman.AddTxAnnouncement(nodeid, GenTxid::Txid(target_txid), time, /*p2p_inv=*/true);
                        }
                        break;
                    }
                    case PossibleMessage::TX_REAL:
                    {
                        const auto& [should_validate, package_to_validate] = txdownloadman.ReceivedTx(nodeid, target_tx);
                        if (should_validate || package_to_validate.has_value()) {
                            got_tx = true;
                        }
                        break;
                    }
                    case PossibleMessage::TX_WITNESS_STRIPPED:
                    {
                        const auto& [should_validate, package] = txdownloadman.ReceivedTx(nodeid, stripped_tx);
                        if (should_validate) {
                            TxValidationState state;
                            state.Invalid(TxValidationResult::TX_WITNESS_STRIPPED, "");
                            txdownloadman.MempoolRejectedTx(stripped_tx, state, nodeid, true);
                        }
                        break;
                    }
                    case PossibleMessage::TX_WITNESS_MALLEATED:
                    {
                        const auto& [should_validate, package] = txdownloadman.ReceivedTx(nodeid, malleated_tx);
                        break;
                    }
                    case PossibleMessage::NOTFOUND:
                    {
                        if (fuzzed_data_provider.ConsumeBool()) {
                            txdownloadman.ReceivedNotFound(nodeid, {target_wtxid});
                        } else if (fuzzed_data_provider.ConsumeBool()) {
                            txdownloadman.ReceivedNotFound(nodeid, {target_txid});
                        } else {
                            txdownloadman.ReceivedNotFound(nodeid, {malleated_wtxid});
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

        // Skip time by 1-300ms
        time += std::chrono::microseconds(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 300) * 1000);
    }
    // Disconnect everybody, check that all data structures are empty.
    for (const auto& peer : all_peers) {
        txdownloadman.DisconnectedPeer(peer.m_nodeid);
        txdownloadman.CheckIsEmpty(peer.m_nodeid);
    }
    txdownloadman.CheckIsEmpty();

    // We must have received the transaction by now.
    Assert(got_tx);
}
} // namespace
