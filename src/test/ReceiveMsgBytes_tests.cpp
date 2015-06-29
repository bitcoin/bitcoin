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
#include "timedata.h"
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
        BOOST_CHECK_EQUAL(testNode.vRecvMsg.size(),1UL);
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
        BOOST_CHECK_EQUAL(testNode.vRecvMsg.size(),1UL);
        CNetMessage& msg = testNode.vRecvMsg[0];
        BOOST_CHECK(msg.complete());
        BOOST_CHECK_EQUAL(msg.hdr.GetCommand(), "ping");
        uint64_t nonce;
        msg.vRecv >> nonce;
        BOOST_CHECK_EQUAL(nonce, (uint64_t)11);
   }
}

BOOST_AUTO_TEST_CASE(TooLargeBlock)
{
    // Random real block (000000000000dab0130bbcc991d3d7ae6b81aa6f50a798888dfe62337458dc45)
    // With one tx
    CBlock block;
    CDataStream stream(ParseHex("0100000079cda856b143d9db2c1caff01d1aecc8630d30625d10e8b4b8b0000000000000b50cc069d6a3e33e3ff84a5c41d9d3febe7c770fdcc96b2c3ff60abe184f196367291b4d4c86041b8fa45d630101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff08044c86041b020a02ffffffff0100f2052a01000000434104ecd3229b0571c3be876feaac0442a9f13c5a572742927af1dc623353ecf8c202225f64868137a18cdd85cbbb4c74fbccfd4f49639cf1bdc94a5672bb15ad5d4cac00000000"), SER_NETWORK, PROTOCOL_VERSION);
    stream >> block;

    CNode testNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));
    testNode.nVersion = 1;

    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << CMessageHeader(Params().MessageStart(), "block", 0);
    size_t headerLen = s.size();
    s << block;

    // Test: too large
    size_t maxBlockSize = Params().GetConsensus().MaxBlockSize(GetAdjustedTime(), sizeForkTime.load());
    s.resize(maxBlockSize+headerLen+1);
    CNetMessage::FinalizeHeader(s);

    BOOST_CHECK(!testNode.ReceiveMsgBytes(&s[0], s.size()));

    testNode.vRecvMsg.clear();

    // Test: exactly at max:
    s.resize(maxBlockSize+headerLen);
    CNetMessage::FinalizeHeader(s);

    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));
}

BOOST_AUTO_TEST_CASE(TooLargeVerack)
{
    CNode testNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));
    testNode.nVersion = 1;

    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << CMessageHeader(Params().MessageStart(), "verack", 0);
    size_t headerLen = s.size();

    CNetMessage::FinalizeHeader(s);
    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));

    // verack is zero-length, so even one byte bigger is too big:
    s.resize(headerLen+1);
    CNetMessage::FinalizeHeader(s);
    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));
    CNodeStateStats stats;
    GetNodeStateStats(testNode.GetId(), stats);
    BOOST_CHECK(stats.nMisbehavior > 0);
}

BOOST_AUTO_TEST_CASE(TooLargePing)
{
    CNode testNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));
    testNode.nVersion = 1;

    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << CMessageHeader(Params().MessageStart(), "ping", 0);
    s << (uint64_t)11; // 8-byte nonce

    CNetMessage::FinalizeHeader(s);
    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));

    // Add another nonce, sanity check should fail
    s << (uint64_t)11; // 8-byte nonce
    CNetMessage::FinalizeHeader(s);
    BOOST_CHECK(testNode.ReceiveMsgBytes(&s[0], s.size()));
    CNodeStateStats stats;
    GetNodeStateStats(testNode.GetId(), stats);
    BOOST_CHECK(stats.nMisbehavior > 0);
}

BOOST_AUTO_TEST_SUITE_END()
