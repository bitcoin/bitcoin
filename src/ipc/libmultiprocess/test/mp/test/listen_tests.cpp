// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common.h"
#include "unixlistener.h"
#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <chrono>
#include <compare>
#include <condition_variable>
#include <cstring>
#include <future>
#include <functional>
#include <kj/async.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/function.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <memory>
#include <mp/proxy.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <ratio> // IWYU pragma: keep
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>

namespace mp {
namespace test {
namespace {

constexpr auto FAILURE_TIMEOUT = std::chrono::seconds{30};

//! Runs a client EventLoop on its own thread and connects one socket FD to the
//! server. The constructed ProxyClient can be used by the test thread to make
//! calls over that connection.
class ClientSetup
{
public:
    explicit ClientSetup(int fd)
        : thread([this, fd] {
              EventLoop loop("mptest-client", DefaultLogHandler);
              client_promise.set_value(ConnectStream<messages::FooInterface>(loop, MakeStream(loop, fd)));
              loop.loop();
          })
    {
        client = client_promise.get_future().get();
    }

    ~ClientSetup()
    {
        client.reset();
        thread.join();
    }

    std::promise<std::unique_ptr<ProxyClient<messages::FooInterface>>> client_promise;
    std::unique_ptr<ProxyClient<messages::FooInterface>> client;

    //! Thread variable should be after other struct members so the thread does
    //! not start until the other members are initialized.
    std::thread thread;
};

//! Runs a server EventLoop on its own thread, starts ListenConnections() on a
//! UnixListener socket, and records connection/disconnection counts through
//! EventLoop test hooks
class ListenSetup
{
public:
    explicit ListenSetup(std::optional<size_t> max_connections = std::nullopt,
                         mp::LogFn log_handler = DefaultLogHandler)
        : thread([this, max_connections, log_handler] {
              EventLoop loop("mptest-server", log_handler);
              loop.testing_hook_disconnected = [&] {
                  Lock lock(counter_mutex);
                  ++disconnected_count;
                  counter_cv.notify_all();
              };
              loop.testing_hook_connected = [&] {
                  Lock lock(counter_mutex);
                  ++connected_count;
                  counter_cv.notify_all();
              };
              m_loop_ref.emplace(loop);
              FooImplementation foo;
              ListenConnections<messages::FooInterface>(loop, listener.release(), foo, max_connections);
              ready_promise.set_value();
              loop.loop();
          })
    {
        ready_promise.get_future().get();
    }

    ~ListenSetup()
    {
        m_loop_ref.reset();
        thread.join();
    }

    size_t ConnectedCount()
    {
        Lock lock(counter_mutex);
        return connected_count;
    }

    size_t DisconnectedCount()
    {
        Lock lock(counter_mutex);
        return disconnected_count;
    }

    void WaitForConnectedCount(size_t expected_count)
    {
        Lock lock(counter_mutex);
        const auto deadline = std::chrono::steady_clock::now() + FAILURE_TIMEOUT;
        const bool matched = counter_cv.wait_until(lock.m_lock, deadline, [&]() MP_REQUIRES(counter_mutex) {
            return connected_count >= expected_count;
        });
        KJ_REQUIRE(matched);
    }

    void WaitForDisconnectedCount(size_t expected_count)
    {
        Lock lock(counter_mutex);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        const bool matched = counter_cv.wait_until(lock.m_lock, deadline, [&]() MP_REQUIRES(counter_mutex) {
            return disconnected_count >= expected_count;
        });
        KJ_REQUIRE(matched);
    }

    UnixListener listener;
    std::promise<void> ready_promise;
    std::optional<EventLoopRef> m_loop_ref;
    Mutex counter_mutex;
    std::condition_variable counter_cv;
    size_t connected_count MP_GUARDED_BY(counter_mutex) {0};
    size_t disconnected_count MP_GUARDED_BY(counter_mutex) {0};
    //! Thread variable should be after other struct members so the thread does
    //! not start until the other members are initialized.
    std::thread thread;
};

KJ_TEST("ListenConnections accepts incoming connections")
{
    ListenSetup server;
    KJ_EXPECT(server.ConnectedCount() == 0)
    auto client = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());

