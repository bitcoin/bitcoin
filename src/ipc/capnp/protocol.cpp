// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/init.capnp.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <ipc/capnp/protocol.h>
#include <ipc/exception.h>
#include <ipc/protocol.h>
#include <kj/async.h>
#include <logging.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <util/threadnames.h>

#include <assert.h>
#include <errno.h>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <thread>

namespace ipc {
namespace capnp {
namespace {
void IpcLogFn(bool raise, std::string message)
{
    LogDebug(BCLog::IPC, "%s\n", message);
    if (raise) throw Exception(message);
}

class CapnpProtocol : public Protocol
{
public:
    ~CapnpProtocol() noexcept(true)
    {
        {
            LOCK(m_loop_mutex);
            if (m_loop) {
                std::unique_lock<std::mutex> lock(m_loop->m_mutex);
                m_loop->removeClient(lock);
            }
        }
        if (m_loop_thread.joinable()) m_loop_thread.join();
        assert(WITH_LOCK(m_loop_mutex, return !m_loop));
    };
    std::unique_ptr<interfaces::Init> connect(int fd, const char* exe_name) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        startLoop(exe_name);
        return mp::ConnectStream<messages::Init>(WITH_LOCK(m_loop_mutex, return *m_loop), fd);
    }
    void listen(int listen_fd, const char* exe_name, interfaces::Init& init) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        startLoop(exe_name);
        if (::listen(listen_fd, /*backlog=*/5) != 0) {
            throw std::system_error(errno, std::system_category());
        }
        mp::ListenConnections<messages::Init>(WITH_LOCK(m_loop_mutex, return *m_loop), listen_fd, init);
    }
    void serve(int fd, const char* exe_name, interfaces::Init& init, const std::function<void()>& ready_fn = {}) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        assert(WITH_LOCK(m_loop_mutex, return !m_loop));
        mp::g_thread_context.thread_name = mp::ThreadName(exe_name);
        auto& loop{WITH_LOCK(m_loop_mutex, m_loop.emplace(exe_name, &IpcLogFn, &m_context); return *m_loop)};
        if (ready_fn) ready_fn();
        mp::ServeStream<messages::Init>(loop, fd, init);
        loop.loop();
        WITH_LOCK(m_loop_mutex, m_loop.reset());
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        mp::ProxyTypeRegister::types().at(type)(iface).cleanup_fns.emplace_back(std::move(cleanup));
    }
    Context& context() override { return m_context; }
    void startLoop(const char* exe_name) EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        if (WITH_LOCK(m_loop_mutex, return m_loop.has_value())) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
            auto& loop{WITH_LOCK(m_loop_mutex, m_loop.emplace(exe_name, &IpcLogFn, &m_context); return *m_loop)};
            {
                std::unique_lock<std::mutex> lock(loop.m_mutex);
                loop.addClient(lock);
            }
            promise.set_value();
            loop.loop();
            WITH_LOCK(m_loop_mutex, m_loop.reset());
        });
        promise.get_future().wait();
    }
    Context m_context;
    std::thread m_loop_thread;
    Mutex m_loop_mutex;
    std::optional<mp::EventLoop> m_loop GUARDED_BY(m_loop_mutex);
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol() { return std::make_unique<CapnpProtocol>(); }
} // namespace capnp
} // namespace ipc
