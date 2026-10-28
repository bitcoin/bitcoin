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
#include <sync.h>
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
        assert(WITH_LOCK(m_loop_mutex, return !m_loop));
    };
    std::unique_ptr<interfaces::Init> connect(mp::Stream stream) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        startLoop();
        mp::EventLoopRef ref{loopRef()};
        return mp::ConnectStream<messages::Init>(*ref, std::move(stream));
    }
    void listen(mp::SocketId listen_fd, interfaces::Init& init, std::optional<size_t> max_connections) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        startLoop();
        mp::EventLoopRef ref{loopRef()};
        if (::listen(listen_fd, /*backlog=*/5) != 0) {
            throw std::system_error(errno, std::system_category());
        }
        mp::ListenConnections<messages::Init>(*ref, listen_fd, init, max_connections);
    }
    void serve(interfaces::Init& init, const std::function<mp::Stream()>& make_stream) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        mp::CurrentThread().thread_name = mp::ThreadName(m_exe_name);
        mp::LogOptions opts = {
            .log_fn = IpcLogFn,
            .log_level = GetRequestedIPCLogLevel()
        };
        mp::EventLoop* loop;
        {
            LOCK(m_loop_mutex);
            assert(!m_loop);
            loop = &m_loop.emplace(m_exe_name, std::move(opts), &m_context);
        }
        {
            // Hold a reference while setting up the parent connection.
            // The make_stream() callback can call makeStream(), which calls
            // loopRef(), and EventLoop::done() at this point will wrongly return
            // true, making loopRef() throw the "IPC event loop is shutting down"
            // error.
            mp::EventLoopRef setup_ref{*loop};
            mp::ServeStream<messages::Init>(*loop, make_stream(), init);
        }
        m_parent_connection = &loop->m_incoming_connections.back();
        loop->loop();
        LOCK(m_loop_mutex);
        m_loop.reset();
    }
    void disconnectIncoming() override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        // Return instead of throwing if the loop is not running, since this is
        // called during shutdown.
        std::optional<mp::EventLoopRef> ref;
        try {
            ref.emplace(loopRef());
        } catch (const Exception&) {
            return;
        }
        // Delete incoming connections, except the connection to a parent
        // process (if there is one), since a parent process should be able to
        // monitor and control this process, even during shutdown.
        (*ref)->sync([&] {
            (*ref)->m_incoming_connections.remove_if([this](mp::Connection& c) { return &c != m_parent_connection; });
        });
    }
    mp::Stream makeStream(mp::SocketId socket) override EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        startLoop();
        mp::EventLoopRef ref{loopRef()};
        return mp::MakeStream(*ref, socket);
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        mp::ProxyTypeRegister::types().at(type)(iface).cleanup_fns.emplace_back(std::move(cleanup));
    }
    Context& context() override { return m_context; }
    void startLoop() EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        if (WITH_LOCK(m_loop_mutex, return m_loop.has_value())) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
            mp::LogOptions opts = {
                .log_fn = IpcLogFn,
                .log_level = GetRequestedIPCLogLevel()
            };
            mp::EventLoop* loop;
            {
                LOCK(m_loop_mutex);
                loop = &m_loop.emplace(m_exe_name, std::move(opts), &m_context);
            }
            m_loop_ref.emplace(*loop);
            promise.set_value();
            loop->loop();
            LOCK(m_loop_mutex);
            m_loop.reset();
        });
        promise.get_future().wait();
    }
    //! Return a reference that keeps the event loop running, or throw if the
    //! loop is not running. Threads that are not handling an IPC request need
    //! this, because in serve() mode nothing else keeps the loop running for
    //! them.
    [[nodiscard]] mp::EventLoopRef loopRef() EXCLUSIVE_LOCKS_REQUIRED(!m_loop_mutex)
    {
        LOCK(m_loop_mutex);
        if (!m_loop) throw Exception("IPC event loop is not running");
        mp::Lock lock(m_loop->m_mutex);
        if (m_loop->done()) throw Exception("IPC event loop is shutting down");
        mp::EventLoopRef ref{*m_loop, &lock};
        ref.m_lock = nullptr; // Lock m_mutex again when the reference is released.
        return ref;
    }
    const char* m_exe_name;
    Context m_context;
    //! Guards m_loop.
    Mutex m_loop_mutex;
    //! EventLoop object which manages I/O events for all connections.
    std::optional<mp::EventLoop> m_loop GUARDED_BY(m_loop_mutex);
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
