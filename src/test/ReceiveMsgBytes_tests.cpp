// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for CNode::ReceiveMsgBytes
//


#include "main.h"
#include "net.h"
#include "pow.h"
#include "serialize.h"
#include "util.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ReceiveMsgBytes_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(FullMessages)
{
    CNode testNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));
    testNode.nVersion = 1;

    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << CMessageHeader(Params().MessageStart(), "ping", 0);
    s << (uint64_t)11; // ping nonce
    CNetMessage::FinalizeHeader(s);

    LOCK(testNode.cs_vRecvMsg);

    // Receive a full 'ping' message
    {
        BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));
        BOOST_CHECK_EQUAL(testNode.vRecvMsg.size(), 1);
        CNetMessage& msg = testNode.vRecvMsg[0];
        BOOST_CHECK(msg.complete());
        BOOST_CHECK_EQUAL(msg.hdr.GetCommand(), "ping");
        uint64_t nonce;
        msg.vRecv >> nonce;
        BOOST_CHECK_EQUAL(nonce, (uint64_t)11);
    }


    testNode.vRecvMsg.clear();

    // ...receive it one byte at a time:
    {
        for (size_t i = 0; i < s.size(); i++) {
            BOOST_CHECK(testNode.ReceiveMsgBytes(&s[i], 1));
        }
        BOOST_CHECK_EQUAL(testNode.vRecvMsg.size(), 1);
        CNetMessage& msg = testNode.vRecvMsg[0];
        BOOST_CHECK(msg.complete());
        BOOST_CHECK_EQUAL(msg.hdr.GetCommand(), "ping");
        uint64_t nonce;
        msg.vRecv >> nonce;
        BOOST_CHECK_EQUAL(nonce, (uint64_t)11);
    }
}

BOOST_AUTO_TEST_CASE(TooLarge)
{
    CNode testNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));
    testNode.nVersion = 1;

    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << CMessageHeader(Params().MessageStart(), "ping", 0);
    size_t headerLen = s.size();
    s << (uint64_t)11; // ping nonce

    // Test: too large
    s.resize(MAX_PROTOCOL_MESSAGE_LENGTH+headerLen+1);
    CNetMessage::FinalizeHeader(s);

    BOOST_CHECK(!testNode.ReceiveMsgBytes(&s[0], s.size()));

    testNode.vRecvMsg.clear();

    // Test: exactly at max:
    s.resize(MAX_PROTOCOL_MESSAGE_LENGTH+headerLen);
    CNetMessage::FinalizeHeader(s);

    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));
}

BOOST_AUTO_TEST_SUITE_END()
