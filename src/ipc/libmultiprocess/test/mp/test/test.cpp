// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <atomic>
#include <capnp/capability.h>
#include <capnp/rpc.h>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <future>
#include <kj/async.h>
#include <kj/async-io.h>
#include <kj/common.h>
#include <kj/exception.h>
#include <kj/debug.h>
#include <kj/memory.h>
#include <kj/string.h>
#include <kj/test.h>
#include <map>
#include <memory>
#include <mp/proxy.h>
#include <mp/proxy.capnp.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <mp/version.h>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

//! Assert that a call throws std::runtime_error with the given message.
#define EXPECT_EXCEPTION(call, message)                                           \
    try {                                                                         \
        call;                                                                     \
        KJ_EXPECT(false);                                                         \
    } catch (const std::runtime_error& e) {                                       \
        KJ_EXPECT(std::string_view{e.what()} == message);                         \
    }

namespace mp {
namespace test {

/** Check version.h header values */
constexpr auto kMP_MAJOR_VERSION{MP_MAJOR_VERSION};
constexpr auto kMP_MINOR_VERSION{MP_MINOR_VERSION};
static_assert(std::is_integral_v<decltype(kMP_MAJOR_VERSION)>, "MP_MAJOR_VERSION must be an integral constant");
static_assert(std::is_integral_v<decltype(kMP_MINOR_VERSION)>, "MP_MINOR_VERSION must be an integral constant");

/**
 * Test setup class creating a two way connection between a
 * ProxyServer<FooInterface> object and a ProxyClient<FooInterface>.
 *
 * Provides disconnection lambdas that can be used to trigger
 * disconnects and test handling of broken and closed connections.
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
    std::function<void()> server_disconnect_later;
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
              server_disconnect_later = [&] {
                  assert(std::this_thread::get_id() == loop.m_thread_id);
                  loop.m_task_set->add(kj::evalLater([&] { server_connection.reset(); }));
              };
              // Set handler to destroy the server when the client disconnects. This
              // is ignored if server_disconnect() is called instead.
              server_connection->onDisconnect([&] { server_connection.reset(); });

              auto client_connection = std::make_unique<Connection>(loop, kj::mv(pipe.ends[1]));
              auto client_proxy = std::make_unique<ProxyClient<messages::FooInterface>>(
                  client_connection->m_rpc_system->bootstrap(ServerVatId().vat_id).castAs<messages::FooInterface>(),
                  client_connection.get(), /* destroy_connection= */ client_owns_connection);
              if (client_owns_connection) {
                  (void)client_connection.release();
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
    in.set_int.insert(2);
    in.set_int.insert(1);
    in.unordered_set_int.insert(2);
    in.unordered_set_int.insert(1);
    in.vector_bool.push_back(false);
    in.vector_bool.push_back(true);
    in.vector_bool.push_back(false);
    in.optional_int = 3;
    in.map_string_int.emplace("a", 1);
    in.map_string_int.emplace("b", 2);
    FooStruct out = foo->pass(in);
    KJ_EXPECT(in.name == out.name);
    KJ_EXPECT(in.set_int.size() == out.set_int.size());
    for (auto init{in.set_int.begin()}, outit{out.set_int.begin()}; init != in.set_int.end() && outit != out.set_int.end(); ++init, ++outit) {
        KJ_EXPECT(*init == *outit);
    }
    KJ_EXPECT(in.unordered_set_int.size() == out.unordered_set_int.size());
    for (const auto& elem : in.unordered_set_int) {
        KJ_EXPECT(out.unordered_set_int.count(elem) == 1);
    }
    KJ_EXPECT(in.vector_bool.size() == out.vector_bool.size());
    for (size_t i = 0; i < in.vector_bool.size(); ++i) {
        KJ_EXPECT(in.vector_bool[i] == out.vector_bool[i]);
    }
    KJ_EXPECT(in.optional_int == out.optional_int);
    KJ_EXPECT(in.map_string_int.size() == out.map_string_int.size());
    for (auto init{in.map_string_int.begin()}, outit{out.map_string_int.begin()}; init != in.map_string_int.end() && outit != out.map_string_int.end(); ++init, ++outit) {
        KJ_EXPECT(init->first == outit->first);
        KJ_EXPECT(init->second == outit->second);
    }

    // Additional checks for std::optional member
    KJ_EXPECT(foo->pass(in).optional_int == 3);
    in.optional_int.reset();
    KJ_EXPECT(!foo->pass(in).optional_int);

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

    FooData empty_data_out = foo->passData(FooData{});
    KJ_EXPECT(empty_data_out.empty());

    FooMessage message1;
    message1.message = "init";
    FooMessage message2{foo->passMessage(message1)};
    KJ_EXPECT(message2.message == "init build read call build read");

    FooMutable mut;
    mut.message = "init";
    foo->passMutable(mut);
    KJ_EXPECT(mut.message == "init build pass call return read");

    KJ_EXPECT(foo->passDouble(1.25) == 1.25);

    KJ_EXPECT(foo->passFn([]{ return 10; }) == 10);

    // Recursive async IPC calls
    KJ_EXPECT(foo->passFn([foo]{
        return foo->passFn([]{ return 1; });
    }) == 1);

    std::vector<FooDataRef> data_in;
    data_in.push_back(std::make_shared<FooData>(FooData{'H', 'i'}));
    data_in.push_back(nullptr);
    std::vector<FooDataRef> data_out{foo->passDataPointers(data_in)};
    KJ_EXPECT(data_out.size() == 2);
    KJ_REQUIRE(data_out[0] != nullptr);
    KJ_EXPECT(*data_out[0] == *data_in[0]);
    KJ_EXPECT(!data_out[1]);
}

KJ_TEST("Call IPC method after client connection is closed")
{
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);
    setup.client_disconnect();

