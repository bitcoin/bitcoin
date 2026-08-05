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

BOOST_AUTO_TEST_CASE(is_racing_true_within_window)
{
    // Register the blocker BEFORE the filter index so our BlockConnected fires
    // first on the scheduler thread, parking it before the filter index gets to
    // run its own CustomAppend.
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync(); // backfills filters for the pre-mined 100 blocks

    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);

    // Block until the scheduler thread has reached our gate. At this point the
    // new block is on the chain, but the filter index's BlockConnected has NOT
    // yet run (CustomAppend not called). This is the exact window a peer's
    // getcfilters could land in.
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);

    // IsRacing is a cheap, synchronous check -- no thread or timeout needed.
    BOOST_CHECK(filter_index.IsRacing(new_tip));

    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(is_racing_false_beyond_window)
{
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    // Park the scheduler on the first of several blocks mined back to back.
    // Since the scheduler is a single serial queue, none of their
    // BlockConnected notifications run until Release(), so the index's
    // m_best_block_index stays behind while the active chain races ahead.
    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    std::vector<CBlock> new_blocks;
    new_blocks.reserve(3);
    for (int i = 0; i < 3; ++i) {
        new_blocks.push_back(CreateAndProcessBlock({}, coinbase_script));
    }
    blocker->WaitForEntry();

    std::vector<const CBlockIndex*> new_tips;
    {
        LOCK(cs_main);
        for (const auto& block : new_blocks) {
            new_tips.push_back(m_node.chainman->m_blockman.LookupBlockIndex(block.GetHash()));
        }
    }
    for (const auto* tip : new_tips) BOOST_REQUIRE(tip);

    // CF_MAX_BLOCKS_AHEAD_RACE_WAIT is 2: 1 and 2 blocks ahead of the index's
    // last processed block are still a plausible race; 3 blocks ahead is not
    // -- something other than a simple race is going on, and waiting won't help.
    BOOST_CHECK(filter_index.IsRacing(new_tips[0]));
    BOOST_CHECK(filter_index.IsRacing(new_tips[1]));
    BOOST_CHECK(!filter_index.IsRacing(new_tips[2]));

    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(is_racing_false_for_stale_miss)
{
    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync(); // m_best_block_index catches up to the tip (height 100)

    // A block well behind the index's last processed block is a genuine miss,
    // not a race -- it is not in flight, and no amount of waiting would help.
    const CBlockIndex* old_block;
    {
        LOCK(cs_main);
        old_block = m_node.chainman->ActiveChain()[1];
    }
    BOOST_REQUIRE(old_block);
    BOOST_CHECK(!filter_index.IsRacing(old_block));

    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(is_racing_false_when_not_synced)
{
    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    // Deliberately do not call Sync() -- the index has not caught up yet, so
    // even the current tip must not be treated as a plausible race.

    const CBlockIndex* tip;
    {
        LOCK(cs_main);
        tip = m_node.chainman->ActiveChain().Tip();
    }
    BOOST_REQUIRE(tip);
    BOOST_CHECK(!filter_index.IsRacing(tip));

    filter_index.Interrupt();
    filter_index.Stop();
}

// Same-height reorg: the index's last processed block A and the requested
// block B sit at the same height on different branches. When BlockConnected(B)
// is queued but not yet processed, IsRacing(B) must still say yes. This
// exercises the `ahead == 0` branch of the helper.
BOOST_AUTO_TEST_CASE(is_racing_true_for_same_height_reorg)
{
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    // Mine A at height 101 and let the filter index fully process it, so
    // m_best_block_index == A.
    const CScript script_A = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock block_A = CreateAndProcessBlock({}, script_A);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    CBlockIndex* a_index;
    {
        LOCK(cs_main);
        a_index = m_node.chainman->m_blockman.LookupBlockIndex(block_A.GetHash());
    }
    BOOST_REQUIRE(a_index);

    // Invalidate A to roll the active chain back to height 100. BaseIndex
    // does not act on BlockDisconnected, so the index's m_best_block_index
    // stays pointing at A -- the divergence is only resolved by the next
    // BlockConnected, which is exactly the race we want to exercise.
    {
        BlockValidationState state;
        BOOST_REQUIRE(m_node.chainman->ActiveChainstate().InvalidateBlock(state, a_index));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    blocker->Arm();

    // Mine a sibling B at height 101 with a different coinbase so the hash differs.
    const CKey other_key{GenerateRandomKey()};
    const CScript script_B = GetScriptForDestination(PKHash(other_key.GetPubKey()));
    const CBlock block_B = CreateAndProcessBlock({}, script_B);
    BOOST_REQUIRE(block_B.GetHash() != block_A.GetHash());

    blocker->WaitForEntry();

    const CBlockIndex* b_index;
    {
        LOCK(cs_main);
        b_index = m_node.chainman->m_blockman.LookupBlockIndex(block_B.GetHash());
    }
    BOOST_REQUIRE(b_index);
    BOOST_REQUIRE_EQUAL(b_index->nHeight, a_index->nHeight);

    // Scheduler is parked inside BlockConnected(B). Index state:
    //   m_best_block_index = A  (stale branch, height 101)
    //   target             = B  (active branch, height 101)
    //   ahead              = 0  -> must be treated as racing.
    BOOST_CHECK(filter_index.IsRacing(b_index));

    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

// End-to-end: a getcfilters request that lands in the race window must not
// be answered immediately (the filter isn't built yet), must not be dropped
// either, and must be resolved by the next SendMessages call once the
// scheduler is released and CustomAppend has run. Drives the real
// ProcessMessage/SendMessages path via ConnmanTestMsg, using PreFilterBlocker
// for the same deterministic control over the race window as the IsRacing
// tests above.
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
    BOOST_REQUIRE(filter_index->IsRacing(new_tip)); // sanity: we are actually in the window

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
    // deferred rather than dropped.
    BOOST_CHECK(!cfilter_sent);

    // Must still not resolve while the scheduler remains parked.
    m_node.peerman->SendMessages(peer);
    BOOST_CHECK(!cfilter_sent);

    // Release the scheduler; CustomAppend runs, the filter lands on disk.
    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // The next SendMessages call must now resolve the deferred request.
    m_node.peerman->SendMessages(peer);
    BOOST_CHECK(cfilter_sent);

    CaptureMessage = CaptureMessageOrig;
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    connman.FlushSendBuffer(peer);
    DestroyBlockFilterIndex(BlockFilterType::BASIC);
}

BOOST_AUTO_TEST_SUITE_END()
