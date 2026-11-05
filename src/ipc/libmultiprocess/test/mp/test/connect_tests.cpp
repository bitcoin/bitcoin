// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "common.h"
#include "unixlistener.h"
#include <kj/async.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <mp/proxy-io.h>
#include <mp/proxy.h>
#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>
#include <mp/util.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring> // IWYU pragma: keep
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace mp {
namespace test {
namespace {

constexpr auto FAILURE_TIMEOUT = std::chrono::seconds{30};

class TestSetup
{
public:
    EventLoop* m_loop;
    std::optional<EventLoopRef> m_loop_ref;
    //! Thread variable should be after other struct members so the thread does
    //! not start until the other members are initialized.
    std::thread m_loop_thread;

    TestSetup(LogFn log_handler = DefaultLogHandler)
    {
        std::promise<EventLoop*> loop_promise;
        m_loop_thread = std::thread([&, log_handler] {
            EventLoop loop("mptest-connect", log_handler);
            loop_promise.set_value(&loop);
            loop.loop();
        });
        m_loop = loop_promise.get_future().get();
        m_loop_ref.emplace(*m_loop);
    }

    ~TestSetup()
    {
        m_loop_ref.reset();
        m_loop_thread.join();
    }
};

KJ_TEST("ConnectStream connects to a socket serving a valid init interface")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    std::thread server_thread([&]() {
        EventLoop server_loop("mptest-valid-server", DefaultLogHandler);
        std::unique_ptr<FooInit> init = std::make_unique<FooInit>();
        ServeStream<messages::FooInit>(server_loop, MakeStream(server_loop, server_fd), *init);
        server_loop.loop();
    });

    // FooInit has a `construct()` method, so this connects to the server and
    // sends an IPC request that must complete successfully.
    auto init = ConnectStream<messages::FooInit>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

    init.reset();
    server_thread.join();
}

KJ_TEST("ConnectStream throws when the socket is already disconnected")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    KJ_SYSCALL(close(server_fd));

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();

        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
}

KJ_TEST("ConnectStream defers disconnect failure to the first IPC request for interfaces without construct()")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    KJ_SYSCALL(close(server_fd));

    // Without a construct() method no IPC call is made during client
    // creation, so ConnectStream succeeds even though the peer is gone.
    auto foo = ConnectStream<messages::FooInterface>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

    try {
        foo->add(1, 2);
        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();

        // There is a race between the event loop detecting the disconnect
        // and foo->add() being called. If the onDisconnect callback fires
        // first and nulls m_context.connection, the error is "called after
        // disconnect"; if foo->add() submits before the callback fires,
        // the error is "interrupted by disconnect".
        KJ_EXPECT(reason == "IPC client method called after disconnect." ||
                  reason == "IPC client method call interrupted by disconnect.");
    }
}

KJ_TEST("ConnectStream handles a disconnect when no client calls are made")
{
    std::mutex mutex;
    std::condition_variable cv;
    bool warned = false;

    TestSetup setup([&](LogMessage log) {
        if (log.level == Log::Warning && log.message.find("unexpected network disconnect") != std::string::npos) {
            const std::lock_guard<std::mutex> lock(mutex);
            warned = true;
            cv.notify_all();
        }
        DefaultLogHandler(log);
    });
    auto [client_fd, server_fd] = SocketPair();

    KJ_SYSCALL(close(server_fd));

    auto foo = ConnectStream<messages::FooInterface>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

    // The disconnect handler registered by ProxyClientBase should run and
    // delete the connection even when no calls are ever made.
    std::unique_lock<std::mutex> lock(mutex);
    KJ_EXPECT(cv.wait_for(lock, FAILURE_TIMEOUT, [&] { return warned; }));
}

KJ_TEST("ConnectStream throws when the socket disconnects after receiving data")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    std::thread server_thread([&]() {
        char buf[128];

        recv(server_fd, buf, sizeof(buf), 0);
        KJ_SYSCALL(close(server_fd));
    });

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();
        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
    server_thread.join();
}

KJ_TEST("ConnectStream throws when a connection accepted from a listener disconnects after receiving data")
{
    UnixListener listener;
    TestSetup setup;
    int client_fd = listener.MakeConnectedSocket();
    int server_fd = listener.release();

    std::thread server_thread([&]() {
        char buf[128];

        int connection_fd = accept(server_fd, nullptr, nullptr);

        if (connection_fd >= 0) {
            recv(connection_fd, buf, sizeof(buf), 0);
            KJ_SYSCALL(close(connection_fd));
        }
        KJ_SYSCALL(close(server_fd));
    });

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.m_loop, MakeStream(*setup.m_loop, client_fd));

        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();
        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
    server_thread.join();
}

} // namespace
} // namespace test
} // namespace mp
