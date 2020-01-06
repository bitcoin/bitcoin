// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Unit tests for denial-of-service detection/prevention code

#include <banman.h>
#include <chainparams.h>
#include <net.h>
#include <net_processing.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <serialize.h>
#include <util/memory.h>
#include <util/system.h>
#include <util/time.h>
#include <validation.h>

#include <test/setup_common.h>

#include <stdint.h>

#include <boost/test/unit_test.hpp>

struct CConnmanTest : public CConnman {
    using CConnman::CConnman;
    void AddNode(CNode& node)
    {
        LOCK(cs_vNodes);
        vNodes.push_back(&node);
    }
    void ClearNodes()
    {
        LOCK(cs_vNodes);
        for (CNode* node : vNodes) {
            delete node;
        }
        vNodes.clear();
    }
};

// Tests these internal-to-net_processing.cpp methods:
extern bool AddOrphanTx(const CTransactionRef& tx, NodeId peer);
extern void EraseOrphansFor(NodeId peer);
extern unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans);
extern void Misbehaving(NodeId nodeid, int howmuch, const std::string& message="");

struct COrphanTx {
    CTransactionRef tx;
    NodeId fromPeer;
    int64_t nTimeExpire;
};
extern CCriticalSection g_cs_orphans;
extern std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(g_cs_orphans);

static CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), Params().GetDefaultPort());
}

static NodeId id = 0;

void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds);

BOOST_FIXTURE_TEST_SUITE(denialofservice_tests, TestingSetup)

// Test eviction of an outbound peer whose chain never advances
// Mock a node connection, and use mocktime to simulate a peer
// which never sends any headers messages.  PeerLogic should
// decide to evict that outbound peer, after the appropriate timeouts.
// Note that we protect 4 outbound nodes from being subject to
// this logic; this test takes advantage of that protection only
// being applied to nodes which send headers with sufficient
// work.
BOOST_AUTO_TEST_CASE(outbound_slow_chain_eviction)
{
    auto connman = MakeUnique<CConnman>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), nullptr, scheduler, false);

    // Mock an outbound peer
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNode dummyNode1(id++, ServiceFlags(NODE_NETWORK|NODE_WITNESS), 0, INVALID_SOCKET, addr1, 0, 0, CAddress(), "", /*fInboundIn=*/ false);
    dummyNode1.SetSendVersion(PROTOCOL_VERSION);

    peerLogic->InitializeNode(&dummyNode1);
    dummyNode1.nVersion = 1;
    dummyNode1.fSuccessfullyConnected = true;

    // This test requires that we have a chain with non-zero work.
    {
        LOCK(cs_main);
        BOOST_CHECK(::ChainActive().Tip() != nullptr);
        BOOST_CHECK(::ChainActive().Tip()->nChainWork > 0);
    }

    // Test starts here
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1)); // should result in getheaders
    }
    {
        LOCK2(cs_main, dummyNode1.cs_vSend);
        BOOST_CHECK(dummyNode1.vSendMsg.size() > 0);
        dummyNode1.vSendMsg.clear();
    }

    int64_t nStartTime = GetTime();
    // Wait 21 minutes
    SetMockTime(nStartTime+21*60);
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1)); // should result in getheaders
    }
    {
        LOCK2(cs_main, dummyNode1.cs_vSend);
        BOOST_CHECK(dummyNode1.vSendMsg.size() > 0);
    }
    // Wait 3 more minutes
    SetMockTime(nStartTime+24*60);
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1)); // should result in disconnect
    }
    BOOST_CHECK(dummyNode1.fDisconnect == true);
    SetMockTime(0);

    bool dummy;
    peerLogic->FinalizeNode(dummyNode1.GetId(), dummy);
}

static void AddRandomOutboundPeer(std::vector<CNode *> &vNodes, PeerLogicValidation &peerLogic, CConnmanTest* connman)
{
    CAddress addr(ip(g_insecure_rand_ctx.randbits(32)), NODE_NONE);
    vNodes.emplace_back(new CNode(id++, ServiceFlags(NODE_NETWORK|NODE_WITNESS), 0, INVALID_SOCKET, addr, 0, 0, CAddress(), "", /*fInboundIn=*/ false));
    CNode &node = *vNodes.back();
    node.SetSendVersion(PROTOCOL_VERSION);

    peerLogic.InitializeNode(&node);
    node.nVersion = 1;
    node.fSuccessfullyConnected = true;

    connman->AddNode(node);
}

