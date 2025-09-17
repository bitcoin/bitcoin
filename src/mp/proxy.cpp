// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/proxy.h>

#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/proxy.capnp.h>
#include <mp/type-threadmap.h>
#include <mp/util.h>

#include <atomic>
#include <capnp/capability.h>
#include <capnp/common.h> // IWYU pragma: keep
#include <capnp/rpc.h>
#include <condition_variable>
#include <functional>
#include <future>
#include <kj/async.h>
#include <kj/async-io.h>
#include <kj/async-prelude.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/function.h>
#include <kj/memory.h>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <utility>

namespace mp {

thread_local ThreadContext g_thread_context;

void LoggingErrorHandler::taskFailed(kj::Exception&& exception)
{
    KJ_LOG(ERROR, "Uncaught exception in daemonized task.", exception);
    m_loop.log() << "Uncaught exception in daemonized task.";
}

EventLoopRef::EventLoopRef(EventLoop& loop, Lock* lock) : m_loop(&loop), m_lock(lock)
{
    auto loop_lock{PtrOrValue{m_lock, m_loop->m_mutex}};
    loop_lock->assert_locked(m_loop->m_mutex);
    m_loop->m_num_clients += 1;
}

// Due to the conditionals in this function, MP_NO_TSA is required to avoid
// error "error: mutex 'loop_lock' is not held on every path through here
// [-Wthread-safety-analysis]"
void EventLoopRef::reset(bool relock) MP_NO_TSA
{
    if (auto* loop{m_loop}) {
        m_loop = nullptr;
        auto loop_lock{PtrOrValue{m_lock, loop->m_mutex}};
        loop_lock->assert_locked(loop->m_mutex);
        assert(loop->m_num_clients > 0);
        loop->m_num_clients -= 1;
        if (loop->done()) {
            loop->m_cv.notify_all();
            int post_fd{loop->m_post_fd};
            loop_lock->unlock();
            char buffer = 0;
            KJ_SYSCALL(write(post_fd, &buffer, 1)); // NOLINT(bugprone-suspicious-semicolon)
            // By default, do not try to relock `loop_lock` after writing,
            // because the event loop could wake up and destroy itself and the
            // mutex might no longer exist.
            if (relock) loop_lock->lock();
        }
    }
}

ProxyContext::ProxyContext(Connection* connection) : connection(connection), loop{*connection->m_loop} {}

Connection::~Connection()
{
    // Connection destructor is always called on the event loop thread. If this
    // is a local disconnect, it will trigger I/O, so this needs to run on the
    // event loop thread, and if there was a remote disconnect, this is called
    // by an onDisconnect callback directly from the event loop thread.
    assert(std::this_thread::get_id() == m_loop->m_thread_id);
    // Shut down RPC system first, since this will garbage collect any
    // ProxyServer objects that were not freed before the connection was closed.
    // Typically all ProxyServer objects associated with this connection will be
    // freed before this call returns. However that will not be the case if
    // there are asynchronous IPC calls over this connection still currently
    // executing. In that case, Cap'n Proto will destroy the ProxyServer objects
    // after the calls finish.
    m_rpc_system.reset();

    // ProxyClient cleanup handlers are in sync list, and ProxyServer cleanup
    // handlers are in the async list.
    //
    // The ProxyClient cleanup handlers are synchronous because they are fast
    // and don't do anything besides release capnp resources and reset state so
    // future calls to client methods immediately throw exceptions instead of
    // trying to communicating across the socket. The synchronous callbacks set
    // ProxyClient capability pointers to null, so new method calls on client
    // objects fail without triggering i/o or relying on event loop which may go
    // out of scope or trigger obscure capnp i/o errors.
    //
    // The ProxySever cleanup handlers call user defined destructors on server
    // object, which can run arbitrary blocking bitcoin code so they have to run
    // asynchronously in a different thread. The asynchronous cleanup functions
    // intentionally aren't started until after the synchronous cleanup
    // functions run, so client objects are fully disconnected before bitcoin
    // code in the destructors are run. This way if the bitcoin code tries to
    // make client requests the requests will just fail immediately instead of
    // sending i/o or accessing the event loop.
    //
    // The context where Connection objects are destroyed and this destructor is invoked
    // is different depending on whether this is an outgoing connection being used
    // to make an Init.makeX call() (e.g. Init.makeNode or Init.makeWalletClient) or an incoming
    // connection implementing the Init interface and handling the Init.makeX() calls.
    //
    // Either way when a connection is closed, capnp behavior is to call all
    // ProxyServer object destructors first, and then trigger an onDisconnect
    // callback.
    //
    // On incoming side of the connection, the onDisconnect callback is written
    // to delete the Connection object from the m_incoming_connections and call
    // this destructor which calls Connection::disconnect.
    //
    // On the outgoing side, the Connection object is owned by top level client
    // object client, which onDisconnect handler doesn't have ready access to,
    // so onDisconnect handler just calls Connection::disconnect directly
    // instead.
    //
    // Either way disconnect code runs in the event loop thread and called both
    // on clean and unclean shutdowns. In unclean shutdown case when the
    // connection is broken, sync and async cleanup lists will filled with
    // callbacks. In the clean shutdown case both lists will be empty.
    Lock lock{m_loop->m_mutex};
    while (!m_sync_cleanup_fns.empty()) {
        CleanupList fn;
        fn.splice(fn.begin(), m_sync_cleanup_fns, m_sync_cleanup_fns.begin());
        Unlock(lock, fn.front());
    }
}

CleanupIt Connection::addSyncCleanup(std::function<void()> fn)
{
    const Lock lock(m_loop->m_mutex);
    // Add cleanup callbacks to the front of list, so sync cleanup functions run
    // in LIFO order. This is a good approach because sync cleanup functions are
    // added as client objects are created, and it is natural to clean up
    // objects in the reverse order they were created. In practice, however,
    // order should not be significant because the cleanup callbacks run
    // synchronously in a single batch when the connection is broken, and they
    // only reset the connection pointers in the client objects without actually
    // deleting the client objects.
    return m_sync_cleanup_fns.emplace(m_sync_cleanup_fns.begin(), std::move(fn));
}

void Connection::removeSyncCleanup(CleanupIt it)
{
    // Require cleanup functions to be removed on the event loop thread to avoid
    // needing to deal with them being removed in the middle of a disconnect.
    assert(std::this_thread::get_id() == m_loop->m_thread_id);
    const Lock lock(m_loop->m_mutex);
    m_sync_cleanup_fns.erase(it);
}

void EventLoop::addAsyncCleanup(std::function<void()> fn)
{
    const Lock lock(m_mutex);
    // Add async cleanup callbacks to the back of the list. Unlike the sync
    // cleanup list, this list order is more significant because it determines
    // the order server objects are destroyed when there is a sudden disconnect,
    // and it is possible objects may need to be destroyed in a certain order.
    // This function is called in ProxyServerBase destructors, and since capnp
    // destroys ProxyServer objects in LIFO order, we should preserve this
    // order, and add cleanup callbacks to the end of the list so they can be
    // run starting from the beginning of the list.
    //
    // In bitcoin core, running these callbacks in the right order is
    // particularly important for the wallet process, because it uses blocking
    // shared_ptrs and requires Chain::Notification pointers owned by the node
    // process to be destroyed before the WalletLoader objects owned by the node
    // process, otherwise shared pointer counts of the CWallet objects (which
    // inherit from Chain::Notification) will not be 1 when WalletLoader
    // destructor runs and it will wait forever for them to be released.
    m_async_fns->emplace_back(std::move(fn));
    startAsyncThread();
}

EventLoop::EventLoop(const char* exe_name, LogFn log_fn, void* context)
    : m_exe_name(exe_name),
      m_io_context(kj::setupAsyncIo()),
      m_task_set(new kj::TaskSet(m_error_handler)),
      m_context(context)
{
    m_log_opts.log_fn = log_fn;
    int fds[2];
    KJ_SYSCALL(socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    m_wait_fd = fds[0];
    m_post_fd = fds[1];
}

EventLoop::~EventLoop()
{
    if (m_async_thread.joinable()) m_async_thread.join();
    const Lock lock(m_mutex);
    KJ_ASSERT(m_post_fn == nullptr);
    KJ_ASSERT(!m_async_fns);
    KJ_ASSERT(m_wait_fd == -1);
    KJ_ASSERT(m_post_fd == -1);
    KJ_ASSERT(m_num_clients == 0);

    // Spin event loop. wait for any promises triggered by RPC shutdown.
    // auto cleanup = kj::evalLater([]{});
    // cleanup.wait(m_io_context.waitScope);
}

void EventLoop::loop()
{
    assert(!g_thread_context.loop_thread);
    g_thread_context.loop_thread = true;
    KJ_DEFER(g_thread_context.loop_thread = false);

    {
        const Lock lock(m_mutex);
        assert(!m_async_fns);
        m_async_fns.emplace();
    }

    kj::Own<kj::AsyncIoStream> wait_stream{
        m_io_context.lowLevelProvider->wrapSocketFd(m_wait_fd, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP)};
    int post_fd{m_post_fd};
    char buffer = 0;
    for (;;) {
        const size_t read_bytes = wait_stream->read(&buffer, 0, 1).wait(m_io_context.waitScope);
        if (read_bytes != 1) throw std::logic_error("EventLoop wait_stream closed unexpectedly");
        Lock lock(m_mutex);
        if (m_post_fn) {
            Unlock(lock, *m_post_fn);
            m_post_fn = nullptr;
            m_cv.notify_all();
        } else if (done()) {
            // Intentionally do not break if m_post_fn was set, even if done()
            // would return true, to ensure that the EventLoopRef write(post_fd)
            // call always succeeds and the loop does not exit between the time
            // that the done condition is set and the write call is made.
            break;
        }
    }
    log() << "EventLoop::loop done, cancelling event listeners.";
    m_task_set.reset();
    log() << "EventLoop::loop bye.";
    wait_stream = nullptr;
    KJ_SYSCALL(::close(post_fd));
    const Lock lock(m_mutex);
    m_wait_fd = -1;
    m_post_fd = -1;
    m_async_fns.reset();
    m_cv.notify_all();
}

void EventLoop::post(kj::Function<void()> fn)
{
    if (std::this_thread::get_id() == m_thread_id) {
        fn();
        return;
    }
    Lock lock(m_mutex);
    EventLoopRef ref(*this, &lock);
    m_cv.wait(lock.m_lock, [this]() MP_REQUIRES(m_mutex) { return m_post_fn == nullptr; });
    m_post_fn = &fn;
    int post_fd{m_post_fd};
    Unlock(lock, [&] {
        char buffer = 0;
        KJ_SYSCALL(write(post_fd, &buffer, 1));
    });
    m_cv.wait(lock.m_lock, [this, &fn]() MP_REQUIRES(m_mutex) { return m_post_fn != &fn; });
}

void EventLoop::startAsyncThread()
{
    assert (std::this_thread::get_id() == m_thread_id);
    if (m_async_thread.joinable()) {
        // Notify to wake up the async thread if it is already running.
        m_cv.notify_all();
    } else if (!m_async_fns->empty()) {
        m_async_thread = std::thread([this] {
            Lock lock(m_mutex);
            while (m_async_fns) {
                if (!m_async_fns->empty()) {
                    EventLoopRef ref{*this, &lock};
                    const std::function<void()> fn = std::move(m_async_fns->front());
                    m_async_fns->pop_front();
                    Unlock(lock, fn);
                    // Important to relock because of the wait() call below.
                    ref.reset(/*relock=*/true);
                    // Continue without waiting in case there are more async_fns
                    continue;
                }
                m_cv.wait(lock.m_lock);
            }
        });
    }
}

bool EventLoop::done() const
{
    assert(m_num_clients >= 0);
    return m_num_clients == 0 && m_async_fns->empty();
}

std::tuple<ConnThread, bool> SetThread(GuardedRef<ConnThreads> threads, Connection* connection, const std::function<Thread::Client()>& make_thread)
{
    assert(std::this_thread::get_id() == connection->m_loop->m_thread_id);
    ConnThread thread;
    bool inserted;
    {
        const Lock lock(threads.mutex);
        std::tie(thread, inserted) = threads.ref.try_emplace(connection);
    }
    if (inserted) {
        thread->second.emplace(make_thread(), connection, /* destroy_connection= */ false);
        thread->second->m_disconnect_cb = connection->addSyncCleanup([threads, thread] {
            // Note: it is safe to use the `thread` iterator in this cleanup
            // function, because the iterator would only be invalid if the map entry
            // was removed, and if the map entry is removed the ProxyClient<Thread>
            // destructor unregisters the cleanup.

            // Connection is being destroyed before thread client is, so reset
            // thread client m_disconnect_cb member so thread client destructor does not
            // try to unregister this callback after connection is destroyed.
            thread->second->m_disconnect_cb.reset();

            // Remove connection pointer about to be destroyed from the map
            const Lock lock(threads.mutex);
            threads.ref.erase(thread);
        });
    }
    return {thread, inserted};
}

ProxyClient<Thread>::~ProxyClient()
{
    // If thread is being destroyed before connection is destroyed, remove the
    // cleanup callback that was registered to handle the connection being
    // destroyed before the thread being destroyed.
    if (m_disconnect_cb) {
        // Remove disconnect callback on the event loop thread with
        // loop->sync(), so if the connection is broken there is not a race
        // between this thread trying to remove the callback and the disconnect
        // handler attempting to call it.
        m_context.loop->sync([&]() {
            if (m_disconnect_cb) {
                m_context.connection->removeSyncCleanup(*m_disconnect_cb);
            }
        });
    }
}

ProxyServer<Thread>::ProxyServer(ThreadContext& thread_context, std::thread&& thread)
    : m_thread_context(thread_context), m_thread(std::move(thread))
{
    assert(m_thread_context.waiter.get() != nullptr);
}

ProxyServer<Thread>::~ProxyServer()
{
    if (!m_thread.joinable()) return;
    // Stop async thread and wait for it to exit. Need to wait because the
    // m_thread handle needs to outlive the thread to avoid "terminate called
    // without an active exception" error. An alternative to waiting would be
    // detach the thread, but this would introduce nondeterminism which could
    // make code harder to debug or extend.
    assert(m_thread_context.waiter.get());
    std::unique_ptr<Waiter> waiter;
    {
        const Lock lock(m_thread_context.waiter->m_mutex);
        //! Reset thread context waiter pointer, as shutdown signal for done
        //! lambda passed as waiter->wait() argument in makeThread code below.
        waiter = std::move(m_thread_context.waiter);
        //! Assert waiter is idle. This destructor shouldn't be getting called if it is busy.
        assert(!waiter->m_fn);
        // Clear client maps now to avoid deadlock in m_thread.join() call
        // below. The maps contain Thread::Client objects that need to be
        // destroyed from the event loop thread (this thread), which can't
        // happen if this thread is busy calling join.
        m_thread_context.request_threads.clear();
        m_thread_context.callback_threads.clear();
        //! Ping waiter.
        waiter->m_cv.notify_all();
    }
    m_thread.join();
}

kj::Promise<void> ProxyServer<Thread>::getName(GetNameContext context)
{
    context.getResults().setResult(m_thread_context.thread_name);
    return kj::READY_NOW;
}

ProxyServer<ThreadMap>::ProxyServer(Connection& connection) : m_connection(connection) {}

kj::Promise<void> ProxyServer<ThreadMap>::makeThread(MakeThreadContext context)
{
    const std::string from = context.getParams().getName();
    std::promise<ThreadContext*> thread_context;
    std::thread thread([&thread_context, from, this]() {
        g_thread_context.thread_name = ThreadName(m_connection.m_loop->m_exe_name) + " (from " + from + ")";
        g_thread_context.waiter = std::make_unique<Waiter>();
        thread_context.set_value(&g_thread_context);
        Lock lock(g_thread_context.waiter->m_mutex);
        // Wait for shutdown signal from ProxyServer<Thread> destructor (signal
        // is just waiter getting set to null.)
        g_thread_context.waiter->wait(lock, [] { return !g_thread_context.waiter; });
    });
    auto thread_server = kj::heap<ProxyServer<Thread>>(*thread_context.get_future().get(), std::move(thread));
    auto thread_client = m_connection.m_threads.add(kj::mv(thread_server));
    context.getResults().setResult(kj::mv(thread_client));
    return kj::READY_NOW;
}

std::atomic<int> server_reqs{0};

std::string LongThreadName(const char* exe_name)
{
    return g_thread_context.thread_name.empty() ? ThreadName(exe_name) : g_thread_context.thread_name;
}

} // namespace mp
