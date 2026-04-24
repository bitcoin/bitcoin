// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <future>
#include <kj/async.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <memory>
#include <mp/proxy-io.h>
#include <mutex>
#include <optional>
#include <ratio> // IWYU pragma: keep
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace mp {
namespace test {
namespace {

class UnixListener
{
public:
    UnixListener()
    {
        char dir_template[] = "/tmp/mptest-listener-XXXXXX";
        char* dir = mkdtemp(dir_template);
        KJ_REQUIRE(dir != nullptr);
        m_dir = dir;
        m_path = m_dir + "/socket";

        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(m_fd >= 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(m_path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, m_path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);
    }

    ~UnixListener()
    {
        if (m_fd >= 0) close(m_fd);
        if (!m_path.empty()) unlink(m_path.c_str());
        if (!m_dir.empty()) rmdir(m_dir.c_str());
    }

    int release()
    {
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }

    int Connect() const
    {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(fd >= 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(m_path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, m_path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        return fd;
    }

private:
    int m_fd{-1};
    std::string m_dir;
    std::string m_path;
};

class ClientSetup
{
public:
    explicit ClientSetup(int fd)
        : thread([this, fd] {
              EventLoop loop("mptest-client", [](mp::LogMessage log) {
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
              });
              client_promise.set_value(ConnectStream<messages::FooInterface>(loop, fd));
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
    std::thread thread;
};

class ListenSetup
{
public:
    explicit ListenSetup(std::optional<size_t> max_connections = std::nullopt)
        : thread([this, max_connections] {
              EventLoop loop("mptest-server", [this](mp::LogMessage log) {
                  if (log.level == mp::Log::Raise) throw std::runtime_error(log.message);
                  if (log.message.find("IPC server: socket connected.") != std::string::npos) {
                      std::lock_guard<std::mutex> lock(counter_mutex);
                      ++connected_count;
                      counter_cv.notify_all();
                  } else if (log.message.find("IPC server: socket disconnected.") != std::string::npos) {
                      std::lock_guard<std::mutex> lock(counter_mutex);
                      ++disconnected_count;
                      counter_cv.notify_all();
                  }
              });
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
        thread.join();
    }

    size_t ConnectedCount()
    {
        std::lock_guard<std::mutex> lock(counter_mutex);
        return connected_count;
    }

    void WaitForConnectedCount(size_t expected_count)
    {
        std::unique_lock<std::mutex> lock(counter_mutex);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        const bool matched = counter_cv.wait_until(lock, deadline, [&] {
            return connected_count >= expected_count;
        });
        KJ_REQUIRE(matched);
    }

    void WaitForDisconnectedCount(size_t expected_count)
    {
        std::unique_lock<std::mutex> lock(counter_mutex);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        const bool matched = counter_cv.wait_until(lock, deadline, [&] {
            return disconnected_count >= expected_count;
        });
        KJ_REQUIRE(matched);
    }

    UnixListener listener;
    std::promise<void> ready_promise;
    std::thread thread;
    std::mutex counter_mutex;
    std::condition_variable counter_cv;
    size_t connected_count{0};
    size_t disconnected_count{0};
};

KJ_TEST("ListenConnections accepts incoming connections")
{
    ListenSetup setup;
    auto client = std::make_unique<ClientSetup>(setup.listener.Connect());

    setup.WaitForConnectedCount(1);
    KJ_EXPECT(client->client->add(1, 2) == 3);
}

KJ_TEST("ListenConnections enforces a local connection limit")
{
    ListenSetup setup(/*max_connections=*/1);

    auto client1 = std::make_unique<ClientSetup>(setup.listener.Connect());
    setup.WaitForConnectedCount(1);
    KJ_EXPECT(client1->client->add(1, 2) == 3);

    auto client2 = std::make_unique<ClientSetup>(setup.listener.Connect());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    KJ_EXPECT(setup.ConnectedCount() == 1);

    client1.reset();
    setup.WaitForDisconnectedCount(1);
    setup.WaitForConnectedCount(2);

    KJ_EXPECT(client2->client->add(2, 3) == 5);
}

} // namespace
} // namespace test
} // namespace mp