BOOST_AUTO_TEST_CASE(stale_tip_peer_management)
{
    auto connman = MakeUnique<CConnmanTest>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), nullptr, scheduler, false);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    constexpr int max_outbound_full_relay = 8;
    CConnman::Options options;
    options.nMaxConnections = 125;
    options.m_max_outbound_full_relay = max_outbound_full_relay;
    options.nMaxFeeler = 1;

    connman->Init(options);
    std::vector<CNode *> vNodes;

    // Mock some outbound peers
    for (int i=0; i<max_outbound_full_relay; ++i) {
        AddRandomOutboundPeer(vNodes, *peerLogic, connman.get());
    }

    peerLogic->CheckForStaleTipAndEvictPeers(consensusParams);

    // No nodes should be marked for disconnection while we have no extra peers
    for (const CNode *node : vNodes) {
        BOOST_CHECK(node->fDisconnect == false);
    }

    SetMockTime(GetTime() + 3*consensusParams.nPowTargetSpacing + 1);

    // Now tip should definitely be stale, and we should look for an extra
    // outbound peer
    peerLogic->CheckForStaleTipAndEvictPeers(consensusParams);
    BOOST_CHECK(connman->GetTryNewOutboundPeer());

    // Still no peers should be marked for disconnection
    for (const CNode *node : vNodes) {
        BOOST_CHECK(node->fDisconnect == false);
    }

    // If we add one more peer, something should get marked for eviction
    // on the next check (since we're mocking the time to be in the future, the
    // required time connected check should be satisfied).
    AddRandomOutboundPeer(vNodes, *peerLogic, connman.get());

    peerLogic->CheckForStaleTipAndEvictPeers(consensusParams);
    for (int i=0; i<max_outbound_full_relay; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    // Last added node should get marked for eviction
    BOOST_CHECK(vNodes.back()->fDisconnect == true);

    vNodes.back()->fDisconnect = false;

    // Update the last announced block time for the last
    // peer, and check that the next newest node gets evicted.
    UpdateLastBlockAnnounceTime(vNodes.back()->GetId(), GetTime());

    peerLogic->CheckForStaleTipAndEvictPeers(consensusParams);
    for (int i=0; i<max_outbound_full_relay-1; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes[max_outbound_full_relay-1]->fDisconnect == true);
    BOOST_CHECK(vNodes.back()->fDisconnect == false);

    bool dummy;
    for (const CNode *node : vNodes) {
        peerLogic->FinalizeNode(node->GetId(), dummy);
    }

    connman->ClearNodes();
}

BOOST_AUTO_TEST_CASE(DoS_banning)
{
    auto banman = MakeUnique<BanMan>(GetDataDir() / "banlist.dat", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    auto connman = MakeUnique<CConnman>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), banman.get(), scheduler, false);

    banman->ClearBanned();
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNode dummyNode1(id++, NODE_NETWORK, 0, INVALID_SOCKET, addr1, 0, 0, CAddress(), "", true);
    dummyNode1.SetSendVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(&dummyNode1);
    dummyNode1.nVersion = 1;
    dummyNode1.fSuccessfullyConnected = true;
    {
        LOCK(cs_main);
        Misbehaving(dummyNode1.GetId(), 100); // Should get banned
    }
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1));
    }
    BOOST_CHECK(banman->IsBanned(addr1));
    BOOST_CHECK(!banman->IsBanned(ip(0xa0b0c001|0x0000ff00))); // Different IP, not banned

    CAddress addr2(ip(0xa0b0c002), NODE_NONE);
    CNode dummyNode2(id++, NODE_NETWORK, 0, INVALID_SOCKET, addr2, 1, 1, CAddress(), "", true);
    dummyNode2.SetSendVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(&dummyNode2);
    dummyNode2.nVersion = 1;
    dummyNode2.fSuccessfullyConnected = true;
    {
        LOCK(cs_main);
        Misbehaving(dummyNode2.GetId(), 50);
    }
    {
        LOCK2(cs_main, dummyNode2.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode2));
    }
    BOOST_CHECK(!banman->IsBanned(addr2)); // 2 not banned yet...
    BOOST_CHECK(banman->IsBanned(addr1));  // ... but 1 still should be
    {
        LOCK(cs_main);
        Misbehaving(dummyNode2.GetId(), 50);
    }
    {
        LOCK2(cs_main, dummyNode2.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode2));
    }
    BOOST_CHECK(banman->IsBanned(addr2));

    bool dummy;
    peerLogic->FinalizeNode(dummyNode1.GetId(), dummy);
    peerLogic->FinalizeNode(dummyNode2.GetId(), dummy);
}

