// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_CONTEXT_H
#define MP_PROXY_TYPE_CONTEXT_H

#include <mp/proxy-io.h>
#include <mp/util.h>

namespace mp {
template <typename Output>
void CustomBuildField(TypeList<>,
    Priority<1>,
    ClientInvokeContext& invoke_context,
    Output&& output,
    typename std::enable_if<std::is_same<decltype(output.get()), Context::Builder>::value>::type* enable = nullptr)
{
    auto& connection = invoke_context.connection;
    auto& thread_context = invoke_context.thread_context;

    // Create local Thread::Server object corresponding to the current thread
    // and pass a Thread::Client reference to it in the Context.callbackThread
    // field so the function being called can make callbacks to this thread.
    // Also store the Thread::Client reference in the callback_threads map so
    // future calls over this connection can reuse it.
    auto [callback_thread, _]{SetThread(
        GuardedRef{thread_context.waiter->m_mutex, thread_context.callback_threads}, &connection,
        [&] { return connection.m_threads.add(kj::heap<ProxyServer<Thread>>(thread_context, std::thread{})); })};

    // Call remote ThreadMap.makeThread function so server will create a
    // dedicated worker thread to run function calls from this thread. Store the
    // Thread::Client reference it returns in the request_threads map.
    auto make_request_thread{[&]{
        // This code will only run if an IPC client call is being made for the
        // first time on this thread. After the first call, subsequent calls
        // will use the existing request thread. This code will also never run at
        // all if the current thread is a request thread created for a different
        // IPC client, because in that case PassField code (below) will have set
        // request_thread to point to the calling thread.
        auto request = connection.m_thread_map.makeThreadRequest();
        request.setName(thread_context.thread_name);
        return request.send().getResult(); // Nonblocking due to capnp request pipelining.
    }};
    auto [request_thread, _1]{SetThread(
        GuardedRef{thread_context.waiter->m_mutex, thread_context.request_threads},
        &connection, make_request_thread)};

    auto context = output.init();
    context.setThread(request_thread->second->m_client);
    context.setCallbackThread(callback_thread->second->m_client);
}

//! PassField override for mp.Context arguments. Return asynchronously and call
//! function on other thread found in context.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
auto PassField(Priority<1>, TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args) ->
    typename std::enable_if<
        std::is_same<decltype(Accessor::get(server_context.call_context.getParams())), Context::Reader>::value,
        kj::Promise<typename ServerContext::CallContext>>::type
{
    const auto& params = server_context.call_context.getParams();
    Context::Reader context_arg = Accessor::get(params);
    auto future = kj::newPromiseAndFulfiller<typename ServerContext::CallContext>();
    auto& server = server_context.proxy_server;
    int req = server_context.req;
    auto invoke = [fulfiller = kj::mv(future.fulfiller),
         call_context = kj::mv(server_context.call_context), &server, req, fn, args...]() mutable {
                const auto& params = call_context.getParams();
                Context::Reader context_arg = Accessor::get(params);
                ServerContext server_context{server, call_context, req};
                {
                    // Before invoking the function, store a reference to the
                    // callbackThread provided by the client in the
                    // thread_local.request_threads map. This way, if this
                    // server thread needs to execute any RPCs that call back to
                    // the client, they will happen on the same client thread
                    // that is waiting for this function, just like what would
                    // happen if this were a normal function call made on the
                    // local stack.
                    //
                    // If the request_threads map already has an entry for this
                    // connection, it will be left unchanged, and it indicates
                    // that the current thread is an RPC client thread which is
                    // in the middle of an RPC call, and the current RPC call is
                    // a nested call from the remote thread handling that RPC
                    // call. In this case, the callbackThread value should point
                    // to the same thread already in the map, so there is no
                    // need to update the map.
                    auto& thread_context = g_thread_context;
                    auto& request_threads = thread_context.request_threads;
                    ConnThread request_thread;
                    bool inserted;
                    server.m_context.loop->sync([&] {
                        std::tie(request_thread, inserted) = SetThread(
                            GuardedRef{thread_context.waiter->m_mutex, request_threads}, server.m_context.connection,
                            [&] { return context_arg.getCallbackThread(); });
                    });

                    // If an entry was inserted into the request_threads map,
                    // remove it after calling fn.invoke. If an entry was not
                    // inserted, one already existed, meaning this must be a
                    // recursive call (IPC call calling back to the caller which
                    // makes another IPC call), so avoid modifying the map.
                    const bool erase_thread{inserted};
                    KJ_DEFER(if (erase_thread) {
                        // Erase the request_threads entry on the event loop
                        // thread with loop->sync(), so if the connection is
                        // broken there is not a race between this thread and
                        // the disconnect handler trying to destroy the thread
                        // client object.
                        server.m_context.loop->sync([&] {
                            // Look up the thread again without using existing
                            // iterator since entry may no longer be there after
                            // a disconnect. Destroy node after releasing
                            // Waiter::m_mutex, so the ProxyClient<Thread>
                            // destructor is able to use EventLoop::mutex
                            // without violating lock order.
                            ConnThreads::node_type removed;
                            {
                                Lock lock(thread_context.waiter->m_mutex);
                                removed = request_threads.extract(server.m_context.connection);
                            }
                        });
                    });
                    fn.invoke(server_context, args...);
                }
                KJ_IF_MAYBE(exception, kj::runCatchingExceptions([&]() {
                    server.m_context.loop->sync([&] {
                        auto fulfiller_dispose = kj::mv(fulfiller);
                        fulfiller_dispose->fulfill(kj::mv(call_context));
                    });
                }))
                {
                    server.m_context.loop->sync([&]() {
                        auto fulfiller_dispose = kj::mv(fulfiller);
                        fulfiller_dispose->reject(kj::mv(*exception));
                    });
                }
            };

    // Lookup Thread object specified by the client. The specified thread should
    // be a local Thread::Server object, but it needs to be looked up
    // asynchronously with getLocalServer().
    auto thread_client = context_arg.getThread();
    return server.m_context.connection->m_threads.getLocalServer(thread_client)
        .then([&server, invoke = kj::mv(invoke), req](const kj::Maybe<Thread::Server&>& perhaps) mutable {
            // Assuming the thread object is found, pass it a pointer to the
            // `invoke` lambda above which will invoke the function on that
            // thread.
            KJ_IF_MAYBE (thread_server, perhaps) {
                const auto& thread = static_cast<ProxyServer<Thread>&>(*thread_server);
                MP_LOG(*server.m_context.loop, Log::Debug)
                    << "IPC server post request  #" << req << " {" << thread.m_thread_context.thread_name << "}";
                if (!thread.m_thread_context.waiter->post(std::move(invoke))) {
                    MP_LOG(*server.m_context.loop, Log::Error)
                        << "IPC server error request #" << req
                        << " {" << thread.m_thread_context.thread_name << "}" << ", thread busy";
                    throw std::runtime_error("thread busy");
                }
            } else {
                MP_LOG(*server.m_context.loop, Log::Error)
                    << "IPC server error request #" << req << ", missing thread to execute request";
                throw std::runtime_error("invalid thread handle");
            }
        })
        // Wait for the invocation to finish before returning to the caller.
        .then([invoke_wait = kj::mv(future.promise)]() mutable { return kj::mv(invoke_wait); });
}
} // namespace mp

#endif // MP_PROXY_TYPE_CONTEXT_H
