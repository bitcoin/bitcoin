// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_IO_H
#define MP_PROXY_IO_H

#include <mp/proxy.h>
#include <mp/util.h>

#include <mp/proxy.capnp.h>

#include <capnp/rpc-twoparty.h>

#include <assert.h>
#include <condition_variable>
#include <functional>
#include <kj/function.h>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace mp {
struct ThreadContext;

struct InvokeContext
{
    Connection& connection;
};

struct ClientInvokeContext : InvokeContext
{
    ThreadContext& thread_context;
    ClientInvokeContext(Connection& conn, ThreadContext& thread_context)
        : InvokeContext{conn}, thread_context{thread_context}
    {
    }
};

template <typename ProxyServer, typename CallContext_>
struct ServerInvokeContext : InvokeContext
{
    using CallContext = CallContext_;

    ProxyServer& proxy_server;
    CallContext& call_context;
    int req;

    ServerInvokeContext(ProxyServer& proxy_server, CallContext& call_context, int req)
        : InvokeContext{*proxy_server.m_context.connection}, proxy_server{proxy_server}, call_context{call_context}, req{req}
    {
    }
};

template <typename Interface, typename Params, typename Results>
using ServerContext = ServerInvokeContext<ProxyServer<Interface>, ::capnp::CallContext<Params, Results>>;

template <>
struct ProxyClient<Thread> : public ProxyClientBase<Thread, ::capnp::Void>
{
    using ProxyClientBase::ProxyClientBase;
    // https://stackoverflow.com/questions/22357887/comparing-two-mapiterators-why-does-it-need-the-copy-constructor-of-stdpair
    ProxyClient(const ProxyClient&) = delete;
    ~ProxyClient();

    //! Reference to callback function that is run if there is a sudden
    //! disconnect and the Connection object is destroyed before this
    //! ProxyClient<Thread> object. The callback will destroy this object and
    //! remove its entry from the thread's request_threads or callback_threads
    //! map. It will also reset m_disconnect_cb so the destructor does not
    //! access it. In the normal case where there is no sudden disconnect, the
    //! destructor will unregister m_disconnect_cb so the callback is never run.
    //! Since this variable is accessed from multiple threads, accesses should
    //! be guarded with the associated Waiter::m_mutex.
    std::optional<CleanupIt> m_disconnect_cb;
};

template <>
struct ProxyServer<Thread> final : public Thread::Server
{
public:
    ProxyServer(ThreadContext& thread_context, std::thread&& thread);
    ~ProxyServer();
    kj::Promise<void> getName(GetNameContext context) override;
    ThreadContext& m_thread_context;
    std::thread m_thread;
};

//! Handler for kj::TaskSet failed task events.
class LoggingErrorHandler : public kj::TaskSet::ErrorHandler
{
public:
    LoggingErrorHandler(EventLoop& loop) : m_loop(loop) {}
    void taskFailed(kj::Exception&& exception) override;
    EventLoop& m_loop;
};

//! Log flags. Update stringify function if changed!
enum class Log {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Raise,
};

kj::StringPtr KJ_STRINGIFY(Log flags);

struct LogMessage {

    //! Message to be logged
    std::string message;

    //! The severity level of this message
    Log level;
};

using LogFn = std::function<void(LogMessage)>;

struct LogOptions {

    //! External logging callback.
    LogFn log_fn;

    //! Maximum number of characters to use when representing
    //! request and response structs as strings.
    size_t max_chars{200};

    //! Messages with a severity level less than log_level will not be
    //! reported.
    Log log_level{Log::Trace};
};

class Logger
{
public:
    Logger(const LogOptions& options, Log log_level) : m_options(options), m_log_level(log_level) {}

    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    ~Logger() noexcept(false)
    {
        if (enabled()) m_options.log_fn({std::move(m_buffer).str(), m_log_level});
    }

    template <typename T>
    friend Logger& operator<<(Logger& logger, T&& value)
    {
        if (logger.enabled()) logger.m_buffer << std::forward<T>(value);
        return logger;
    }

    template <typename T>
    friend Logger& operator<<(Logger&& logger, T&& value)
    {
        return logger << std::forward<T>(value);
    }

    explicit operator bool() const
    {
        return enabled();
    }

private:
    bool enabled() const
    {
        return m_options.log_fn && m_log_level >= m_options.log_level;
    }

