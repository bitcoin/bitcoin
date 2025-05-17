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
#include <system_error>
#include <thread>

#ifndef WIN32
#include <sys/socket.h>
#endif

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
    CapnpProtocol(const char* exe_name) : m_exe_name{exe_name} {}
    ~CapnpProtocol() noexcept(true)
    {
        if (m_loop) {
            std::unique_lock<std::mutex> lock(m_loop->m_mutex);
            m_loop->removeClient(lock);
        }
        if (m_loop_thread.joinable()) m_loop_thread.join();
        assert(!m_loop);
    };
    std::unique_ptr<interfaces::Init> connect(mp::Stream stream) override
    {
        startLoop();
        return mp::ConnectStream<messages::Init>(*m_loop, std::move(stream));
    }
    void listen(mp::SocketId listen_fd, interfaces::Init& init) override
    {
        startLoop();
        if (::listen(listen_fd, /*backlog=*/5) != 0) {
            throw std::system_error(errno, std::system_category());
        }
        mp::ListenConnections<messages::Init>(*m_loop, listen_fd, init);
    }
    void serve(interfaces::Init& init, const std::function<mp::Stream()>& make_stream) override
    {
        assert(!m_loop);
        mp::g_thread_context.thread_name = mp::ThreadName(m_exe_name);
        m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
        mp::ServeStream<messages::Init>(*m_loop, make_stream(), init);
        m_loop->loop();
        m_loop.reset();
    }
    mp::Stream makeStream(mp::SocketId socket) override
    {
        startLoop();
        return m_loop->m_io_context.lowLevelProvider->wrapSocketFd(socket, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP);
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        mp::ProxyTypeRegister::types().at(type)(iface).cleanup_fns.emplace_back(std::move(cleanup));
    }
    Context& context() override { return m_context; }
    void startLoop()
    {
        if (m_loop) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
            m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
            {
                std::unique_lock<std::mutex> lock(m_loop->m_mutex);
                m_loop->addClient(lock);
            }
            promise.set_value();
            m_loop->loop();
            m_loop.reset();
        });
        promise.get_future().wait();
    }
    const char* m_exe_name;
    Context m_context;
    std::optional<mp::EventLoop> m_loop;
    std::thread m_loop_thread;
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol(const char* exe_name) { return std::make_unique<CapnpProtocol>(exe_name); }
} // namespace capnp
} // namespace ipc
