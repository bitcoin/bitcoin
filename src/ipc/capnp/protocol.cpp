// Copyright (c) 2021-present The Bitcoin Core developers
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
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <util/log.h>
#include <util/threadnames.h>

#include <cassert>
#include <cerrno>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <vector>

namespace ipc {
namespace capnp {
namespace {

mp::Log GetRequestedIPCLogLevel()
{
    if (util::log::ShouldTraceLog(BCLog::IPC)) return mp::Log::Trace;
    if (util::log::ShouldDebugLog(BCLog::IPC)) return mp::Log::Debug;

    // Info, Warning, and Error are logged unconditionally
    return mp::Log::Info;
}

void IpcLogFn(mp::LogMessage message)
{
    switch (message.level) {
    case mp::Log::Trace:
        LogTrace(BCLog::IPC, "%s", message.message);
        return;
    case mp::Log::Debug:
        LogDebug(BCLog::IPC, "%s", message.message);
        return;
    case mp::Log::Info:
        LogInfo("ipc: %s", message.message);
        return;
    case mp::Log::Warning:
        LogWarning("ipc: %s", message.message);
        return;
    case mp::Log::Error:
        LogError("ipc: %s", message.message);
        return;
    case mp::Log::Raise:
        LogError("ipc: %s", message.message);
        throw Exception(message.message);
    } // no default case, so the compiler can warn about missing cases

    // Be conservative and assume that if MP ever adds a new log level, it
    // should only be shown at our most verbose level.
    LogTrace(BCLog::IPC, "%s", message.message);
}

class CapnpProtocol : public Protocol
{
public:
    CapnpProtocol(const char* exe_name) : m_exe_name{exe_name} {}
    ~CapnpProtocol() noexcept(true)
    {
        m_loop_ref.reset();
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
        mp::CurrentThread().thread_name = mp::ThreadName(m_exe_name);
        mp::LogOptions opts = {
            .log_fn = IpcLogFn,
            .log_level = GetRequestedIPCLogLevel()
        };
        m_loop.emplace(m_exe_name, std::move(opts), &m_context);
        mp::ServeStream<messages::Init>(*m_loop, make_stream(), init);
        m_parent_connection = &m_loop->incoming_connections().back();
        m_loop->loop();
        m_loop.reset();
    }
    void disconnectIncoming() override
    {
        if (!m_loop) return;
        // Disconnect incoming connections, except the connection to a parent
        // process (if there is one), since a parent process should be able to
        // monitor and control this process, even during shutdown.
        //
        // Disconnecting cancels the KJ promise of any call in progress, but a
        // server method body that was already dispatched to a worker thread is
        // NOT interrupted and runs to completion. So before destroying the
        // connections and returning, wait for in-flight call bodies to finish
        // (mp::Connection::waitDrained). Without this wait, a still-running
        // body (e.g. Mining.checkBlock) could dereference node state that the
        // caller (Shutdown) frees right after this function returns, causing
        // https://github.com/bitcoin/bitcoin/issues/35845.
        //
        // The wait happens off the event loop thread, between two sync()
        // calls: call bodies need the event loop to deliver their results, so
        // blocking the loop until they finish would deadlock.
        //
        // Limitations, intentional to keep this narrow (see #35845 discussion):
        // - Only server call *bodies* are drained. ProxyServer m_impl
        //   destructors scheduled via EventLoop::addAsyncCleanup() run later
        //   on the async cleanup thread and are not waited for. (One of
        //   several reasons IPC object implementations should not have
        //   nontrivial destructors.)
        // - Only the removed connections are drained; the kept-open parent
        //   connection is not (that would require pausing new calls on it).
        // - New incoming connections can still be accepted while this runs;
        //   preventing that requires an API to stop the listener, proposed in
        //   https://github.com/bitcoin-core/libmultiprocess/pull/269.
        std::vector<mp::Connection*> disconnected;
        m_loop->sync([&] {
            for (mp::Connection& connection : m_loop->incoming_connections()) {
                if (&connection != m_parent_connection) {
                    connection.disconnect();
                    disconnected.push_back(&connection);
                }
            }
        });
        // After disconnect(), the remaining counted server objects are exactly
        // the ones kept alive by in-flight calls (idle ones are garbage
        // collected synchronously by the disconnect), so a nonzero count means
        // the wait below will actually block. Log when that happens so a
        // shutdown hang here is diagnosable.
        size_t pending{0};
        for (const mp::Connection* connection : disconnected) pending += connection->pendingServerObjects();
        if (pending > 0) {
            LogInfo("ipc: Waiting for %d server object(s) in use by in-flight IPC call(s) on %d connection(s) to be freed",
                    pending, disconnected.size());
        }
        for (mp::Connection* connection : disconnected) connection->waitDrained();
        if (pending > 0) LogInfo("ipc: Incoming IPC connections drained");
        m_loop->sync([&] {
            m_loop->m_incoming_connections.remove_if([this](mp::Connection& c) { return &c != m_parent_connection; });
        });
    }
    mp::Stream makeStream(mp::SocketId socket) override
    {
        startLoop();
        return mp::MakeStream(*m_loop, socket);
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
            mp::LogOptions opts = {
                .log_fn = IpcLogFn,
                .log_level = GetRequestedIPCLogLevel()
            };
            m_loop.emplace(m_exe_name, std::move(opts), &m_context);
            m_loop_ref.emplace(*m_loop);
            promise.set_value();
            m_loop->loop();
            m_loop.reset();
        });
        promise.get_future().wait();
    }
    const char* m_exe_name;
    Context m_context;
    //! EventLoop object which manages I/O events for all connections.
    std::optional<mp::EventLoop> m_loop;
    //! Reference to the same EventLoop. Increments the loop’s refcount on
    //! creation, decrements on destruction. The loop thread exits when the
    //! refcount reaches 0. Other IPC objects also hold their own EventLoopRef.
    std::optional<mp::EventLoopRef> m_loop_ref;
    //! Connection to parent, if this is a child process spawned by a parent process.
    mp::Connection* m_parent_connection{nullptr};
    std::thread m_loop_thread;
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol(const char* exe_name) { return std::make_unique<CapnpProtocol>(exe_name); }
} // namespace capnp
} // namespace ipc
