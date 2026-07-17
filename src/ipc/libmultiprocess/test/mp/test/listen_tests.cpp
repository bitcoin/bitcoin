// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cassert>
#include <filesystem>
#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <future>
#include <functional>
#include <kj/async.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <memory>
#include <mp/proxy.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <ratio> // IWYU pragma: keep
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <variant>

#ifdef WIN32
#include <afunix.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

namespace mp {
namespace test {
namespace {

constexpr auto FAILURE_TIMEOUT = std::chrono::seconds{30};

//! Owns a temporary listening socket used by ListenSetup. Tests call
//! Connect() to create client socket FDs and release() to transfer the listening
//! FD to ListenConnections().
class SocketListener
{
public:
    SocketListener()
    {
        // For now, use AF_UNIX sockets by default, and TCP on Windows to work
        // around limitations with AF_UNIX on in Wine
        // (https://gitlab.winehq.org/wine/wine/-/merge_requests/7650), in
        // particular lack of compatibility between AF_UNIX and AcceptEx. It
        // could make sense later to test TCP connections on Unix, or to avoid
        // AcceptEx in Wine by calling accept in a thread.
#ifdef WIN32
        m_addr.emplace<sockaddr_in>();
#else
        m_addr.emplace<sockaddr_un>();
#endif
        std::visit([this](auto& addr) { Init(addr); }, m_addr);
    }

    ~SocketListener()
    {
        if (m_fd != SocketError) mp::CloseSocket(m_fd);
        if (auto* un = std::get_if<sockaddr_un>(&m_addr)) {
            std::error_code ec;
            if (un->sun_path[0]) std::filesystem::remove(un->sun_path, ec);
            if (!m_dir.empty()) std::filesystem::remove(m_dir, ec);
        }
    }

    SocketId release()
    {
        assert(m_fd != SocketError);
        SocketId fd = m_fd;
        m_fd = SocketError;
        return fd;
    }

    SocketId MakeConnectedSocket() const
    {
        return std::visit([](const auto& addr) { return Connect(addr); }, m_addr);
    }

private:
    void Init(sockaddr_in& addr)
    {
        m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        KJ_REQUIRE(m_fd != SocketError);

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);

        socklen_t len = sizeof(addr);
        KJ_REQUIRE(getsockname(m_fd, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    }

    void Init(sockaddr_un& addr)
    {
        auto base = std::filesystem::temp_directory_path();
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; ; ++attempt) {
            auto path = base / ("mptest-listener-" + std::to_string(now) + std::to_string(attempt));
            if (std::filesystem::create_directory(path)) {
                m_dir = path.string();
                break;
            }
        }
        std::string path = m_dir + "/socket";

        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(m_fd != SocketError);

        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);
    }

    static SocketId Connect(const sockaddr_in& addr)
    {
        SocketId fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        KJ_REQUIRE(fd != SocketError);

        sockaddr_in a = addr;
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0);
        return fd;
    }

    static SocketId Connect(const sockaddr_un& addr)
    {
        SocketId fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(fd != SocketError);

        sockaddr_un a = addr;
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0);
        return fd;
    }

    SocketId m_fd{SocketError};
    std::string m_dir;
    std::variant<sockaddr_in, sockaddr_un> m_addr;
};

//! Runs a client EventLoop on its own thread and connects one socket FD to the
//! server. The constructed ProxyClient can be used by the test thread to make
//! calls over that connection.
class ClientSetup
{
public:
    explicit ClientSetup(SocketId fd)
        : thread([this, fd] {
              EventLoop loop("mptest-client", [](mp::LogMessage log) {
                  KJ_LOG(INFO, log.level, log.message);
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
              });
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
    explicit ListenSetup(std::optional<size_t> max_connections = std::nullopt)
        : thread([this, max_connections] {
              EventLoop loop("mptest-server", [](mp::LogMessage log) {
                  KJ_LOG(INFO, log.level, log.message);
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
              });
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

    SocketListener listener;
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
    // first accepts clients disconnects.

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

} // namespace
} // namespace test
} // namespace mp