    const LogOptions& m_options;
    Log m_log_level;
    std::ostringstream m_buffer;
};

#define MP_LOGPLAIN(loop, ...) if (mp::Logger logger{(loop).m_log_opts, __VA_ARGS__}; logger) logger

#define MP_LOG(loop, ...) MP_LOGPLAIN(loop, __VA_ARGS__) << "{" << LongThreadName((loop).m_exe_name) << "} "

std::string LongThreadName(const char* exe_name);

//! Event loop implementation.
//!
//! Cap'n Proto threading model is very simple: all I/O operations are
//! asynchronous and must be performed on a single thread. This includes:
//!
//! - Code starting an asynchronous operation (calling a function that returns a
//!   promise object)
//! - Code notifying that an asynchronous operation is complete (code using a
//!   fulfiller object)
//! - Code handling a completed operation (code chaining or waiting for a promise)
//!
//! All of this code needs to access shared state, and there is no mutex that
//! can be acquired to lock this state because Cap'n Proto
//! assumes it will only be accessed from one thread. So all this code needs to
//! actually run on one thread, and the EventLoop::loop() method is the entry point for
//! this thread. ProxyClient and ProxyServer objects that use other threads and
//! need to perform I/O operations post to this thread using EventLoop::post()
//! and EventLoop::sync() methods.
//!
//! Specifically, because ProxyClient methods can be called from arbitrary
//! threads, and ProxyServer methods can run on arbitrary threads, ProxyClient
//! methods use the EventLoop thread to send requests, and ProxyServer methods
//! use the thread to return results.
//!
//! Based on https://groups.google.com/d/msg/capnproto/TuQFF1eH2-M/g81sHaTAAQAJ
class EventLoop
{
public:
    //! Construct event loop object with default logging options.
    EventLoop(const char* exe_name, LogFn log_fn, void* context = nullptr)
        : EventLoop(exe_name, LogOptions{std::move(log_fn)}, context){}

    //! Construct event loop object with specified logging options.
    EventLoop(const char* exe_name, LogOptions log_opts, void* context = nullptr);

    //! Backwards-compatible constructor for previous (deprecated) logging callback signature
    EventLoop(const char* exe_name, std::function<void(bool, std::string)> old_callback, void* context = nullptr)
        : EventLoop(exe_name,
                LogFn{[old_callback = std::move(old_callback)](LogMessage log_data) {old_callback(log_data.level == Log::Raise, std::move(log_data.message));}},
                context){}

    ~EventLoop();

    //! Run event loop. Does not return until shutdown. This should only be
    //! called once from the m_thread_id thread. This will block until
    //! the m_num_clients reference count is 0.
    void loop();

    //! Run function on event loop thread. Does not return until function completes.
    //! Must be called while the loop() function is active.
    void post(kj::Function<void()> fn);

    //! Wrapper around EventLoop::post that takes advantage of the
    //! fact that callable will not go out of scope to avoid requirement that it
    //! be copyable.
    template <typename Callable>
    void sync(Callable&& callable)
    {
        post(std::forward<Callable>(callable));
    }

    //! Register cleanup function to run on asynchronous worker thread without
    //! blocking the event loop thread.
    void addAsyncCleanup(std::function<void()> fn);

    //! Start asynchronous worker thread if necessary. This is only done if
    //! there are ProxyServerBase::m_impl objects that need to be destroyed
    //! asynchronously, without tying up the event loop thread. This can happen
    //! when an interface does not declare a destroy() method that would allow
    //! the client to wait for the destructor to finish and run it on a
    //! dedicated thread. It can also happen whenever this is a broken
    //! connection and the client is no longer around to call the destructors
    //! and the server objects need to be garbage collected. In both cases, it
    //! is important that ProxyServer::m_impl destructors do not run on the
    //! eventloop thread because they may need it to do I/O if they perform
    //! other IPC calls.
    void startAsyncThread() MP_REQUIRES(m_mutex);

    //! Check if loop should exit.
    bool done() const MP_REQUIRES(m_mutex);

    //! Process name included in thread names so combined debug output from
    //! multiple processes is easier to understand.
    const char* m_exe_name;

    //! ID of the event loop thread
    std::thread::id m_thread_id = std::this_thread::get_id();

