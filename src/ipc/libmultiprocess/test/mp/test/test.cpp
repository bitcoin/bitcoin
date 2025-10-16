// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <atomic>
#include <capnp/capability.h>
#include <capnp/rpc.h>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <future>
#include <kj/async.h>
#include <kj/async-io.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/exception.h>
#include <kj/memory.h>
#include <kj/string.h>
#include <kj/test.h>
#include <memory>
#include <mp/proxy.h>
#include <mp/proxy.capnp.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace mp {
namespace test {

/**
 * Test setup class creating a two way connection between a
 * ProxyServer<FooInterface> object and a ProxyClient<FooInterface>.
 *
 * Provides client_disconnect and server_disconnect lambdas that can be used to
 * trigger disconnects and test handling of broken and closed connections.
 *
 * Accepts a client_owns_connection option to test different ProxyClient
 * destroy_connection values and control whether destroying the ProxyClient
 * object destroys the client Connection object. Normally it makes sense for
 * this to be true to simplify shutdown and avoid needing to call
 * client_disconnect manually, but false allows testing more ProxyClient
 * behavior and the "IPC client method called after disconnect" code path.
 */
class TestSetup
{
public:
    std::function<void()> server_disconnect;
    std::function<void()> client_disconnect;
    std::promise<std::unique_ptr<ProxyClient<messages::FooInterface>>> client_promise;
    std::unique_ptr<ProxyClient<messages::FooInterface>> client;
    ProxyServer<messages::FooInterface>* server{nullptr};
    //! Thread variable should be after other struct members so the thread does
    //! not start until the other members are initialized.
    std::thread thread;

    TestSetup(bool client_owns_connection = true)
        : thread{[&] {
              EventLoop loop("mptest", [](mp::LogMessage log) {
                  // Info logs are not printed by default, but will be shown with `mptest --verbose`
                  KJ_LOG(INFO, log.level, log.message);
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
              });
              auto pipe = loop.m_io_context.provider->newTwoWayPipe();

              auto server_connection =
                  std::make_unique<Connection>(loop, kj::mv(pipe.ends[0]), [&](Connection& connection) {
                      auto server_proxy = kj::heap<ProxyServer<messages::FooInterface>>(
                          std::make_shared<FooImplementation>(), connection);
                      server = server_proxy;
                      return capnp::Capability::Client(kj::mv(server_proxy));
                  });
              server_disconnect = [&] { loop.sync([&] { server_connection.reset(); }); };
              // Set handler to destroy the server when the client disconnects. This
              // is ignored if server_disconnect() is called instead.
              server_connection->onDisconnect([&] { server_connection.reset(); });

              auto client_connection = std::make_unique<Connection>(loop, kj::mv(pipe.ends[1]));
              auto client_proxy = std::make_unique<ProxyClient<messages::FooInterface>>(
                  client_connection->m_rpc_system->bootstrap(ServerVatId().vat_id).castAs<messages::FooInterface>(),
                  client_connection.get(), /* destroy_connection= */ client_owns_connection);
              if (client_owns_connection) {
                  client_connection.release();
              } else {
                  client_disconnect = [&] { loop.sync([&] { client_connection.reset(); }); };
              }

              client_promise.set_value(std::move(client_proxy));
              loop.loop();
          }}
    {
        client = client_promise.get_future().get();
    }

    ~TestSetup()
    {
        // Test that client cleanup_fns are executed.
        bool destroyed = false;
        client->m_context.cleanup_fns.emplace_front([&destroyed] { destroyed = true; });
        client.reset();
        KJ_EXPECT(destroyed);

        thread.join();
    }
};

KJ_TEST("Call FooInterface methods")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();

    KJ_EXPECT(foo->add(1, 2) == 3);
    int ret;
    foo->addOut(3, 4, ret);
    KJ_EXPECT(ret == 7);
    foo->addInOut(3, ret);
    KJ_EXPECT(ret == 10);

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

    KJ_EXPECT(foo->passFn([]{ return 10; }) == 10);
}

KJ_TEST("Call IPC method after client connection is closed")
{
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);
    setup.client_disconnect();

    bool disconnected{false};
    try {
        foo->add(1, 2);
    } catch (const std::runtime_error& e) {
        KJ_EXPECT(std::string_view{e.what()} == "IPC client method called after disconnect.");
        disconnected = true;
    }
    KJ_EXPECT(disconnected);
}

KJ_TEST("Calling IPC method after server connection is closed")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);
    setup.server_disconnect();

    bool disconnected{false};
    try {
        foo->add(1, 2);
    } catch (const std::runtime_error& e) {
        KJ_EXPECT(std::string_view{e.what()} == "IPC client method call interrupted by disconnect.");
        disconnected = true;
    }
    KJ_EXPECT(disconnected);
}

