// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chain.h>
#include <consensus/validation.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <key.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <future>
#include <memory>
#include <vector>

namespace {

// A CValidationInterface that, when armed, stalls the validation-signals
// scheduler thread inside BlockConnected until Release() is called. Register
// this before the BlockFilterIndex so our callback fires first -- at that
// point the filter index's CustomAppend has not yet run, so its
// m_best_block_index has not advanced past whatever it was before the armed
// block(s).
class PreFilterBlocker final : public CValidationInterface
{
public:
    void Arm() { m_armed = true; }
    void WaitForEntry() { m_entered.get_future().wait(); }
    void Release()
    {
        if (!m_released.exchange(true)) m_release.set_value();
    }

protected:
    void BlockConnected(const kernel::ChainstateRole& role,
                        const std::shared_ptr<const CBlock>&,
                        const CBlockIndex*) override
    {
        if (!role.validated) return;
        if (m_armed.exchange(false)) {
            m_entered.set_value();
            m_release_fut.wait();
        }
    }

private:
    std::atomic_bool m_armed{false};
    std::atomic_bool m_released{false};
    std::promise<void> m_entered;
    std::promise<void> m_release;
    std::shared_future<void> m_release_fut{m_release.get_future()};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(cfilter_race_tests, TestChain100Setup)

// End-to-end: a getcfilters request that lands in the race window must not
// be answered immediately (the filter isn't built yet), must not be dropped
// either, and must be resolved by a later ProcessMessages call once the
// scheduler is released and CustomAppend has run. Drives the real
// ProcessMessage path via ConnmanTestMsg, using PreFilterBlocker for the
// same deterministic control over the race window as the racing-check tests
// above.
BOOST_AUTO_TEST_CASE(deferred_cfilter_response_reaches_peer)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    // Register the blocker BEFORE the filter index so our BlockConnected
    // fires first on the scheduler thread, parking it before the filter
    // index gets to run its own CustomAppend.
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    // Register a fresh global filter index bound to this test's chain,
    // clearing out anything a different test file may have left registered
    // in the shared g_filter_indexes registry.
    DestroyBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(InitBlockFilterIndex([&] { return interfaces::MakeChain(m_node); },
                                       BlockFilterType::BASIC, /*n_cache_size=*/1_MiB, /*f_memory=*/true));
    BlockFilterIndex* filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(filter_index);
    BOOST_REQUIRE(filter_index->Init());
    filter_index->Sync(); // backfills filters for the pre-mined 100 blocks

    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    m_node.connman->SetCaptureMessages(true);

    // Park the scheduler inside BlockConnected(new_block), before the filter
    // index's own CustomAppend runs.
    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);
    // sanity: the synced index has not yet processed the new block, so a
    // lookup for it is in the race window.
    IndexSummary summary{filter_index->GetSummary()};
    BOOST_REQUIRE(summary.synced);
    BOOST_REQUIRE(summary.best_block_hash != new_tip->GetBlockHash());