    //! Handle of an async worker thread. Joined on destruction. Unset if async
    //! method has not been called.
    std::thread m_async_thread;

    //! Callback function to run on event loop thread during post() or sync() call.
    kj::Function<void()>* m_post_fn MP_GUARDED_BY(m_mutex) = nullptr;

    //! Callback functions to run on async thread.
    std::optional<CleanupList> m_async_fns MP_GUARDED_BY(m_mutex);

    //! Pipe read handle used to wake up the event loop thread.
    int m_wait_fd = -1;

    //! Pipe write handle used to wake up the event loop thread.
    int m_post_fd = -1;

    //! Number of clients holding references to ProxyServerBase objects that
    //! reference this event loop.
    int m_num_clients MP_GUARDED_BY(m_mutex) = 0;

    //! Mutex and condition variable used to post tasks to event loop and async
    //! thread.
    Mutex m_mutex;
    std::condition_variable m_cv;

    //! Capnp IO context.
    kj::AsyncIoContext m_io_context;

    //! Capnp error handler. Needs to outlive m_task_set.
    LoggingErrorHandler m_error_handler{*this};

    //! Capnp list of pending promises.
    std::unique_ptr<kj::TaskSet> m_task_set;

    //! List of connections.
    std::list<Connection> m_incoming_connections;

    //! Logging options
    LogOptions m_log_opts;

    //! External context pointer.
    void* m_context;
};

//! Single element task queue used to handle recursive capnp calls. (If server
//! makes an callback into the client in the middle of a request, while client
//! thread is blocked waiting for server response, this is what allows the
//! client to run the request in the same thread, the same way code would run in
//! single process, with the callback sharing same thread stack as the original
//! call.
struct Waiter
{
    Waiter() = default;

    template <typename Fn>
    bool post(Fn&& fn)
    {
        const Lock lock(m_mutex);
        if (m_fn) return false;
        m_fn = std::forward<Fn>(fn);
        m_cv.notify_all();
        return true;
    }

    template <class Predicate>
    void wait(Lock& lock, Predicate pred)
    {
        m_cv.wait(lock.m_lock, [&]() MP_REQUIRES(m_mutex) {
            // Important for this to be "while (m_fn)", not "if (m_fn)" to avoid
            // a lost-wakeup bug. A new m_fn and m_cv notification might be sent
            // after the fn() call and before the lock.lock() call in this loop
            // in the case where a capnp response is sent and a brand new
            // request is immediately received.
            while (m_fn) {
                auto fn = std::move(*m_fn);
                m_fn.reset();
                Unlock(lock, fn);
            }
            const bool done = pred();
            return done;
        });
    }

    //! Mutex mainly used internally by waiter class, but also used externally
    //! to guard access to related state. Specifically, since the thread_local
    //! ThreadContext struct owns a Waiter, the Waiter::m_mutex is used to guard
    //! access to other parts of the struct to avoid needing to deal with more
    //! mutexes than necessary. This mutex can be held at the same time as
    //! EventLoop::m_mutex as long as Waiter::mutex is locked first and
    //! EventLoop::m_mutex is locked second.
    Mutex m_mutex;
    std::condition_variable m_cv;
    std::optional<kj::Function<void()>> m_fn MP_GUARDED_BY(m_mutex);
};

//! Object holding network & rpc state associated with either an incoming server
//! connection, or an outgoing client connection. It must be created and destroyed
//! on the event loop thread.
//! In addition to Cap'n Proto state, it also holds lists of callbacks to run
//! when the connection is closed.
class Connection
{
public:
    Connection(EventLoop& loop, kj::Own<kj::AsyncIoStream>&& stream_)
        : m_loop(loop), m_stream(kj::mv(stream_)),
          m_network(*m_stream, ::capnp::rpc::twoparty::Side::CLIENT, ::capnp::ReaderOptions()),
          m_rpc_system(::capnp::makeRpcClient(m_network)) {}
    Connection(EventLoop& loop,
        kj::Own<kj::AsyncIoStream>&& stream_,
        const std::function<::capnp::Capability::Client(Connection&)>& make_client)
        : m_loop(loop), m_stream(kj::mv(stream_)),
          m_network(*m_stream, ::capnp::rpc::twoparty::Side::SERVER, ::capnp::ReaderOptions()),
          m_rpc_system(::capnp::makeRpcServer(m_network, make_client(*this))) {}

