// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <any>
#include <atomic>
#include <capnp/capability.h>
#include <capnp/rpc-twoparty.h>
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
#include <kj/function.h>
#include <kj/memory.h>
#include <kj/string.h>
#include <kj/test.h>
#include <list> // IWYU pragma: keep
#include <map>
#include <memory>
#include <mp/config.h>
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
 *
 * Also accepts an analogous server_owns_connection option controlling
 * whether the server Connection is erased automatically when the peer
 * disconnects. False is needed by tests that need to keep the server
 * Connection alive across a disconnect notification, tearing it down
 * explicitly later via server_disconnect() instead.
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

    TestSetup(bool client_owns_connection = true, bool server_owns_connection = true)
        : thread{[&] {
              EventLoop loop("mptest", [](mp::LogMessage log) {
                  // Info logs are not printed by default, but will be shown with `mptest --verbose`
                  KJ_LOG(INFO, log.level, log.message);
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
              });
              auto pipe = loop.m_io_context.provider->newTwoWayPipe();

              auto server_result = ServeStream<messages::FooInterface>(
                  loop, kj::mv(pipe.ends[0]), std::make_shared<FooImplementation>(), server_owns_connection);
              server = server_result.first;
              EventLoop::Connections::iterator server_it = server_result.second;
              server_disconnect = [&] { loop.sync([&] { loop.m_incoming_connections.erase(server_it); }); };
              server_disconnect_later = [&] {
                  assert(std::this_thread::get_id() == loop.m_thread_id);
                  loop.m_task_set->add(kj::evalLater([&] { loop.m_incoming_connections.erase(server_it); }));
              };

              auto client_proxy =
                  ConnectStream<messages::FooInterface>(loop, kj::mv(pipe.ends[1]), client_owns_connection);
              if (!client_owns_connection) {
                  Connection* client_connection = client_proxy->m_context.connection;
                  client_disconnect = [&loop, client_connection] { loop.sync([&] { delete client_connection; }); };
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
        KJ_EXPECT(out.unordered_set_int.contains(elem));
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
    custom_in.v3 = {10, 20, 30};
    FooCustom custom_out = foo->passCustom(custom_in);
    KJ_EXPECT(custom_in.v1 == custom_out.v1);
    KJ_EXPECT(custom_in.v2 == custom_out.v2);
    KJ_EXPECT(custom_in.v3 == custom_out.v3);

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

    // Test returning vector<unique_ptr<interface>> from server. This exercises
    // BuildList with interface element types, which requires non-const iteration
    // so unique_ptr::release() can transfer ownership to the proxy server.
    std::vector<std::unique_ptr<Bar>> bars{foo->listBars(3)};
    KJ_REQUIRE(bars.size() == 3u);
    for (int i = 0; i < 3; ++i) {
        KJ_REQUIRE(bars[i] != nullptr);
        KJ_EXPECT(bars[i]->value() == i);
    }
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

    try {
        foo->add(1, 2);
        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason{e.what()};

        // The disconnect handler may delete the connection before the
        // call is processed or while the call is in flight, both errors are possible.
        KJ_EXPECT(reason == "IPC client method called after disconnect." || reason == "IPC client method call interrupted by disconnect.");
    }
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

KJ_TEST("Calling async IPC method with a remote disconnect while results are built")
{
    // Regression test for bitcoin-core/libmultiprocess#348, a data race
    // reported by ThreadSanitizer where a server worker thread called
    // call_context.getResults() while the event loop thread was tearing down
    // Cap'n Proto connection state (capnp::_::RpcConnectionState::disconnect()
    // overwriting the RpcConnectionState::connection field) after an abrupt
    // remote disconnect.
    //
    // The test makes an async IPC call (callMessageAsync) whose method body
    // just signals the main thread. When the worker thread returns from the
    // method body, it calls call_context.getResults(), reading the connection
    // state, and starts serializing the FooMessage result, where the
    // testing_hook_misc hook set below makes it sleep. Meanwhile the main
    // thread destroys the client connection, and the event loop thread
    // processes the resulting EOF, running RpcConnectionState::disconnect()
    // and overwriting the connection state the worker thread just read, with
    // no synchronization between the two accesses.
    //
    // Two details are essential for ThreadSanitizer to detect the race:
    //
    // - There must be no synchronization between the worker thread's
    //   getResults() call and the event loop thread's disconnect processing.
    //   The worker signals the main thread *before* getResults() and sleeps
    //   (sleeping creates no happens-before edge) across the disconnect, so
    //   the racing accesses are not ordered by any of the test's own
    //   synchronization.
    //
    // - The worker thread's read should come *before* the event loop thread's
    //   write, close in time. In the write-then-read order (e.g. method
    //   returning long after the disconnect), the race exists too, but
    //   ThreadSanitizer usually misses it: right after disconnect() writes the
    //   connection field, the loop thread's teardown destructors re-read it
    //   many times (~ImportClient etc. check connection.is<Connected>() to
    //   decide whether to send messages), evicting the write from the
    //   per-granule shadow history before a late reader comes along.
    //
    // The server Connection object is deliberately kept alive during all this
    // by constructing TestSetup with server_owns_connection=false: destroying
    // it would cancel the in-flight request (Connection::~Connection calls
    // m_canceler.cancel(), setting request_canceled) and the worker would
    // throw InterruptException instead of proceeding into getResults().
    // Keeping it alive matches the window in the original report, where the
    // worker races with capnp's own internal teardown, which runs before any
    // TwoPartyVatNetwork::onDisconnect notification. The connection is
    // destroyed explicitly at the end of the test instead.

    TestSetup setup{/*client_owns_connection=*/false, /*server_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    KJ_EXPECT(foo->add(1, 2) == 3);
    foo->initThreadMap();

    // Signaled by the worker thread when the method body runs, just before it
    // returns and the worker calls getResults() and serializes the results.
    std::promise<void> fn_called;
    setup.server->m_impl->m_fn = [&] { fn_called.set_value(); };

    // Keep the worker thread inside the results-building step, without
    // synchronizing, while the event loop thread processes the disconnect.
    EventLoop& loop = *setup.server->m_context.connection->m_loop;
    loop.testing_hook_misc = [](std::any arg) {
        if (const char* const* tag{std::any_cast<const char*>(&arg)};
            tag && std::string_view{*tag} == "build FooMessage") {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    };

    // Signaled by the worker thread when it is completely done with the
    // request, including serializing the results.
    std::promise<void> request_done;
    loop.testing_hook_async_request_done = [&] { request_done.set_value(); };

    // Make the IPC call from a separate thread so this thread can trigger the
    // client disconnect while the call is executing.
    std::thread caller{[&] {
        EXPECT_EXCEPTION(foo->callMessageAsync(), "IPC client method call interrupted by disconnect.");
    }};

    fn_called.get_future().get();
    setup.client_disconnect();
    caller.join();

    // Wait for the worker thread to finish the request, then tear down the
    // server connection that was deliberately kept alive above, so the event
    // loop is able to exit.
    request_done.get_future().get();
    setup.server_disconnect();
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

KJ_TEST("Thread exiting while its connection is destroyed")
{
    // Regression test for a race between a thread exiting after making IPC
    // calls and its connection being destroyed on the event loop thread.
    // ~ThreadContext on the exiting thread and the SetThread disconnect
    // callback run by ~Connection both remove the thread's map entries for
    // the connection, and previously nothing synchronized them, so both could
    // destroy the same ProxyClient<Thread> object.
    //
    // The testing_hook_thread_client_destroy hook, called at the start of
    // ~ProxyClient<Thread>, blocks the exiting thread inside its first map
    // entry destructor while the main thread destroys the connection. The
    // disconnect callback must leave that entry alone: it resets
    // m_disconnect_cb only when it finds the entry in the map and takes over
    // destroying it, so the entry's m_disconnect_cb must still be set when
    // the exiting thread resumes.
    TestSetup setup{/*client_owns_connection=*/false};
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();
    setup.server->m_impl->m_fn = [] {};
    EventLoop& loop = *foo->m_context.loop;

    std::promise<void> caller_exiting, release_caller;
    bool caller_blocked{false};   // caller thread only
    bool disconnect_cb_set{false}; // caller thread, read after join
    loop.testing_hook_thread_client_destroy = [&](ProxyClient<Thread>* client) {
        // The hook also runs on the event loop thread for entries the
        // disconnect callback destroys. Block only the exiting caller thread,
        // in the first destructor it runs.
        if (std::this_thread::get_id() == loop.m_thread_id || caller_blocked) return;
        caller_blocked = true;
        caller_exiting.set_value();
        release_caller.get_future().get();
        disconnect_cb_set = client->m_disconnect_cb.has_value();
    };

    // Make a call taking an mp.Context argument, which adds callback and
    // request thread entries for the connection to the caller's thread-local
    // ThreadContext maps. They are destroyed when the thread exits.
    std::thread caller{[&] { foo->callFnAsync(); }};
    caller_exiting.get_future().get();
    setup.client_disconnect();
    release_caller.set_value();
    caller.join();
    loop.testing_hook_thread_client_destroy = nullptr;
    KJ_EXPECT(disconnect_cb_set);
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

KJ_TEST("Waiting for in-flight server call to finish after disconnect")
{
    // Regression test for bitcoin/bitcoin#35845. Disconnecting a connection
    // cancels the KJ promise of an in-flight call, but a C++ server method
    // body already dispatched to a worker thread runs to completion. Verify
    // that Connection::waitDrained() blocks until such a body finishes and its
    // server object is destroyed, so shutdown code can wait for a disconnected
    // connection to become quiescent before freeing state the body accesses.

    std::promise<void> body_started, release_body;
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();

    // A server call body that signals when it starts and then blocks until the
    // test releases it, so the in-flight state can be observed
    // deterministically.
    setup.server->m_impl->m_fn = [&] {
        body_started.set_value();
        release_body.get_future().get();
    };

    // Grab the server Connection object on the event loop thread before
    // disconnecting. It stays valid until server_disconnect() destroys it
    // below.
    Connection* connection{nullptr};
    foo->m_context.loop->sync([&] { connection = setup.server->m_context.connection; });

    // Invoke the async method on a separate thread so its body blocks there
    // while this thread makes assertions. callFnAsync() takes an mp.Context,
    // so its body runs on a worker thread via ProxyServer<Thread>::post().
    std::thread call_thread([&] {
        EXPECT_EXCEPTION(foo->callFnAsync(), "IPC client method call interrupted by disconnect.");
    });
    body_started.get_future().get();

    // The FooInterface server object is the connection's only counted server
    // object, and its call body is executing.
    KJ_EXPECT(connection->tracker()->pendingServerObjects() == 1);

    // Disconnect. This cancels the call's promise (the client above sees the
    // disconnect error), but the body is still blocked on the worker thread,
    // so its server object must still be alive.
    foo->m_context.loop->sync([&] { connection->disconnect(); });
    KJ_EXPECT(connection->tracker()->pendingServerObjects() == 1);

    // A drain must block while the body runs and return only once it
    // finishes, which is what Ipc::disconnectIncoming relies on during
    // shutdown.
    std::promise<void> drain_waiting;
    connection->tracker()->testing_hook_wait = [&] { drain_waiting.set_value(); };
    std::atomic<bool> drained{false};
    std::thread drain_thread([&] {
        connection->tracker()->waitDrained();
        drained = true;
    });

    // Wait until waitDrained() has observed the live server object and is
    // about to block, then verify it does not return while the body is blocked.
    drain_waiting.get_future().get();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    KJ_EXPECT(!drained);

    // Let the body finish; the drain should now complete.
    release_body.set_value();
    drain_thread.join();
    KJ_EXPECT(drained);
    KJ_EXPECT(connection->tracker()->pendingServerObjects() == 0);
    call_thread.join();

    // Destroy the drained connection. (~Connection notices disconnect() has
    // already run and does not tear things down twice.)
    setup.server_disconnect();
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
    ThreadContext& tc{CurrentThread()};
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

    ThreadContext& tc{CurrentThread()};
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

#ifdef HAVE_PTHREAD_GETNAME_NP
KJ_TEST("Worker thread has OS thread name")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();

    std::promise<std::string> thread_name;
    setup.server->m_impl->m_fn = [&] { thread_name.set_value(ThreadName("")); };
    foo->callFnAsync();

    const std::string name{thread_name.get_future().get()};
    KJ_EXPECT(name.find("/capnp-worker-") != std::string::npos, name);
}

KJ_TEST("Pool thread has OS thread name")
{
    TestSetup setup;
    ProxyClient<messages::FooInterface>* foo = setup.client.get();
    foo->initThreadMap();

    std::promise<std::string> thread_name;
    setup.server->m_impl->m_fn = [&] { thread_name.set_value(ThreadName("")); };

    std::promise<void> pool_ready;
    foo->m_context.loop->sync([&] {
        auto pool_req = foo->m_context.connection->m_thread_map.makePoolRequest();
        pool_req.setCount(1);
        foo->m_context.loop->m_task_set->add(
            pool_req.send().then([&](auto&&) { pool_ready.set_value(); }));
    });
    pool_ready.get_future().get();

    std::promise<void> done;
    foo->m_context.loop->sync([&] {
        auto request{foo->m_client.callFnAsyncRequest()};
        foo->m_context.loop->m_task_set->add(
            request.send().then([&](auto&&) { done.set_value(); }));
    });
    // Wait for the reply before returning, so the connection is not torn down
    // while the request is still in flight.
    done.get_future().get();

    const std::string name{thread_name.get_future().get()};
    KJ_EXPECT(name.find("/capnp-pool-0-") != std::string::npos, name);
}

KJ_TEST("Async cleanup thread has OS thread name")
{
    std::promise<std::string> thread_name;
    {
        TestSetup setup;
        // FooInterface has no destroy method, so the server ProxyServer runs
        // its cleanup functions on the async thread when it is destroyed.
        setup.server->m_context.cleanup_fns.emplace_front(
            [&] { thread_name.set_value(ThreadName("")); });
    }
    const std::string name{thread_name.get_future().get()};
    KJ_EXPECT(name.find("/capnp-async-") != std::string::npos, name);
}
#endif // HAVE_PTHREAD_GETNAME_NP

KJ_TEST("onDisconnect handler does not run after the connection is destroyed")
{
    // Regression test for a race condition where an onDisconnect handler
    // could fire even after the connection had already been disconnected
    // locally. Local disconnects are supposed to preempt onDisconnect
    // handlers so they are able to free Connection objects without risking double
    // deletions.
    //
    // To hit the previous race window deterministically without relying on
    // socket timing, this test exploits two ordering facts:
    // m_network.onDisconnect() hands out branches of a forked promise that fire
    // in the order they were added, and kj::evalLater tasks queued during one
    // event loop turn run on the next turn in FIFO order. A "destroyer" branch
    // is registered before the handler under test, so when the peer disconnects
    // the destroyer fires first and queues an evalLater task that destroys the
    // Connection ahead of the handler. The handler records whether it ran after
    // the Connection was destroyed, reading only test-owned flags (never the
    // freed Connection), so a violation is a deterministic wrong result rather
    // than a flaky crash.
    //
    // The Connection must be destroyed from that separate task, not from inside
    // an onDisconnect continuation: destroying it there tears down m_network
    // while a continuation of m_network's promise is still running, and crashes.

    std::atomic<bool> connection_destroyed{false};
    std::atomic<bool> handler_ran{false};
    std::atomic<bool> handler_ran_after_destroy{false};

    std::thread thread{[&] {
        EventLoop loop("mptest", [](mp::LogMessage log) {
            KJ_LOG(INFO, log.level, log.message);
            if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
        });
        auto pipe = loop.m_io_context.provider->newTwoWayPipe();

        // Server-side connection whose onDisconnect handler is under test.
        auto server_conn = std::make_unique<Connection>(
            loop, kj::mv(pipe.ends[0]), [&](Connection& connection) {
                return ::capnp::Capability::Client(kj::heap<ProxyServer<messages::FooInterface>>(
                    std::make_shared<FooImplementation>(), connection));
            });

        // Raw peer end of the pipe. Closing it makes the server connection's
        // m_network.onDisconnect() resolve, i.e. a real remote disconnect.
        kj::Own<kj::AsyncIoStream> peer_end = kj::mv(pipe.ends[1]);

        // Destroyer branch, registered BEFORE the handler under test so it
        // fires first and its destroy task E_d is queued before the buggy
        // bounced handler task E_f. It schedules the destroy on a *separate*
        // task, so the connection is not torn down from inside an onDisconnect
        // continuation.
        loop.m_task_set->add(server_conn->m_network->onDisconnect().then([&] {
            loop.m_task_set->add(kj::evalLater([&] {
                server_conn.reset();
                connection_destroyed = true;
            }));
        }));

        // Handler under test. It intentionally does not dereference the
        // Connection; it only records whether it ran after the connection was
        // destroyed, so the result is deterministic.
        server_conn->onDisconnect([&] {
            handler_ran = true;
            handler_ran_after_destroy = connection_destroyed.load();
        });

        // Trigger the remote disconnect once the loop is running.
        loop.m_task_set->add(kj::evalLater([&] { peer_end = nullptr; }));

        loop.loop();
    }};
    thread.join();

    // The handler must have run (both versions schedule it), but it must never
    // run after the connection was destroyed.
    KJ_EXPECT(handler_ran);
    KJ_EXPECT(!handler_ran_after_destroy);
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
