// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>
#include <governance/governance.h>
#include <governance/net_governance.h>
#include <governance/object.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netfulfilledman.h>
#include <node/connection_types.h>
#include <protocol.h>
#include <scheduler.h>
#include <streams.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <version.h>

#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {
struct GovernanceInvSetup : public TestingSetup {
    GovernanceInvSetup() : TestingSetup{CBaseChainParams::MAIN}
    {
        // ConfirmInventoryRequest and CheckAndRemove short-circuit on
        // !IsBlockchainSynced(); CheckAndRemove also asserts metaman.IsValid().
        // NetGovernance::AlreadyHave gates on m_gov_manager.IsValid().
        BOOST_REQUIRE(m_node.mn_sync);
        m_node.mn_sync->SwitchToNextAsset();
        BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());

        BOOST_REQUIRE(m_node.mn_metaman);
        BOOST_REQUIRE(m_node.mn_metaman->LoadCache(/*load_cache=*/false));

        BOOST_REQUIRE(m_node.govman);
        // Match runtime preconditions: NetGovernance::AlreadyHave claims we
        // already have the inv when governance isn't loaded (e.g.
        // -disablegovernance), so ConfirmInventoryRequest would never run.
        BOOST_REQUIRE(m_node.govman->LoadCache(/*load_cache=*/false));

        BOOST_REQUIRE(m_node.netfulfilledman);
        // Loaded here for the later test that advances GOVERNANCE -> FINISHED;
        // the sync notifier asserts netfulfilledman.IsValid().
        BOOST_REQUIRE(m_node.netfulfilledman->LoadCache(/*load_cache=*/false));
        BOOST_REQUIRE(m_node.connman);
        BOOST_REQUIRE(m_node.peerman);

        // Intentional unit-test boundary: TestingSetup does not run the
        // init.cpp/AppInit startup path that registers the Dash-specific
        // handlers, so the INV branch in PeerManagerImpl::AlreadyHave would not
        // route MSG_GOVERNANCE_OBJECT[_VOTE] anywhere. Install the same
        // NetGovernance handler init.cpp registers so a real INV reaches
        // CGovernanceManager::ConfirmInventoryRequest; the startup registration
        // itself stays outside this unit test.
        m_node.peerman->AddExtraHandler(std::make_unique<NetGovernance>(
            m_node.peerman.get(), *m_node.govman, *m_node.mn_sync,
            *m_node.netfulfilledman, *m_node.connman));

        // Anchor the mocked clock so SetMockTime advances are deterministic.
        SetMockTime(1'700'000'000s);
    }
};

size_t CountQueuedMessages(const CNode& peer, const std::string& msg_type)
{
    LOCK(peer.cs_vSend);
    size_t count{0};
    for (const auto& msg : peer.vSendMsg) {
        if (msg.m_type == msg_type) {
            ++count;
        }
    }
    return count;
}

size_t CountQueuedInventory(const CNode& peer, const CInv& expected_inv)
{
    LOCK(peer.cs_vSend);
    size_t count{0};
    for (const auto& msg : peer.vSendMsg) {
        if (msg.m_type != NetMsgType::INV) {
            continue;
        }
        CDataStream stream{msg.data, SER_NETWORK, PROTOCOL_VERSION};
        std::vector<CInv> invs;
        stream >> invs;
        for (const auto& inv : invs) {
            if (inv.type == expected_inv.type && inv.hash == expected_inv.hash) {
                ++count;
            }
        }
    }
    return count;
}

std::unique_ptr<CNode> MakeGovernanceInvPeer(NodeId id)
{
    in_addr peer_in_addr{};
    peer_in_addr.s_addr = htonl(0x01020305 + id);
    auto peer{std::make_unique<CNode>(id,
                                      /*sock=*/nullptr,
                                      /*addrIn=*/CAddress{CService{peer_in_addr, 8333}, NODE_NETWORK},
                                      /*nKeyedNetGroupIn=*/0,
                                      /*nLocalHostNonceIn=*/0,
                                      /*addrBindIn=*/CAddress{},
                                      /*addrNameIn=*/std::string{},
                                      /*conn_type_in=*/ConnectionType::INBOUND,
                                      /*inbound_onion=*/false)};
    peer->nVersion = PROTOCOL_VERSION;
    peer->SetCommonVersion(PROTOCOL_VERSION);
    peer->fSuccessfullyConnected = true;
    return peer;
}