    //! Run cleanup functions. Must be called from the event loop thread. First
    //! calls synchronous cleanup functions while blocked (to free capnp
    //! Capability::Client handles owned by ProxyClient objects), then schedules
    //! asynchronous cleanup functions to run in a worker thread (to run
    //! destructors of m_impl instances owned by ProxyServer objects).
    ~Connection();

    //! Register synchronous cleanup function to run on event loop thread (with
    //! access to capnp thread local variables) when disconnect() is called.
    //! any new i/o.
    CleanupIt addSyncCleanup(std::function<void()> fn);
    void removeSyncCleanup(CleanupIt it);

    //! Add disconnect handler.
    template <typename F>
    void onDisconnect(F&& f)
    {
        // Add disconnect handler to local TaskSet to ensure it is cancelled and
        // will never run after connection object is destroyed. But when disconnect
        // handler fires, do not call the function f right away, instead add it
        // to the EventLoop TaskSet to avoid "Promise callback destroyed itself"
        // error in cases where f deletes this Connection object.
        m_on_disconnect.add(m_network.onDisconnect().then(
            [f = std::forward<F>(f), this]() mutable { m_loop->m_task_set->add(kj::evalLater(kj::mv(f))); }));
    }

    EventLoopRef m_loop;
    kj::Own<kj::AsyncIoStream> m_stream;
    LoggingErrorHandler m_error_handler{*m_loop};
    kj::TaskSet m_on_disconnect{m_error_handler};
    ::capnp::TwoPartyVatNetwork m_network;
    std::optional<::capnp::RpcSystem<::capnp::rpc::twoparty::VatId>> m_rpc_system;

    // ThreadMap interface client, used to create a remote server thread when an
    // client IPC call is being made for the first time from a new thread.
    ThreadMap::Client m_thread_map{nullptr};

    //! Collection of server-side IPC worker threads (ProxyServer<Thread> objects previously returned by
    //! ThreadMap.makeThread) used to service requests to clients.
    ::capnp::CapabilityServerSet<Thread> m_threads;

    //! Cleanup functions to run if connection is broken unexpectedly.  List
    //! will be empty if all ProxyClient are destroyed cleanly before the
    //! connection is destroyed.
    CleanupList m_sync_cleanup_fns;
};

//! Vat id for server side of connection. Required argument to RpcSystem::bootStrap()
//!
//! "Vat" is Cap'n Proto nomenclature for a host of various objects that facilitates
//! bidirectional communication with other vats; it is often but not always 1-1 with
//! processes. Cap'n Proto doesn't reference clients or servers per se; instead everything
//! is just a vat.
//!
//! See also: https://github.com/capnproto/capnproto/blob/9021f0c722b36cb11e3690b0860939255ebad39c/c%2B%2B/src/capnp/rpc.capnp#L42-L56
struct ServerVatId
{
    ::capnp::word scratch[4]{};
    ::capnp::MallocMessageBuilder message{scratch};
    ::capnp::rpc::twoparty::VatId::Builder vat_id{message.getRoot<::capnp::rpc::twoparty::VatId>()};
    ServerVatId() { vat_id.setSide(::capnp::rpc::twoparty::Side::SERVER); }
};

template <typename Interface, typename Impl>
ProxyClientBase<Interface, Impl>::ProxyClientBase(typename Interface::Client client,
    Connection* connection,
    bool destroy_connection)
    : m_client(std::move(client)), m_context(connection)

