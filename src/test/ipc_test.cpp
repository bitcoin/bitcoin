// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <mp/proxy-types.h>
#include <test/ipc_test.capnp.h>
#include <test/ipc_test.capnp.proxy.h>
#include <test/ipc_test.h>
#include <validation.h>

#include <future>
#include <kj/common.h>
#include <kj/memory.h>
#include <kj/test.h>

#include <boost/test/unit_test.hpp>

using node::CBlockTemplate;

//! Unit test that tests execution of IPC calls without actually creating a
//! separate process. This test is primarily intended to verify behavior of type
//! conversion code that converts C++ objects to Cap'n Proto messages and vice
//! versa.
//!
//! The test creates a thread which creates a FooImplementation object (defined
//! in ipc_test.h) and a two-way pipe accepting IPC requests which call methods
//! on the object through FooInterface (defined in ipc_test.capnp).
void IpcTest()
{
    // Setup: create FooImplemention object and listen for FooInterface requests
    std::promise<std::unique_ptr<mp::ProxyClient<gen::FooInterface>>> foo_promise;
    std::function<void()> disconnect_client;
    std::thread thread([&]() {
        mp::EventLoop loop("IpcTest", [](bool raise, const std::string& log) { LogPrintf("LOG%i: %s\n", raise, log); });
        auto pipe = loop.m_io_context.provider->newTwoWayPipe();

        auto connection_client = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[0]));
        auto foo_client = std::make_unique<mp::ProxyClient<gen::FooInterface>>(
            connection_client->m_rpc_system.bootstrap(mp::ServerVatId().vat_id).castAs<gen::FooInterface>(),
            connection_client.get(), /* destroy_connection= */ false);
        foo_promise.set_value(std::move(foo_client));
        disconnect_client = [&] { loop.sync([&] { connection_client.reset(); }); };

        auto connection_server = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[1]), [&](mp::Connection& connection) {
            auto foo_server = kj::heap<mp::ProxyServer<gen::FooInterface>>(std::make_shared<FooImplementation>(), connection);
            return capnp::Capability::Client(kj::mv(foo_server));
        });
        connection_server->onDisconnect([&] { connection_server.reset(); });
        loop.loop();
    });
    std::unique_ptr<mp::ProxyClient<gen::FooInterface>> foo{foo_promise.get_future().get()};

    // Test: make sure arguments were sent and return value is received
    BOOST_CHECK_EQUAL(foo->add(1, 2), 3);

    COutPoint txout1{Txid::FromUint256(uint256{100}), 200};
    COutPoint txout2{foo->passOutPoint(txout1)};
    BOOST_CHECK(txout1 == txout2);

    UniValue uni1{UniValue::VOBJ};
    uni1.pushKV("i", 1);
    uni1.pushKV("s", "two");
    UniValue uni2{foo->passUniValue(uni1)};
    BOOST_CHECK_EQUAL(uni1.write(), uni2.write());

    CMutableTransaction mtx;
    mtx.version = 2;
    mtx.nLockTime = 3;
    mtx.vin.emplace_back(txout1);
    mtx.vout.emplace_back(COIN, CScript());
    CTransactionRef tx1{MakeTransactionRef(mtx)};
    CTransactionRef tx2{foo->passTransaction(tx1)};
    BOOST_CHECK(*Assert(tx1) == *Assert(tx2));

    BlockValidationState bs1;
    bs1.Invalid(BlockValidationResult::BLOCK_CHECKPOINT, "reject reason", "debug message");
    BlockValidationState bs2{foo->passBlockState(bs1)};
    BOOST_CHECK_EQUAL(bs1.IsValid(), bs2.IsValid());
    BOOST_CHECK_EQUAL(bs1.IsError(), bs2.IsError());
    BOOST_CHECK_EQUAL(bs1.IsInvalid(), bs2.IsInvalid());
    BOOST_CHECK_EQUAL(static_cast<int>(bs1.GetResult()), static_cast<int>(bs2.GetResult()));
    BOOST_CHECK_EQUAL(bs1.GetRejectReason(), bs2.GetRejectReason());
    BOOST_CHECK_EQUAL(bs1.GetDebugMessage(), bs2.GetDebugMessage());

    BlockValidationState bs3;
    BlockValidationState bs4{foo->passBlockState(bs3)};
    BOOST_CHECK_EQUAL(bs3.IsValid(), bs4.IsValid());
    BOOST_CHECK_EQUAL(bs3.IsError(), bs4.IsError());
    BOOST_CHECK_EQUAL(bs3.IsInvalid(), bs4.IsInvalid());
    BOOST_CHECK_EQUAL(static_cast<int>(bs3.GetResult()), static_cast<int>(bs4.GetResult()));
    BOOST_CHECK_EQUAL(bs3.GetRejectReason(), bs4.GetRejectReason());
    BOOST_CHECK_EQUAL(bs3.GetDebugMessage(), bs4.GetDebugMessage());

    std::vector<char> vec1{'H', 'e', 'l', 'l', 'o'};
    std::vector<char> vec2{foo->passVectorChar(vec1)};
    BOOST_CHECK_EQUAL(std::string_view(vec1.begin(), vec1.end()), std::string_view(vec2.begin(), vec2.end()));

    CBlockTemplate temp1;
    temp1.block.nTime = 5;
    CBlockTemplate temp2{foo->passBlockTemplate(temp1)};
    BOOST_CHECK_EQUAL(temp1.block.nTime, temp2.block.nTime);

    // Test cleanup: disconnect pipe and join thread
    disconnect_client();
    thread.join();
}