KJ_TEST("Calling IPC method and disconnecting during the call")
{
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);

    // Set m_fn to initiate client disconnect when server is in the middle of
    // handling the callFn call to make sure this case is handled cleanly.
    setup.server->m_impl->m_fn = setup.client_disconnect;

    bool disconnected{false};
    try {
        foo->callFn();
    } catch (const std::runtime_error& e) {
        KJ_EXPECT(std::string_view{e.what()} == "IPC client method call interrupted by disconnect.");
        disconnected = true;
    }
    KJ_EXPECT(disconnected);
}

KJ_TEST("Calling IPC method, disconnecting and blocking during the call")
{
    // This test is similar to last test, except that instead of letting the IPC
    // call return immediately after triggering a disconnect, make it disconnect
    // & wait so server is forced to deal with having a disconnection and call
    // in flight at the same time.
    //
    // Test uses callFnAsync() instead of callFn() to implement this. Both of
    // these methods have the same implementation, but the callFnAsync() capnp
    // method declaration takes an mp.Context argument so the method executes on
    // an asynchronous thread instead of executing in the event loop thread, so
    // it is able to block without deadlocking the event lock thread.
    //
    // This test adds important coverage because it causes the server Connection
    // object to be destroyed before ProxyServer object, which is not a
    // condition that usually happens because the m_rpc_system.reset() call in
    // the ~Connection destructor usually would immediately free all remaining
    // ProxyServer objects associated with the connection. Having an in-progress
    // RPC call requires keeping the ProxyServer longer.

    std::promise<void> signal;
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);

    foo->initThreadMap();
    setup.server->m_impl->m_fn = [&] {
        EventLoopRef loop{*setup.server->m_context.loop};
        setup.client_disconnect();
        signal.get_future().get();
    };

    bool disconnected{false};
    try {
        foo->callFnAsync();
    } catch (const std::runtime_error& e) {
        KJ_EXPECT(std::string_view{e.what()} == "IPC client method call interrupted by disconnect.");
        disconnected = true;
    }
    KJ_EXPECT(disconnected);

    // Now that the disconnect has been detected, set signal allowing the
    // callFnAsync() IPC call to return. Since signalling may not wake up the
    // thread right away, it is important for the signal variable to be declared
    // *before* the TestSetup variable so is not destroyed while
    // signal.get_future().get() is called.
    signal.set_value();
}

KJ_TEST("Make simultaneous IPC calls to trigger 'thread busy' error")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    std::promise<void> signal;

    foo->initThreadMap();
    // Use callFnAsync() to get the client to set up the request_thread
    // that will be used for the test.
    setup.server->m_impl->m_fn = [&] {};
    foo->callFnAsync();
    ThreadContext& tc{g_thread_context};
    Thread::Client *callback_thread, *request_thread;
    foo->m_context.loop->sync([&] {
        Lock lock(tc.waiter->m_mutex);
        callback_thread = &tc.callback_threads.at(foo->m_context.connection)->m_client;
        request_thread = &tc.request_threads.at(foo->m_context.connection)->m_client;
    });

    setup.server->m_impl->m_fn = [&] {
        try
        {
            signal.get_future().get();
        }
        catch (const std::future_error& e)
        {
            KJ_EXPECT(e.code() == std::make_error_code(std::future_errc::future_already_retrieved));
        }
    };

    auto client{foo->m_client};
    bool caught_thread_busy = false;
    // NOTE: '3' was chosen because it was the lowest number
    // of simultaneous calls required to reliably catch a "thread busy" error
    std::atomic<size_t> running{3};
    foo->m_context.loop->sync([&]
    {
        for (size_t i = 0; i < running; i++)
        {
            auto request{client.callFnAsyncRequest()};
            auto context{request.initContext()};
            context.setCallbackThread(*callback_thread);
            context.setThread(*request_thread);
            foo->m_context.loop->m_task_set->add(request.send().then(
                [&](auto&& results) {
                    running -= 1;
                    tc.waiter->m_cv.notify_all();
                },
                [&](kj::Exception&& e) {
                    KJ_EXPECT(std::string_view{e.getDescription().cStr()} ==
                        "remote exception: std::exception: thread busy");
                    caught_thread_busy = true;
                    running -= 1;
                    signal.set_value();
                    tc.waiter->m_cv.notify_all();
                }
            ));
        }
    });
    {
        Lock lock(tc.waiter->m_mutex);
        tc.waiter->wait(lock, [&running] { return running == 0; });
    }
    KJ_EXPECT(caught_thread_busy);
}

} // namespace test
} // namespace mp
