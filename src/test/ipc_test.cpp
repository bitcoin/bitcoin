// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <mp/proxy-types.h>
#include <test/ipc_test.capnp.h>
#include <test/ipc_test.capnp.proxy.h>
#include <test/ipc_test.h>

#include <future>
#include <kj/common.h>
#include <kj/memory.h>
#include <kj/test.h>

#include <boost/test/unit_test.hpp>

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

    // Test cleanup: disconnect pipe and join thread
    disconnect_client();
    thread.join();
}