void ProcessInv(PeerManager& peerman, CNode& peer, const CInv& inv)
    EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
{
    CDataStream inv_stream{SER_NETWORK, PROTOCOL_VERSION};
    inv_stream << std::vector<CInv>{inv};
    std::atomic<bool> interrupt_dummy{false};
    peerman.ProcessMessage(peer, NetMsgType::INV, inv_stream, GetTime<std::chrono::microseconds>(), interrupt_dummy);
}

CGovernanceObject MakeGovernanceObject(int64_t creation_time, const uint256& collateral_hash)
{
    const std::string data{
        R"({"type":1,"name":"proposal","start_epoch":1700000000,"end_epoch":1700100000,"payment_amount":1.0,"payment_address":"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwV","url":"https://dash.org"})"};
    return CGovernanceObject{uint256{}, /*revision=*/1, creation_time, collateral_hash, HexStr(data)};
}

void ProcessGovernanceObject(NetGovernance& net_gov, CNode& peer, const CGovernanceObject& govobj)
{
    CDataStream object_stream{SER_NETWORK, PROTOCOL_VERSION};
    object_stream << govobj;
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCEOBJECT, object_stream);
}

CGovernanceVote MakeGovernanceVote(const uint256& parent_hash)
{
    CGovernanceVote vote{COutPoint{uint256S("11"), 1}, parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES};
    vote.SetTime(GetTime<std::chrono::seconds>().count());
    vote.SetSignature(std::vector<unsigned char>(CGovernanceVote::COMPACT_SIG_SIZE));
    return vote;
}

void ProcessGovernanceVote(NetGovernance& net_gov, CNode& peer, const CGovernanceVote& vote)
{
    CDataStream vote_stream{SER_NETWORK, PROTOCOL_VERSION};
    vote_stream << vote;
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCEOBJECTVOTE, vote_stream);
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(governance_inv_tests, GovernanceInvSetup)