    EXPECT_EXCEPTION(foo->add(1, 2), "IPC client method called after disconnect.");
}

KJ_TEST("Calling IPC method after server connection is closed")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);
    setup.server_disconnect();

    EXPECT_EXCEPTION(foo->add(1, 2), "IPC client method call interrupted by disconnect.");
}

KJ_TEST("Calling IPC method and disconnecting during the call")
{
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);

    // Set m_fn to initiate client disconnect when server is in the middle of
    // handling the callFn call to make sure this case is handled cleanly.
    setup.server->m_impl->m_fn = setup.client_disconnect;

    EXPECT_EXCEPTION(foo->callFn(), "IPC client method call interrupted by disconnect.");
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

    EXPECT_EXCEPTION(foo->callFnAsync(), "IPC client method call interrupted by disconnect.");

    // Now that the disconnect has been detected, set signal allowing the
    // callFnAsync() IPC call to return. Since signalling may not wake up the
    // thread right away, it is important for the signal variable to be declared
    // *before* the TestSetup variable so is not destroyed while
    // signal.get_future().get() is called.
    signal.set_value();
}

KJ_TEST("Worker thread destroyed before it is initialized")
{
    // Regression test for bitcoin/bitcoin#34711, bitcoin/bitcoin#34756 where a
    // worker thread is destroyed before it starts waiting for work.
    //
    // The test uses the `makethread` hook to trigger a disconnect as soon as
    // ProxyServer<ThreadMap>::makeThread is called, so without the bugfix,
    // ProxyServer<Thread>::~ProxyServer would run and destroy the waiter before
    // the worker thread started waiting, causing a SIGSEGV when it did start.
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();
    setup.server->m_impl->m_fn = [] {};

    EventLoop& loop = *setup.server->m_context.connection->m_loop;
    loop.testing_hook_makethread = [&] {
        // Use disconnect_later to queue the disconnect, because the makethread
        // hook is called on the event loop thread. The disconnect should happen
        // as soon as the event loop is idle.
        setup.server_disconnect_later();
    };
    loop.testing_hook_makethread_created = [&] {
        // Sleep to allow event loop to run and process the queued disconnect
        // before the worker thread starts waiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    EXPECT_EXCEPTION(foo->callFnAsync(), "IPC client method call interrupted by disconnect.");
}

KJ_TEST("Calling async IPC method, with server disconnect racing the call")
{
    // Regression test for bitcoin/bitcoin#34777 heap-use-after-free where
    // an async request is canceled before it starts to execute.
    //
    // Use testing_hook_async_request_start to trigger a disconnect from the
    // worker thread as soon as it begins to execute an async request. Without
    // the bugfix, the worker thread would trigger a SIGSEGV after this by
    // calling call_context.getParams().
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();
    setup.server->m_impl->m_fn = [] {};

    EventLoop& loop = *setup.server->m_context.connection->m_loop;
    loop.testing_hook_async_request_start = [&] {
        setup.server_disconnect();
        // Sleep is necessary to let the event loop fully clean up after the
        // disconnect and trigger the SIGSEGV.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    EXPECT_EXCEPTION(foo->callFnAsync(), "IPC client method call interrupted by disconnect.");
}

KJ_TEST("Calling async IPC method, with server disconnect after cleanup")
{
    // Regression test for bitcoin/bitcoin#34782 stack-use-after-return where
    // an async request is canceled after it finishes executing but before the
    // response is sent.
    //
    // Use testing_hook_async_request_done to trigger a disconnect from the
    // worker thread after it executes an async request but before it returns.
    // Without the bugfix, the m_on_cancel callback would be called at this
    // point, accessing the cancel_mutex stack variable that had gone out of
    // scope.
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();
    setup.server->m_impl->m_fn = [] {};

    EventLoop& loop = *setup.server->m_context.connection->m_loop;
    loop.testing_hook_async_request_done = [&] {
        setup.server_disconnect();
    };

    EXPECT_EXCEPTION(foo->callFnAsync(), "IPC client method call interrupted by disconnect.");
}

KJ_TEST("Destroying ProxyClient<> with destroy method after peer disconnect")
{
    // Regression test for bitcoin-core/libmultiprocess#219 where
    // ~ProxyClientBase would call std::terminate if the remote destroy RPC
    // failed during teardown.
    //
    // Save a callback on the server so it holds a ProxyClient<FooCallback>
    // pointing back to this side, then disconnect. When the server is torn
    // down, the ProxyClient<FooCallback> destructor issues a destroy RPC over
    // the now dead connection; without the bugfix the exception escapes the
    // noexcept destructor and aborts the process.

    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();

    class Callback : public FooCallback
    {
    public:
        int call(int arg) override { return arg; }
    };

    foo->saveCallback(std::make_shared<Callback>());
    setup.client_disconnect();
}

KJ_TEST("Make simultaneous IPC calls on single remote thread")
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

    // Call callIntFnAsync 3 times with n=100, 200, 300
    std::atomic<int> expected = 100;

    setup.server->m_impl->m_int_fn = [&](int n) {
        assert(n == expected);
        expected += 100;
        return n;
    };

    auto client{foo->m_client};
    std::atomic<size_t> running{3};
    foo->m_context.loop->sync([&]
    {
        for (size_t i = 0; i < running; i++)
        {
            auto request{client.callIntFnAsyncRequest()};
            auto context{request.initContext()};
            context.setCallbackThread(*callback_thread);
            context.setThread(*request_thread);
            request.setArg(100 * (i+1));
            foo->m_context.loop->m_task_set->add(request.send().then(
                [&running, &tc, i](auto&& results) {
                    assert(results.getResult() == static_cast<int32_t>(100 * (i+1)));
                    running -= 1;
                    Lock lock(tc.waiter->m_mutex);
                    tc.waiter->m_cv.notify_all();
                }));
        }
    });
    {
        Lock lock(tc.waiter->m_mutex);
        tc.waiter->wait(lock, [&running] { return running == 0; });
    }
    KJ_EXPECT(expected == 400);
}

KJ_TEST("Call async IPC method dispatched to pool thread")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();

    // Set up the thread map exchange so the client has the server's ThreadMap,
    // then call makePool to pre-allocate two server threads.
    foo->initThreadMap();
    setup.server->m_impl->m_int_fn = [](int n) { return n * 2; };

    ThreadContext& tc{g_thread_context};
    std::atomic<size_t> running{3};
    std::promise<void> pool_ready;
    foo->m_context.loop->sync([&] {
        auto pool_req = foo->m_context.connection->m_thread_map.makePoolRequest();
        pool_req.setCount(2);
        foo->m_context.loop->m_task_set->add(
            pool_req.send().then([&](auto&&) { pool_ready.set_value(); }));
    });
    pool_ready.get_future().get();

    // Send three callIntFnAsync requests with no context.thread set.
    // The server should dispatch each to a pool thread.
    auto client{foo->m_client};
    foo->m_context.loop->sync([&] {
        for (size_t i = 0; i < running; ++i) {
            auto request{client.callIntFnAsyncRequest()};
            request.initContext(); // context present but thread unset
            request.setArg(static_cast<int32_t>(i + 1));
            foo->m_context.loop->m_task_set->add(request.send().then(
                [&running, &tc, i](auto&& results) {
                    assert(results.getResult() == static_cast<int32_t>((i + 1) * 2));
                    running -= 1;
                    Lock lock(tc.waiter->m_mutex);
                    tc.waiter->m_cv.notify_all();
                }));
        }
    });
    {
        Lock lock(tc.waiter->m_mutex);
        tc.waiter->wait(lock, [&running] { return running == 0; });
    }
}

KJ_TEST("Call async IPC method without thread or pool errors correctly")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    setup.server->m_impl->m_fn = [] {};

    // Send a callFnAsync request with no context.thread and no pool configured.
    // The server should throw the "no thread specified and no pool configured" error.
    std::promise<void> done;
    bool error_thrown{false};
    foo->m_context.loop->sync([&] {
        auto request{foo->m_client.callFnAsyncRequest()};
        request.initContext();
        foo->m_context.loop->m_task_set->add(
            request.send().then(
                [&](auto&&) { done.set_value(); },
                [&](kj::Exception&& e) {
                    error_thrown = true;
                    KJ_EXPECT(std::string_view{e.getDescription().cStr()}.find(
                        "no thread specified and no pool configured") != std::string_view::npos);
                    done.set_value();
                }));
    });
    done.get_future().get();
    KJ_EXPECT(error_thrown);
}

} // namespace test
} // namespace mp