BOOST_AUTO_TEST_CASE(DoS_banscore)
{
    auto banman = MakeUnique<BanMan>(GetDataDir() / "banlist.dat", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    auto connman = MakeUnique<CConnman>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), banman.get(), scheduler, false);

    banman->ClearBanned();
    gArgs.ForceSetArg("-banscore", "111"); // because 11 is my favorite number
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNode dummyNode1(id++, NODE_NETWORK, 0, INVALID_SOCKET, addr1, 3, 1, CAddress(), "", true);
    dummyNode1.SetSendVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(&dummyNode1);
    dummyNode1.nVersion = 1;
    dummyNode1.fSuccessfullyConnected = true;
    {
        LOCK(cs_main);
        Misbehaving(dummyNode1.GetId(), 100);
    }
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1));
    }
    BOOST_CHECK(!banman->IsBanned(addr1));
    {
        LOCK(cs_main);
        Misbehaving(dummyNode1.GetId(), 10);
    }
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1));
    }
    BOOST_CHECK(!banman->IsBanned(addr1));
    {
        LOCK(cs_main);
        Misbehaving(dummyNode1.GetId(), 1);
    }
    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1));
    }
    BOOST_CHECK(banman->IsBanned(addr1));
    gArgs.ForceSetArg("-banscore", std::to_string(DEFAULT_BANSCORE_THRESHOLD));

    bool dummy;
    peerLogic->FinalizeNode(dummyNode1.GetId(), dummy);
}

BOOST_AUTO_TEST_CASE(DoS_bantime)
{
    auto banman = MakeUnique<BanMan>(GetDataDir() / "banlist.dat", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    auto connman = MakeUnique<CConnman>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), banman.get(), scheduler, false);

    banman->ClearBanned();
    int64_t nStartTime = GetTime();
    SetMockTime(nStartTime); // Overrides future calls to GetTime()

    CAddress addr(ip(0xa0b0c001), NODE_NONE);
    CNode dummyNode(id++, NODE_NETWORK, 0, INVALID_SOCKET, addr, 4, 4, CAddress(), "", true);
    dummyNode.SetSendVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(&dummyNode);
    dummyNode.nVersion = 1;
    dummyNode.fSuccessfullyConnected = true;

    {
        LOCK(cs_main);
        Misbehaving(dummyNode.GetId(), 100);
    }
    {
        LOCK2(cs_main, dummyNode.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode));
    }
    BOOST_CHECK(banman->IsBanned(addr));

    SetMockTime(nStartTime+60*60);
    BOOST_CHECK(banman->IsBanned(addr));

    SetMockTime(nStartTime+60*60*24+1);
    BOOST_CHECK(!banman->IsBanned(addr));

    bool dummy;
    peerLogic->FinalizeNode(dummyNode.GetId(), dummy);
}

static CTransactionRef RandomOrphan()
{
    std::map<uint256, COrphanTx>::iterator it;
    LOCK2(cs_main, g_cs_orphans);
    it = mapOrphanTransactions.lower_bound(InsecureRand256());
    if (it == mapOrphanTransactions.end())
        it = mapOrphanTransactions.begin();
    return it->second.tx;
}

BOOST_AUTO_TEST_CASE(DoS_mapOrphans)
{
    CKey key;
    key.MakeNewKey(true);
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKey(key));

    // 50 orphan transactions:
    for (int i = 0; i < 50; i++)
    {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = InsecureRand256();
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

        AddOrphanTx(MakeTransactionRef(tx), i);
    }

    // ... and 50 that depend on other orphans:
    for (int i = 0; i < 50; i++)
    {
        CTransactionRef txPrev = RandomOrphan();

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = txPrev->GetHash();
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL));

        AddOrphanTx(MakeTransactionRef(tx), i);
    }

    // This really-big orphan should be ignored:
    for (int i = 0; i < 10; i++)
    {
        CTransactionRef txPrev = RandomOrphan();

        CMutableTransaction tx;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        tx.vin.resize(2777);
        for (unsigned int j = 0; j < tx.vin.size(); j++)
        {
            tx.vin[j].prevout.n = j;
            tx.vin[j].prevout.hash = txPrev->GetHash();
        }
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL));
        // Re-use same signature for other inputs
        // (they don't have to be valid for this test)
        for (unsigned int j = 1; j < tx.vin.size(); j++)
            tx.vin[j].scriptSig = tx.vin[0].scriptSig;

        BOOST_CHECK(!AddOrphanTx(MakeTransactionRef(tx), i));
    }

    LOCK2(cs_main, g_cs_orphans);
    // Test EraseOrphansFor:
    for (NodeId i = 0; i < 3; i++)
    {
        size_t sizeBefore = mapOrphanTransactions.size();
        EraseOrphansFor(i);
        BOOST_CHECK(mapOrphanTransactions.size() < sizeBefore);
    }

    // Test LimitOrphanTxSize() function:
    LimitOrphanTxSize(40);
    BOOST_CHECK(mapOrphanTransactions.size() <= 40);
    LimitOrphanTxSize(10);
    BOOST_CHECK(mapOrphanTransactions.size() <= 10);
    LimitOrphanTxSize(0);
    BOOST_CHECK(mapOrphanTransactions.empty());
}

BOOST_AUTO_TEST_SUITE_END()