    // Simulate an inbound peer connecting and completing the version handshake.
    CNode peer{/*id=*/0,
              /*sock=*/nullptr,
              /*addrIn=*/CAddress{CService{}, NODE_NETWORK},
              /*nKeyedNetGroupIn=*/0,
              /*nLocalHostNonceIn=*/0,
              /*addrBindIn=*/CService{},
              /*addrNameIn=*/std::string{},
              /*conn_type_in=*/ConnectionType::INBOUND,
              /*inbound_onion=*/false,
              /*network_key=*/0};
    m_node.peerman->InitializeNode(peer, ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_COMPACT_FILTERS));

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERSION, PROTOCOL_VERSION,
                                                            uint64_t{NODE_NETWORK}, int64_t{0},
                                                            uint64_t{NODE_NETWORK}, CAddress::V1_NETWORK(CService{}))));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERACK)));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    // Watch for the CFILTER response being sent.
    bool cfilter_sent = false;
    const auto CaptureMessageOrig = CaptureMessage;
    CaptureMessage = [&](const CAddress&, const std::string& msg_type, std::span<const unsigned char>, bool is_incoming) {
        if (!is_incoming && msg_type == NetMsgType::CFILTER) cfilter_sent = true;
    };

    // Ask for the filter of the block whose CustomAppend hasn't run yet.
    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::GETCFILTERS,
                                                            static_cast<uint8_t>(BlockFilterType::BASIC),
                                                            static_cast<uint32_t>(new_tip->nHeight),
                                                            new_tip->GetBlockHash())));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);

    // Must not be answered yet -- the filter isn't built, and this must be
    // parked rather than dropped.
    BOOST_CHECK(!cfilter_sent);

    // Must still not resolve while the scheduler remains parked: the filter
    // index has made no progress, so the retry isn't attempted.
    connman.ProcessMessagesOnce(peer);
    BOOST_CHECK(!cfilter_sent);

    // Release the scheduler; CustomAppend runs, the filter lands on disk.
    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // The next ProcessMessages call must now resolve the parked request.
    connman.ProcessMessagesOnce(peer);
    BOOST_CHECK(cfilter_sent);

    CaptureMessage = CaptureMessageOrig;
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    connman.FlushSendBuffer(peer);
    DestroyBlockFilterIndex(BlockFilterType::BASIC);
}

// While a racing request is parked, no later message from the same peer may
// be processed (a ping goes unanswered), and once the race resolves the
// parked request's response must come before the response to the later
// message.
BOOST_AUTO_TEST_CASE(pending_request_pauses_message_processing)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    DestroyBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(InitBlockFilterIndex([&] { return interfaces::MakeChain(m_node); },
                                       BlockFilterType::BASIC, /*n_cache_size=*/1_MiB, /*f_memory=*/true));
    BlockFilterIndex* filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(filter_index);
    BOOST_REQUIRE(filter_index->Init());
    filter_index->Sync();

    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    m_node.connman->SetCaptureMessages(true);

    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);

    CNode peer{/*id=*/0,
              /*sock=*/nullptr,
              /*addrIn=*/CAddress{CService{}, NODE_NETWORK},
              /*nKeyedNetGroupIn=*/0,
              /*nLocalHostNonceIn=*/0,
              /*addrBindIn=*/CService{},
              /*addrNameIn=*/std::string{},
              /*conn_type_in=*/ConnectionType::INBOUND,
              /*inbound_onion=*/false,
              /*network_key=*/0};
    m_node.peerman->InitializeNode(peer, ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_COMPACT_FILTERS));

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERSION, PROTOCOL_VERSION,
                                                            uint64_t{NODE_NETWORK}, int64_t{0},
                                                            uint64_t{NODE_NETWORK}, CAddress::V1_NETWORK(CService{}))));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERACK)));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    std::vector<std::string> sent_types;
    const auto CaptureMessageOrig = CaptureMessage;
    CaptureMessage = [&](const CAddress&, const std::string& msg_type, std::span<const unsigned char>, bool is_incoming) {
        if (!is_incoming && (msg_type == NetMsgType::CFILTER || msg_type == NetMsgType::PONG)) {
            sent_types.push_back(msg_type);
        }
    };

    // A racing getcfilters request, followed by a ping.
    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::GETCFILTERS,
                                                            static_cast<uint8_t>(BlockFilterType::BASIC),
                                                            static_cast<uint32_t>(new_tip->nHeight),
                                                            new_tip->GetBlockHash())));
    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::PING, uint64_t{0x1234})));
    peer.fPauseSend = false;

    // The first call parks the getcfilters request; further calls must not
    // touch the ping while it is parked.
    connman.ProcessMessagesOnce(peer);
    connman.ProcessMessagesOnce(peer);
    connman.ProcessMessagesOnce(peer);
    BOOST_CHECK(sent_types.empty());

    // Release the scheduler; CustomAppend runs, the filter lands on disk.
    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // One call resolves the parked request, the next answers the ping: the
    // responses arrive in the order the requests were made. (The response
    // push flips the test node's fPauseSend, so reset it between calls.)
    connman.ProcessMessagesOnce(peer);
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    BOOST_REQUIRE_EQUAL(sent_types.size(), 2U);
    BOOST_CHECK_EQUAL(sent_types[0], NetMsgType::CFILTER);
    BOOST_CHECK_EQUAL(sent_types[1], NetMsgType::PONG);

    CaptureMessage = CaptureMessageOrig;
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    connman.FlushSendBuffer(peer);
    DestroyBlockFilterIndex(BlockFilterType::BASIC);
}

