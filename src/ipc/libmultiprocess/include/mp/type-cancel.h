// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_CANCEL_H
#define MP_PROXY_TYPE_CANCEL_H

#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>

#include <functional>
#include <utility>

namespace mp {
//! Called when a request is canceled.
using CancelFn = std::function<void()>;

//! Called to register a CancelFn that is called when a request is canceled.
//! On the client, calling the received CancelFn cancels the in-flight request.
//! On the server, the wrapped method receives its own CancelArg registering a
//! callback that runs if the request is canceled.
using CancelArg = std::function<void(CancelFn)>;

//! Store the caller's CancelArg so clientInvoke can pass it a function
//! canceling the call once the request is sent.
inline void CustomBuildExtraParam(TypeList<CancelArg>, ClientInvokeContext& invoke_context, CancelArg&& value)
{
    if (value) invoke_context.cancel_receiver = std::move(value);
}

//! Builds the CancelArg passed to a wrapped method. It registers one
//! callback per request, run under request_mutex on the event loop, or
//! immediately on the worker thread if the request was already canceled.
template <typename ServerContext>
CancelArg CustomReadExtraParam(TypeList<CancelArg>, ServerContext& server_context)
{
    // request_mutex is set by mp.Context's PassField overload.
    // If null, assume we're on the event loop thread, where cancellation
    // signals are dispatched.
    if (!server_context.request_mutex) {
        return [](CancelFn) {};
    }
    return [&server_context](CancelFn fn) {
        {
            const Lock lock{*server_context.request_mutex};
            if (!server_context.request_canceled) {
                server_context.cancel_fn = std::move(fn);
                return;
            }
        }
        fn();
    };
}
} // namespace mp

#endif // MP_PROXY_TYPE_CANCEL_H