{
    // Handler for the connection getting destroyed before this client object.
    auto disconnect_cb = m_context.connection->addSyncCleanup([this]() {
        // Release client capability by move-assigning to temporary.
        {
            typename Interface::Client(std::move(m_client));
        }
        Lock lock{m_context.loop->m_mutex};
        m_context.connection = nullptr;
    });

    // Two shutdown sequences are supported:
    //
    // - A normal sequence where client proxy objects are deleted by external
    //   code that no longer needs them
    //
    // - A garbage collection sequence where the connection or event loop shuts
    //   down while external code is still holding client references.
    //
    // The first case is handled here when m_context.connection is not null. The
    // second case is handled by the disconnect_cb function, which sets
    // m_context.connection to null so nothing happens here.
    m_context.cleanup_fns.emplace_front([this, destroy_connection, disconnect_cb]{
    {
        // If the capnp interface defines a destroy method, call it to destroy
        // the remote object, waiting for it to be deleted server side. If the
        // capnp interface does not define a destroy method, this will just call
        // an empty stub defined in the ProxyClientBase class and do nothing.
        Sub::destroy(*this);

        // FIXME: Could just invoke removed addCleanup fn here instead of duplicating code
        m_context.loop->sync([&]() {
            // Remove disconnect callback on cleanup so it doesn't run and try
            // to access this object after it's destroyed. This call needs to
            // run inside loop->sync() on the event loop thread because
            // otherwise, if there were an ill-timed disconnect, the
            // onDisconnect handler could fire and delete the Connection object
            // before the removeSyncCleanup call.
            if (m_context.connection) m_context.connection->removeSyncCleanup(disconnect_cb);

            // Release client capability by move-assigning to temporary.
            {
                typename Interface::Client(std::move(m_client));
            }
            if (destroy_connection) {
                delete m_context.connection;
                m_context.connection = nullptr;
            }
        });
    }
    });
    Sub::construct(*this);
}

template <typename Interface, typename Impl>
ProxyClientBase<Interface, Impl>::~ProxyClientBase() noexcept
{
    CleanupRun(m_context.cleanup_fns);
}

template <typename Interface, typename Impl>
ProxyServerBase<Interface, Impl>::ProxyServerBase(std::shared_ptr<Impl> impl, Connection& connection)
    : m_impl(std::move(impl)), m_context(&connection)
{
    assert(m_impl);
}

//! ProxyServer destructor, called from the EventLoop thread by Cap'n Proto
//! garbage collection code after there are no more references to this object.
//! This will typically happen when the corresponding ProxyClient object on the
//! other side of the connection is destroyed. It can also happen earlier if the
//! connection is broken or destroyed. In the latter case this destructor will
//! typically be called inside m_rpc_system.reset() call in the ~Connection
//! destructor while the Connection object still exists. However, because
//! ProxyServer objects are refcounted, and the Connection object could be
//! destroyed while asynchronous IPC calls are still in-flight, it's possible
//! for this destructor to be called after the Connection object no longer
//! exists, so it is NOT valid to dereference the m_context.connection pointer
//! from this function.
template <typename Interface, typename Impl>
ProxyServerBase<Interface, Impl>::~ProxyServerBase()
{
    if (m_impl) {
        // If impl is non-null at this point, it means no client is waiting for
        // the m_impl server object to be destroyed synchronously. This can
        // happen either if the interface did not define a "destroy" method (see
        // invokeDestroy method below), or if a destroy method was defined, but
        // the connection was broken before it could be called.
        //
        // In either case, be conservative and run the cleanup on an
        // asynchronous thread, to avoid destructors or cleanup functions
        // blocking or deadlocking the current EventLoop thread, since they
        // could be making IPC calls.
        //
        // Technically this is a little too conservative since if the interface
        // defines a "destroy" method, but the destroy method does not accept a
        // Context parameter specifying a worker thread, the cleanup method
        // would run on the EventLoop thread normally (when connection is
        // unbroken), but will not run on the EventLoop thread now (when
        // connection is broken). Probably some refactoring of the destructor
        // and invokeDestroy function is possible to make this cleaner and more
        // consistent.
        m_context.loop->addAsyncCleanup([impl=std::move(m_impl), fns=std::move(m_context.cleanup_fns)]() mutable {
            impl.reset();
            CleanupRun(fns);
        });
    }
    assert(m_context.cleanup_fns.empty());
}

//! If the capnp interface defined a special "destroy" method, as described the
//! ProxyClientBase class, this method will be called and synchronously destroy
//! m_impl before returning to the client.
//!
//! If the capnp interface does not define a "destroy" method, this will never
//! be called and the ~ProxyServerBase destructor will be responsible for
//! deleting m_impl asynchronously, whenever the ProxyServer object gets garbage
//! collected by Cap'n Proto.
//!
//! This method is called in the same way other proxy server methods are called,
//! via the serverInvoke function. Basically serverInvoke just calls this as a
//! substitute for a non-existent m_impl->destroy() method. If the destroy
//! method has any parameters or return values they will be handled in the
//! normal way by PassField/ReadField/BuildField functions. Particularly if a
//! Context.thread parameter was passed, this method will run on the worker
//! thread specified by the client. Otherwise it will run on the EventLoop
//! thread, like other server methods without an assigned thread.
template <typename Interface, typename Impl>
void ProxyServerBase<Interface, Impl>::invokeDestroy()
{
    m_impl.reset();
    CleanupRun(m_context.cleanup_fns);
}

