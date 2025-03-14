// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/proxy-io.h>
#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>
#include <mp/test/foo.h>

#include <capnp/capability.h>
#include <cstdio>
#include <future>
#include <functional>
#include <iostream>
#include <memory>
#include <kj/common.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <string>
#include <thread>
#include <utility>

namespace mp {
namespace test {

KJ_TEST("Call FooInterface methods")
{
    std::promise<std::unique_ptr<ProxyClient<messages::FooInterface>>> foo_promise;
    std::function<void()> disconnect_client;
    std::thread thread([&]() {
        EventLoop loop("mptest", [](bool raise, const std::string& log) {
            std::cout << "LOG" << raise << ": " << log << "\n";
        });
        auto pipe = loop.m_io_context.provider->newTwoWayPipe();

        auto connection_client = std::make_unique<Connection>(loop, kj::mv(pipe.ends[0]));
        auto foo_client = std::make_unique<ProxyClient<messages::FooInterface>>(
            connection_client->m_rpc_system->bootstrap(ServerVatId().vat_id).castAs<messages::FooInterface>(),
            connection_client.get(), /* destroy_connection= */ false);
        foo_promise.set_value(std::move(foo_client));
        disconnect_client = [&] { loop.sync([&] { connection_client.reset(); }); };

        auto connection_server = std::make_unique<Connection>(loop, kj::mv(pipe.ends[1]), [&](Connection& connection) {
            auto foo_server = kj::heap<ProxyServer<messages::FooInterface>>(std::make_shared<FooImplementation>(), connection);
            return capnp::Capability::Client(kj::mv(foo_server));
        });
        connection_server->onDisconnect([&] { connection_server.reset(); });
        loop.loop();
    });

    auto foo = foo_promise.get_future().get();
    KJ_EXPECT(foo->add(1, 2) == 3);

    FooStruct in;
    in.name = "name";
    in.setint.insert(2);
    in.setint.insert(1);
    in.vbool.push_back(false);
    in.vbool.push_back(true);
    in.vbool.push_back(false);
    FooStruct out = foo->pass(in);
    KJ_EXPECT(in.name == out.name);
    KJ_EXPECT(in.setint.size() == out.setint.size());
    for (auto init{in.setint.begin()}, outit{out.setint.begin()}; init != in.setint.end() && outit != out.setint.end(); ++init, ++outit) {
        KJ_EXPECT(*init == *outit);
    }
    KJ_EXPECT(in.vbool.size() == out.vbool.size());
    for (size_t i = 0; i < in.vbool.size(); ++i) {
        KJ_EXPECT(in.vbool[i] == out.vbool[i]);
    }

    FooStruct err;
    try {
        foo->raise(in);
    } catch (const FooStruct& e) {
        err = e;
    }
    KJ_EXPECT(in.name == err.name);

    class Callback : public ExtendedCallback
    {
    public:
        Callback(int expect, int ret) : m_expect(expect), m_ret(ret) {}
        int call(int arg) override
        {
            KJ_EXPECT(arg == m_expect);
            return m_ret;
        }
        int callExtended(int arg) override
        {
            KJ_EXPECT(arg == m_expect + 10);
            return m_ret + 10;
        }
        int m_expect, m_ret;
    };

    foo->initThreadMap();
    Callback callback(1, 2);
    KJ_EXPECT(foo->callback(callback, 1) == 2);
    KJ_EXPECT(foo->callbackUnique(std::make_unique<Callback>(3, 4), 3) == 4);
    KJ_EXPECT(foo->callbackShared(std::make_shared<Callback>(5, 6), 5) == 6);
    auto saved = std::make_shared<Callback>(7, 8);
    KJ_EXPECT(saved.use_count() == 1);
    foo->saveCallback(saved);
    KJ_EXPECT(saved.use_count() == 2);
    foo->callbackSaved(7);
    KJ_EXPECT(foo->callbackSaved(7) == 8);
    foo->saveCallback(nullptr);
    KJ_EXPECT(saved.use_count() == 1);
    KJ_EXPECT(foo->callbackExtended(callback, 11) == 12);

    FooCustom custom_in;
    custom_in.v1 = "v1";
    custom_in.v2 = 5;
    FooCustom custom_out = foo->passCustom(custom_in);
    KJ_EXPECT(custom_in.v1 == custom_out.v1);
    KJ_EXPECT(custom_in.v2 == custom_out.v2);

    foo->passEmpty(FooEmpty{});

    FooMessage message1;
    message1.message = "init";
    FooMessage message2{foo->passMessage(message1)};
    KJ_EXPECT(message2.message == "init build read call build read");

    FooMutable mut;
    mut.message = "init";
    foo->passMutable(mut);
    KJ_EXPECT(mut.message == "init build pass call return read");

    disconnect_client();
    thread.join();

    bool destroyed = false;
    foo->m_context.cleanup_fns.emplace_front([&destroyed]{ destroyed = true; });
    foo.reset();
    KJ_EXPECT(destroyed);
}

} // namespace test
} // namespace mp