// The same park/resume behaviour must cover getcfheaders: for a
// BIP157-following client this is the request most likely to race the index,
// since it fetches and verifies filter headers the moment it learns of a new
// block, and only then downloads the filter itself.
BOOST_AUTO_TEST_CASE(deferred_cfheaders_response_reaches_peer)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    DestroyBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(InitBlockFilterIndex([&] { return interfaces::MakeChain(m_node); },
                                       BlockFilterType::BASIC, /*n_cache_size=*/1_MiB, /*f_memory=*/true));
    BlockFilterIndex* filter_index = GetBlockFilterIndex(BlockFilterType::BASIC);
    BOOST_REQUIRE(filter_index);
    BOOST_REQUIRE(filter_index->Init());
    filter_index->Sync();

    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    m_node.connman->SetCaptureMessages(true);

    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);

    CNode peer{/*id=*/0,
               /*sock=*/nullptr,
               /*addrIn=*/CAddress{CService{}, NODE_NETWORK},
               /*nKeyedNetGroupIn=*/0,
               /*nLocalHostNonceIn=*/0,
               /*addrBindIn=*/CService{},
               /*addrNameIn=*/std::string{},
               /*conn_type_in=*/ConnectionType::INBOUND,
               /*inbound_onion=*/false,
               /*network_key=*/0};
    m_node.peerman->InitializeNode(peer, ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_COMPACT_FILTERS));

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERSION, PROTOCOL_VERSION,
                                                            uint64_t{NODE_NETWORK}, int64_t{0},
                                                            uint64_t{NODE_NETWORK}, CAddress::V1_NETWORK(CService{}))));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERACK)));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);
    m_node.peerman->SendMessages(peer);
    connman.FlushSendBuffer(peer);

    bool cfheaders_sent = false;
    const auto CaptureMessageOrig = CaptureMessage;
    CaptureMessage = [&](const CAddress&, const std::string& msg_type, std::span<const unsigned char>, bool is_incoming) {
        if (!is_incoming && msg_type == NetMsgType::CFHEADERS) cfheaders_sent = true;
    };

    // Ask for the filter headers up to the block whose CustomAppend hasn't
    // run yet.
    BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::GETCFHEADERS,
                                                            static_cast<uint8_t>(BlockFilterType::BASIC),
                                                            static_cast<uint32_t>(new_tip->nHeight),
                                                            new_tip->GetBlockHash())));
    peer.fPauseSend = false;
    connman.ProcessMessagesOnce(peer);

    // Must be parked, not dropped, and must not resolve while the scheduler
    // remains parked.
    BOOST_CHECK(!cfheaders_sent);
    connman.ProcessMessagesOnce(peer);
    BOOST_CHECK(!cfheaders_sent);

    // Release the scheduler; CustomAppend runs, the filter lands on disk.
    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    connman.ProcessMessagesOnce(peer);
    BOOST_CHECK(cfheaders_sent);

    CaptureMessage = CaptureMessageOrig;
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    connman.FlushSendBuffer(peer);
    DestroyBlockFilterIndex(BlockFilterType::BASIC);
}

BOOST_AUTO_TEST_SUITE_END()