//! Map from Connection to local or remote thread handle which will be used over
//! that connection. This map will typically only contain one entry, but can
//! contain multiple if a single thread makes IPC calls over multiple
//! connections. A std::optional value type is used to avoid the map needing to
//! be locked while ProxyClient<Thread> objects are constructed, see
//! ThreadContext "Synchronization note" below.
using ConnThreads = std::map<Connection*, std::optional<ProxyClient<Thread>>>;
using ConnThread = ConnThreads::iterator;

// Retrieve ProxyClient<Thread> object associated with this connection from a
// map, or create a new one and insert it into the map. Return map iterator and
// inserted bool.
std::tuple<ConnThread, bool> SetThread(GuardedRef<ConnThreads> threads, Connection* connection, const std::function<Thread::Client()>& make_thread);

//! The thread_local ThreadContext g_thread_context struct provides information
//! about individual threads and a way of communicating between them. Because
//! it's a thread local struct, each ThreadContext instance is initialized by
//! the thread that owns it.
//!
//! ThreadContext is used for any client threads created externally which make
//! IPC calls, and for server threads created by
//! ProxyServer<ThreadMap>::makeThread() which execute IPC calls for clients.
//!
//! In both cases, the struct holds information like the thread name, and a
//! Waiter object where the EventLoop can post incoming IPC requests to execute
//! on the thread. The struct also holds ConnThread maps associating the thread
//! with local and remote ProxyClient<Thread> objects.
struct ThreadContext
{
    //! Identifying string for debug.
    std::string thread_name;

    //! Waiter object used to allow remote clients to execute code on this
    //! thread. For server threads created by
    //! ProxyServer<ThreadMap>::makeThread(), this is initialized in that
    //! function. Otherwise, for client threads created externally, this is
    //! initialized the first time the thread tries to make an IPC call. Having
    //! a waiter is necessary for threads making IPC calls in case a server they
    //! are calling expects them to execute a callback during the call, before
    //! it sends a response.
    //!
    //! For IPC client threads, the Waiter pointer is never cleared and the Waiter
    //! just gets destroyed when the thread does. For server threads created by
    //! makeThread(), this pointer is set to null in the ~ProxyServer<Thread> as
    //! a signal for the thread to exit and destroy itself. In both cases, the
    //! same Waiter object is used across different calls and only created and
    //! destroyed once for the lifetime of the thread.
    std::unique_ptr<Waiter> waiter = nullptr;

    //! When client is making a request to a server, this is the
    //! `callbackThread` argument it passes in the request, used by the server
    //! in case it needs to make callbacks into the client that need to execute
    //! while the client is waiting. This will be set to a local thread object.
    //!
    //! Synchronization note: The callback_thread and request_thread maps are
    //! only ever accessed internally by this thread's destructor and externally
    //! by Cap'n Proto event loop threads. Since it's possible for IPC client
    //! threads to make calls over different connections that could have
    //! different event loops, these maps are guarded by Waiter::m_mutex in case
    //! different event loop threads add or remove map entries simultaneously.
    //! However, individual ProxyClient<Thread> objects in the maps will only be
    //! associated with one event loop and guarded by EventLoop::m_mutex. So
    //! Waiter::m_mutex does not need to be held while accessing individual
    //! ProxyClient<Thread> instances, and may even need to be released to
    //! respect lock order and avoid locking Waiter::m_mutex before
    //! EventLoop::m_mutex.
    ConnThreads callback_threads MP_GUARDED_BY(waiter->m_mutex);

    //! When client is making a request to a server, this is the `thread`
    //! argument it passes in the request, used to control which thread on
    //! server will be responsible for executing it. If client call is being
    //! made from a local thread, this will be a remote thread object returned
    //! by makeThread. If a client call is being made from a thread currently
    //! handling a server request, this will be set to the `callbackThread`
    //! request thread argument passed in that request.
    //!
    //! Synchronization note: \ref callback_threads note applies here as well.
    ConnThreads request_threads MP_GUARDED_BY(waiter->m_mutex);