BOOST_AUTO_TEST_CASE(per_object_vote_sync_is_fulfilled_request_limited)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    // NetGovernance::ProcessMessage ignores MNGOVERNANCESYNC until sync is
    // fully finished; advance from GOVERNANCE to FINISHED.
    m_node.mn_sync->SwitchToNextAsset();
    BOOST_REQUIRE(m_node.mn_sync->IsSynced());

    in_addr peer_in_addr{};
    peer_in_addr.s_addr = htonl(0x01020305);
    CNode peer{/*id=*/1,
               /*sock=*/nullptr,
               /*addrIn=*/CAddress{CService{peer_in_addr, 8333}, NODE_NETWORK},
               /*nKeyedNetGroupIn=*/0,
               /*nLocalHostNonceIn=*/0,
               /*addrBindIn=*/CAddress{},
               /*addrNameIn=*/std::string{},
               /*conn_type_in=*/ConnectionType::INBOUND,
               /*inbound_onion=*/false};
    peer.nVersion = PROTOCOL_VERSION;
    peer.SetCommonVersion(PROTOCOL_VERSION);
    m_node.peerman->InitializeNode(peer, NODE_NETWORK);
    peer.fSuccessfullyConnected = true;

    auto make_request_stream = [](const uint256& object_hash, const CBloomFilter& filter) {
        CDataStream stream{SER_NETWORK, PROTOCOL_VERSION};
        stream << object_hash << filter;
        return stream;
    };

    NetGovernance net_gov(m_node.peerman.get(), *m_node.govman, *m_node.mn_sync,
                          *m_node.netfulfilledman, *m_node.connman);
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    CNodeStateStats stats;
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    const CBloomFilter vote_filter{1, 0.001, 0, BLOOM_UPDATE_NONE};
    CGovernanceObject postponed_object{uint256(), /*revision=*/1, GetTime(), uint256::ONE, /*data=*/{}};
    m_node.govman->AddPostponedObject(postponed_object);

    const uint256 postponed_object_hash{postponed_object.GetHash()};
    const std::string postponed_vote_sync_request{strprintf("%s-votes-%s", NetMsgType::MNGOVERNANCESYNC,
                                                            postponed_object_hash.ToString())};
    auto postponed_stream = make_request_stream(postponed_object_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, postponed_stream);

    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, postponed_vote_sync_request));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    auto duplicate_postponed_stream = make_request_stream(postponed_object_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_postponed_stream);

    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    CGovernanceObject syncable_object{uint256(), /*revision=*/1, GetTime() + 1, uint256S("02"), /*data=*/{}};
    m_node.govman->AddGovernanceObjectForTesting(syncable_object);

    const uint256 syncable_object_hash{syncable_object.GetHash()};
    const std::string syncable_vote_sync_request{strprintf("%s-votes-%s", NetMsgType::MNGOVERNANCESYNC,
                                                           syncable_object_hash.ToString())};
    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, syncable_vote_sync_request));

    connman.FlushSendBuffer(peer);
    auto syncable_object_fetch_stream = make_request_stream(syncable_object_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, syncable_object_fetch_stream);

    BOOST_CHECK_EQUAL(CountQueuedInventory(peer, CInv{MSG_GOVERNANCE_OBJECT, syncable_object_hash}), 1U);
    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, syncable_vote_sync_request));
    BOOST_CHECK_EQUAL(CountQueuedMessages(peer, NetMsgType::SYNCSTATUSCOUNT), 0U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    connman.FlushSendBuffer(peer);
    auto duplicate_syncable_object_fetch_stream = make_request_stream(syncable_object_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_syncable_object_fetch_stream);

    BOOST_CHECK_EQUAL(CountQueuedInventory(peer, CInv{MSG_GOVERNANCE_OBJECT, syncable_object_hash}), 1U);
    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, syncable_vote_sync_request));
    BOOST_CHECK_EQUAL(CountQueuedMessages(peer, NetMsgType::SYNCSTATUSCOUNT), 0U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    connman.FlushSendBuffer(peer);
    auto syncable_stream = make_request_stream(syncable_object_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, syncable_stream);

    BOOST_CHECK(m_node.netfulfilledman->HasFulfilledRequest(peer.addr, syncable_vote_sync_request));
    BOOST_CHECK_EQUAL(CountQueuedMessages(peer, NetMsgType::SYNCSTATUSCOUNT), 1U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    auto duplicate_syncable_stream = make_request_stream(syncable_object_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_syncable_stream);

    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    connman.FlushSendBuffer(peer);
    CGovernanceObject empty_filter_object{uint256(), /*revision=*/1, GetTime() + 2, uint256S("03"), /*data=*/{}};
    m_node.govman->AddPostponedObject(empty_filter_object);
    const uint256 empty_filter_object_hash{empty_filter_object.GetHash()};
    const std::string empty_filter_vote_request{strprintf("%s-votes-%s", NetMsgType::MNGOVERNANCESYNC,
                                                          empty_filter_object_hash.ToString())};
    auto empty_filter_stream = make_request_stream(empty_filter_object_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, empty_filter_stream);

    BOOST_CHECK_EQUAL(CountQueuedInventory(peer, CInv{MSG_GOVERNANCE_OBJECT, empty_filter_object_hash}), 1U);
    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, empty_filter_vote_request));
    BOOST_CHECK_EQUAL(CountQueuedMessages(peer, NetMsgType::SYNCSTATUSCOUNT), 0U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    connman.FlushSendBuffer(peer);
    auto duplicate_empty_filter_stream = make_request_stream(empty_filter_object_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_empty_filter_stream);

    BOOST_CHECK_EQUAL(CountQueuedInventory(peer, CInv{MSG_GOVERNANCE_OBJECT, empty_filter_object_hash}), 1U);
    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, empty_filter_vote_request));
    BOOST_CHECK_EQUAL(CountQueuedMessages(peer, NetMsgType::SYNCSTATUSCOUNT), 0U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    const uint256 unknown_vote_hash{uint256S("09")};
    const std::string unknown_vote_request{strprintf("%s-votes-%s", NetMsgType::MNGOVERNANCESYNC,
                                                     unknown_vote_hash.ToString())};
    auto unknown_vote_stream = make_request_stream(unknown_vote_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, unknown_vote_stream);

    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, unknown_vote_request));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    auto duplicate_unknown_vote_stream = make_request_stream(unknown_vote_hash, vote_filter);
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_unknown_vote_stream);

    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, unknown_vote_request));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    const uint256 object_fetch_hash{uint256S("0a")};
    const std::string object_fetch_request{strprintf("%s-votes-%s", NetMsgType::MNGOVERNANCESYNC,
                                                     object_fetch_hash.ToString())};
    auto object_fetch_stream = make_request_stream(object_fetch_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, object_fetch_stream);

    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, object_fetch_request));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    auto duplicate_object_fetch_stream = make_request_stream(object_fetch_hash, CBloomFilter{});
    net_gov.ProcessMessage(peer, NetMsgType::MNGOVERNANCESYNC, duplicate_object_fetch_stream);

    BOOST_CHECK(!m_node.netfulfilledman->HasFulfilledRequest(peer.addr, object_fetch_request));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(peer.GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 20);

    m_node.peerman->FinalizeNode(peer);
}

