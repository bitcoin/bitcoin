// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_INTERFACE_H
#define MP_PROXY_TYPE_INTERFACE_H

#include <mp/util.h>

namespace mp {
template <typename Interface, typename Impl>
kj::Own<typename Interface::Server> MakeProxyServer(InvokeContext& context, std::shared_ptr<Impl> impl)
{
    return kj::heap<ProxyServer<Interface>>(std::move(impl), context.connection);
}

template <typename Interface, typename Impl>
kj::Own<typename Interface::Server> CustomMakeProxyServer(InvokeContext& context, std::shared_ptr<Impl>&& impl)
{
    return MakeProxyServer<Interface, Impl>(context, std::move(impl));
}

template <typename Impl, typename Value, typename Output>
void CustomBuildField(TypeList<std::unique_ptr<Impl>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename Decay<decltype(output.get())>::Calls* enable = nullptr)
{
    if (value) {
        using Interface = typename decltype(output.get())::Calls;
        output.set(CustomMakeProxyServer<Interface, Impl>(invoke_context, std::shared_ptr<Impl>(value.release())));
    }
}

template <typename Impl, typename Value, typename Output>
void CustomBuildField(TypeList<std::shared_ptr<Impl>>,
    Priority<2>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename Decay<decltype(output.get())>::Calls* enable = nullptr)
{
    if (value) {
        using Interface = typename decltype(output.get())::Calls;
        output.set(CustomMakeProxyServer<Interface, Impl>(invoke_context, std::forward<Value>(value)));
    }
}

template <typename Impl, typename Output>
void CustomBuildField(TypeList<Impl&>,
    Priority<1>,
    InvokeContext& invoke_context,
    Impl& value,
    Output&& output,
    typename decltype(output.get())::Calls* enable = nullptr)
{
    // Disable deleter so proxy server object doesn't attempt to delete the
    // wrapped implementation when the proxy client is destroyed or
    // disconnected.
    using Interface = typename decltype(output.get())::Calls;
    output.set(CustomMakeProxyServer<Interface, Impl>(invoke_context, std::shared_ptr<Impl>(&value, [](Impl*){})));
}

template <typename Interface, typename Impl>
std::unique_ptr<Impl> MakeProxyClient(InvokeContext& context, typename Interface::Client&& client)
{
    return std::make_unique<ProxyClient<Interface>>(
        std::move(client), &context.connection, /* destroy_connection= */ false);
}

template <typename Interface, typename Impl>
std::unique_ptr<Impl> CustomMakeProxyClient(InvokeContext& context, typename Interface::Client&& client)
{
    return MakeProxyClient<Interface, Impl>(context, kj::mv(client));
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::unique_ptr<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename Decay<decltype(input.get())>::Calls* enable = nullptr)
{
    using Interface = typename Decay<decltype(input.get())>::Calls;
    if (input.has()) {
        return read_dest.construct(
                                   CustomMakeProxyClient<Interface, LocalType>(invoke_context, std::move(input.get())));
    }
    return read_dest.construct();
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::shared_ptr<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename Decay<decltype(input.get())>::Calls* enable = nullptr)
{
    using Interface = typename Decay<decltype(input.get())>::Calls;
    if (input.has()) {
        return read_dest.construct(
            CustomMakeProxyClient<Interface, LocalType>(invoke_context, std::move(input.get())));
    }
    return read_dest.construct();
}
} // namespace mp

#endif // MP_PROXY_TYPE_INTERFACE_H