    //! Whether this thread is a capnp event loop thread. Not really used except
    //! to assert false if there's an attempt to execute a blocking operation
    //! which could deadlock the thread.
    bool loop_thread = false;
};

//! Given stream file descriptor, make a new ProxyClient object to send requests
//! over the stream. Also create a new Connection object embedded in the
//! client that is freed when the client is closed.
template <typename InitInterface>
std::unique_ptr<ProxyClient<InitInterface>> ConnectStream(EventLoop& loop, int fd)
{
    typename InitInterface::Client init_client(nullptr);
    std::unique_ptr<Connection> connection;
    loop.sync([&] {
        auto stream =
            loop.m_io_context.lowLevelProvider->wrapSocketFd(fd, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP);
        connection = std::make_unique<Connection>(loop, kj::mv(stream));
        init_client = connection->m_rpc_system->bootstrap(ServerVatId().vat_id).castAs<InitInterface>();
        Connection* connection_ptr = connection.get();
        connection->onDisconnect([&loop, connection_ptr] {
            MP_LOG(loop, Log::Warning) << "IPC client: unexpected network disconnect.";
            delete connection_ptr;
        });
    });
    return std::make_unique<ProxyClient<InitInterface>>(
        kj::mv(init_client), connection.release(), /* destroy_connection= */ true);
}

//! Given stream and init objects, construct a new ProxyServer object that
//! handles requests from the stream by calling the init object. Embed the
//! ProxyServer in a Connection object that is stored and erased if
//! disconnected. This should be called from the event loop thread.
template <typename InitInterface, typename InitImpl>
void _Serve(EventLoop& loop, kj::Own<kj::AsyncIoStream>&& stream, InitImpl& init)
{
    loop.m_incoming_connections.emplace_front(loop, kj::mv(stream), [&](Connection& connection) {
        // Disable deleter so proxy server object doesn't attempt to delete the
        // init implementation when the proxy client is destroyed or
        // disconnected.
        return kj::heap<ProxyServer<InitInterface>>(std::shared_ptr<InitImpl>(&init, [](InitImpl*){}), connection);
    });
    auto it = loop.m_incoming_connections.begin();
    it->onDisconnect([&loop, it] {
        MP_LOG(loop, Log::Info) << "IPC server: socket disconnected.";
        loop.m_incoming_connections.erase(it);
    });
}

//! Given connection receiver and an init object, handle incoming connections by
//! calling _Serve, to create ProxyServer objects and forward requests to the
//! init object.
template <typename InitInterface, typename InitImpl>
void _Listen(EventLoop& loop, kj::Own<kj::ConnectionReceiver>&& listener, InitImpl& init)
{
    auto* ptr = listener.get();
    loop.m_task_set->add(ptr->accept().then(
        [&loop, &init, listener = kj::mv(listener)](kj::Own<kj::AsyncIoStream>&& stream) mutable {
            _Serve<InitInterface>(loop, kj::mv(stream), init);
            _Listen<InitInterface>(loop, kj::mv(listener), init);
        }));
}

//! Given stream file descriptor and an init object, handle requests on the
//! stream by calling methods on the Init object.
template <typename InitInterface, typename InitImpl>
void ServeStream(EventLoop& loop, int fd, InitImpl& init)
{
    _Serve<InitInterface>(
        loop, loop.m_io_context.lowLevelProvider->wrapSocketFd(fd, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP), init);
}

//! Given listening socket file descriptor and an init object, handle incoming
//! connections and requests by calling methods on the Init object.
template <typename InitInterface, typename InitImpl>
void ListenConnections(EventLoop& loop, int fd, InitImpl& init)
{
    loop.sync([&]() {
        _Listen<InitInterface>(loop,
            loop.m_io_context.lowLevelProvider->wrapListenSocketFd(fd, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP),
            init);
    });
}

extern thread_local ThreadContext g_thread_context; // NOLINT(bitcoin-nontrivial-threadlocal)
// Silence nonstandard bitcoin tidy error "Variable with non-trivial destructor
// cannot be thread_local" which should not be a problem on modern platforms, and
// could lead to a small memory leak at worst on older ones.

} // namespace mp

#endif // MP_PROXY_IO_H