BOOST_AUTO_TEST_CASE(governance_objects_require_peer_announcement_or_request)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    TestChainState& chainstate =
        *static_cast<TestChainState*>(&m_node.chainman->ActiveChainstate());
    chainstate.JumpOutOfIbd();

    NetGovernance net_gov(m_node.peerman.get(), *m_node.govman, *m_node.mn_sync,
                          *m_node.netfulfilledman, *m_node.connman);

    auto announcing_peer{MakeGovernanceInvPeer(/*id=*/11)};
    auto second_announcing_peer{MakeGovernanceInvPeer(/*id=*/12)};
    auto unsolicited_peer{MakeGovernanceInvPeer(/*id=*/13)};
    m_node.peerman->InitializeNode(*announcing_peer, NODE_NETWORK);
    m_node.peerman->InitializeNode(*second_announcing_peer, NODE_NETWORK);
    m_node.peerman->InitializeNode(*unsolicited_peer, NODE_NETWORK);

    CNodeStateStats stats;
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(announcing_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    const CGovernanceObject govobj{MakeGovernanceObject(GetTime<std::chrono::seconds>().count(), uint256S("21"))};
    const CInv object_inv{MSG_GOVERNANCE_OBJECT, govobj.GetHash()};

    ProcessInv(*m_node.peerman, *announcing_peer, object_inv);
    ProcessInv(*m_node.peerman, *second_announcing_peer, object_inv);
    // Pre-seed the object so ProcessObject short-circuits on "already seen" and the
    // acceptance gate is exercised in isolation from collateral/chain validation. Both
    // ProcessInv calls ran first, so the object was still absent at INV time and both
    // peers registered a real pending request in the net-layer tracker.
    m_node.govman->AddPostponedObject(govobj);

    // Announcing peer: the gate accepts (it announced the inv) and consumes its per-peer
    // request entry. Checking the entry is gone proves the gate actually consulted the
    // net-layer tracker, independent of the pre-seed above (a rejected object would leave
    // the entry untouched).
    ProcessGovernanceObject(net_gov, *announcing_peer, govobj);
    BOOST_CHECK(
        !WITH_LOCK(::cs_main, return m_node.peerman->PeerConsumeObjectRequest(announcing_peer->GetId(), object_inv)));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(announcing_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    ProcessGovernanceObject(net_gov, *second_announcing_peer, govobj);
    // Consumption is per-peer: the second announcer's own entry is accepted and consumed,
    // independent of the first peer's already-consumed entry.
    BOOST_CHECK(!WITH_LOCK(::cs_main,
                           return m_node.peerman->PeerConsumeObjectRequest(second_announcing_peer->GetId(), object_inv)));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(second_announcing_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    const CGovernanceObject unsolicited_govobj{MakeGovernanceObject(GetTime<std::chrono::seconds>().count() + 1, uint256S("22"))};
    ProcessGovernanceObject(net_gov, *unsolicited_peer, unsolicited_govobj);
    BOOST_CHECK(!m_node.govman->HaveObjectForHash(unsolicited_govobj.GetHash()));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(unsolicited_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    m_node.peerman->FinalizeNode(*announcing_peer);
    m_node.peerman->FinalizeNode(*second_announcing_peer);
    m_node.peerman->FinalizeNode(*unsolicited_peer);
    chainstate.ResetIbd();
}

BOOST_AUTO_TEST_CASE(governance_votes_require_peer_announcement_or_request)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    TestChainState& chainstate =
        *static_cast<TestChainState*>(&m_node.chainman->ActiveChainstate());
    chainstate.JumpOutOfIbd();

    NetGovernance net_gov(m_node.peerman.get(), *m_node.govman, *m_node.mn_sync,
                          *m_node.netfulfilledman, *m_node.connman);

    auto announcing_peer{MakeGovernanceInvPeer(/*id=*/21)};
    auto second_announcing_peer{MakeGovernanceInvPeer(/*id=*/22)};
    auto unsolicited_peer{MakeGovernanceInvPeer(/*id=*/23)};
    m_node.peerman->InitializeNode(*announcing_peer, NODE_NETWORK);
    m_node.peerman->InitializeNode(*second_announcing_peer, NODE_NETWORK);
    m_node.peerman->InitializeNode(*unsolicited_peer, NODE_NETWORK);

    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    CNodeStateStats stats;

    const CGovernanceVote vote{MakeGovernanceVote(uint256S("31"))};
    const CInv vote_inv{MSG_GOVERNANCE_OBJECT_VOTE, vote.GetHash()};

    ProcessGovernanceVote(net_gov, *unsolicited_peer, vote);
    BOOST_CHECK_EQUAL(CountQueuedMessages(*unsolicited_peer, NetMsgType::MNGOVERNANCESYNC), 0U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(unsolicited_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    ProcessInv(*m_node.peerman, *announcing_peer, vote_inv);
    ProcessInv(*m_node.peerman, *second_announcing_peer, vote_inv);

    connman.FlushSendBuffer(*announcing_peer);
    ProcessGovernanceVote(net_gov, *announcing_peer, vote);
    BOOST_CHECK_EQUAL(CountQueuedMessages(*announcing_peer, NetMsgType::MNGOVERNANCESYNC), 1U);
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(announcing_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    connman.FlushSendBuffer(*second_announcing_peer);
    ProcessGovernanceVote(net_gov, *second_announcing_peer, vote);
    // Second announcer: the gate accepts (it independently announced the vote) and consumes
    // its per-peer request entry. A rejected vote would return before the gate consumes and
    // leave the entry intact, so this proves the accept path independently of the (deduped)
    // orphan-request side effect.
    BOOST_CHECK(!WITH_LOCK(::cs_main,
                           return m_node.peerman->PeerConsumeObjectRequest(second_announcing_peer->GetId(), vote_inv)));
    BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(second_announcing_peer->GetId(), stats));
    BOOST_CHECK_EQUAL(stats.m_misbehavior_score, 0);

    m_node.peerman->FinalizeNode(*announcing_peer);
    m_node.peerman->FinalizeNode(*second_announcing_peer);
    m_node.peerman->FinalizeNode(*unsolicited_peer);
    chainstate.ResetIbd();
}

// A message received while not blockchain-synced is dropped, but the per-peer authorization must
// survive so a retransmit after sync is still accepted (the consume happens after the sync gate).
BOOST_AUTO_TEST_CASE(governance_vote_authorization_survives_unsynced_drop)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    TestChainState& chainstate =
        *static_cast<TestChainState*>(&m_node.chainman->ActiveChainstate());
    chainstate.JumpOutOfIbd();

    NetGovernance net_gov(m_node.peerman.get(), *m_node.govman, *m_node.mn_sync,
                          *m_node.netfulfilledman, *m_node.connman);

    auto peer{MakeGovernanceInvPeer(/*id=*/31)};
    m_node.peerman->InitializeNode(*peer, NODE_NETWORK);
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);

    const CGovernanceVote vote{MakeGovernanceVote(uint256S("41"))};
    const CInv vote_inv{MSG_GOVERNANCE_OBJECT_VOTE, vote.GetHash()};

    // Synced: announce the vote so we hold a per-peer request for it.
    BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());
    ProcessInv(*m_node.peerman, *peer, vote_inv);

    // Not synced: delivering the vote is dropped at the sync gate and must NOT consume the request.
    m_node.mn_sync->Reset(/*fForce=*/true, /*fNotifyReset=*/false);
    BOOST_REQUIRE(!m_node.mn_sync->IsBlockchainSynced());
    connman.FlushSendBuffer(*peer);
    ProcessGovernanceVote(net_gov, *peer, vote);
    BOOST_CHECK_EQUAL(CountQueuedMessages(*peer, NetMsgType::MNGOVERNANCESYNC), 0U);

    // Back in sync, the retransmit is still authorized: ProcessVote runs and (orphan parent)
    // requests the missing object. Had the unsynced drop consumed the request, the gate would now
    // reject the vote as unrequested and send no MNGOVERNANCESYNC.
    m_node.mn_sync->SwitchToNextAsset();
    BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());
    connman.FlushSendBuffer(*peer);
    ProcessGovernanceVote(net_gov, *peer, vote);
    BOOST_CHECK_EQUAL(CountQueuedMessages(*peer, NetMsgType::MNGOVERNANCESYNC), 1U);

    m_node.peerman->FinalizeNode(*peer);
    chainstate.ResetIbd();
}

BOOST_AUTO_TEST_SUITE_END()