    server.WaitForConnectedCount(1);
    KJ_EXPECT(client->client->add(1, 2) == 3);
}

KJ_TEST("ListenConnections enforces a local connection limit")
{
    // With max-connections=1, the second socket can connect to the kernel
    // backlog, but ListenConnections should not accept or serve it until the
    // first accepted client disconnects.

    ListenSetup server(/*max_connections=*/1);

    KJ_EXPECT(server.ConnectedCount() == 0)
    auto client1 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    server.WaitForConnectedCount(1);

    KJ_EXPECT(client1->client->add(1, 2) == 3);

    auto client2 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    // Without this sync, ConnectedCount() == 1 might pass even if
    // max_connections was not enforced because the event loop has not accepted
    // client2 yet.
    (**server.m_loop_ref).sync([] {});

    KJ_EXPECT(server.ConnectedCount() == 1);
    KJ_EXPECT(server.DisconnectedCount() == 0);
    client1.reset();
    server.WaitForDisconnectedCount(1);
    server.WaitForConnectedCount(2);

    KJ_EXPECT(client2->client->add(2, 3) == 5);

    KJ_EXPECT(server.DisconnectedCount() == 1);
    client2.reset();
    server.WaitForDisconnectedCount(2);

    KJ_EXPECT(server.DisconnectedCount() == 2);
    auto client3 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    server.WaitForConnectedCount(3);
    KJ_EXPECT(client3->client->add(3, 4) == 7);
}

KJ_TEST("ListenConnections accepts multiple connections")
{
    // With max-connections=2, two clients should be accepted and usable at the
    // same time, while a third waits until one active client disconnects.

    ListenSetup server(/*max_connections=*/2);

    KJ_EXPECT(server.ConnectedCount() == 0);
    auto client1 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    auto client2 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    server.WaitForConnectedCount(2);

    KJ_EXPECT(client1->client->add(1, 2) == 3);
    KJ_EXPECT(client2->client->add(2, 3) == 5);

    auto client3 = std::make_unique<ClientSetup>(server.listener.MakeConnectedSocket());
    // Without this sync, ConnectedCount() == 2 might pass even if
    // max_connections was not enforced because the event loop has not accepted
    // client3 yet.
    (**server.m_loop_ref).sync([] {});

    KJ_EXPECT(server.ConnectedCount() == 2);
    KJ_EXPECT(server.DisconnectedCount() == 0);
    client1.reset();
    server.WaitForDisconnectedCount(1);
    server.WaitForConnectedCount(3);

    KJ_EXPECT(client3->client->add(3, 4) == 7);
}

KJ_TEST("ListenConnections handles a client that disconnects before being accepted")
{
    Mutex mutex;
    bool accept_error = false;
    ListenSetup server(std::nullopt, [&](mp::LogMessage log) {
        // On macOS, accept() can fail if the peer closes the connection before it is accepted.
        // The event loop then reports this as an uncaught task exception. We catch and ignore
        // this specific error here so that the corresponding CI job does not fail.
        //
        // This is a Cap'n Proto bug, fixed by https://github.com/capnproto/capnproto/pull/2748
        if (log.level == mp::Log::Error && log.message.find("Uncaught exception in daemonized task.") != std::string::npos) {
            Lock lock(mutex);
            accept_error = true;
        }
        DefaultLogHandler(log);
    });

    // This is racy, if the close does not happen before accept(),
    // the connection is accepted normally.
    int fd = server.listener.MakeConnectedSocket();
    KJ_SYSCALL(close(fd));

    // Wait for the connection to either be accepted and disconnected, or fail
    // to be accepted and log the error above.
    const auto deadline = std::chrono::steady_clock::now() + FAILURE_TIMEOUT;
    auto accept_error_seen = [&] { Lock lock(mutex); return accept_error; };
    while (!accept_error_seen() && server.DisconnectedCount() < 1 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

#ifndef __APPLE__
    // No error is expected on platforms other than macOS.
    KJ_REQUIRE(!accept_error_seen());
#endif
    KJ_EXPECT(server.DisconnectedCount() == (accept_error_seen() ? 0 : 1));
}

} // namespace
} // namespace test
} // namespace mp
