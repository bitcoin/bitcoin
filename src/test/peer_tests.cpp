// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Unit tests for Peer code

#include "peer.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Peer_tests)

PeerManager pm;

BOOST_AUTO_TEST_CASE(Peer_peermgr)
{
    // make and register a typical peer
    constexpr int NODE_ID = 15;
    const std::string NODE_NAME = "node name";
    Peer::Params params{NODE_ID, NODE_NAME};
    params.fNoBan = false;
    params.fNoDisconnect = false;
    pm.Register(NODE_ID, std::make_shared<Peer>(std::move(params)));

    // get it back
    {
        auto ppeer = pm.Get(NODE_ID);
        BOOST_CHECK(ppeer->GetId() == NODE_ID);
        BOOST_CHECK(ppeer->GetName() == NODE_NAME);
        BOOST_CHECK(ppeer->CanDisconnect() == true);
        BOOST_CHECK(ppeer->CanBan() == true);
    }

    pm.Unregister(NODE_ID);
    assert(!pm.Get(NODE_ID));
}

static void SetConnectedDownloadable(Peer& peer)
{
    std::unique_ptr<Peer::ConnectedInfo> pconn{new Peer::ConnectedInfo};
    pconn->SetCanDownload(true);
    pm.SetConnected(peer, std::move(pconn));
}

BOOST_AUTO_TEST_CASE(Peer_prefer)
{
    // low priority peer
    Peer::Params params1{1, ""};
    params1.preferedness = DownloadPreferredness::BELOW_NORMAL;
    pm.Register(1, std::make_shared<Peer>(std::move(params1)));
    auto pLow = pm.Get(1);
    SetConnectedDownloadable(*pLow);
    BOOST_CHECK(!pm.IsPreferred(*pLow));

    // normal priority
    Peer::Params params2{2, ""};
    pm.Register(2, std::make_shared<Peer>(std::move(params2)));
    auto pNormal1 = pm.Get(2);
    SetConnectedDownloadable(*pNormal1);
    BOOST_CHECK(pm.IsPreferred(*pNormal1));
    Peer::Params params3{3, ""};
    pm.Register(3, std::make_shared<Peer>(std::move(params3)));
    auto pNormal2 = pm.Get(3);
    SetConnectedDownloadable(*pNormal2);
    BOOST_CHECK(pm.IsPreferred(*pNormal2));

    // high priority
    Peer::Params params4{4, ""};
    params4.preferedness = DownloadPreferredness::ABOVE_NORMAL;
    pm.Register(4, std::make_shared<Peer>(std::move(params4)));
    auto pHigh1 = pm.Get(4);
    SetConnectedDownloadable(*pHigh1);
    BOOST_CHECK(pm.IsPreferred(*pHigh1));
    BOOST_CHECK(!pm.IsPreferred(*pNormal1));
    BOOST_CHECK(!pm.IsPreferred(*pNormal2));
    Peer::Params params5{5, ""};
    params5.preferedness = DownloadPreferredness::ABOVE_NORMAL;
    pm.Register(5, std::make_shared<Peer>(std::move(params5)));
    auto pHigh2 = pm.Get(5);
    SetConnectedDownloadable(*pHigh2);
    BOOST_CHECK(pm.IsPreferred(*pHigh2));
    BOOST_CHECK(!pm.IsPreferred(*pNormal1));
    BOOST_CHECK(!pm.IsPreferred(*pNormal2));
}

BOOST_AUTO_TEST_SUITE_END()
