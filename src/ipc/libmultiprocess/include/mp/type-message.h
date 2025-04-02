// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_MESSAGE_H
#define MP_PROXY_TYPE_MESSAGE_H

#include <mp/util.h>

namespace mp {
//! Overload CustomBuildField to serialize objects that have CustomBuildMessage
//! overloads. Defining a CustomBuildMessage overload is simpler than defining a
//! CustomBuildField overload because it only requires defining a normal
//! function, not a template function, but less flexible.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
                      decltype(CustomBuildMessage(invoke_context, value, std::move(output.get())))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, std::move(output.init()));
}

//! Overload CustomReadField to serialize objects that have CustomReadMessage
//! overloads. Defining a CustomReadMessage overload is simpler than defining a
//! CustomReadField overload because it only requires defining a normal
//! function, not a template function, but less flexible.
template <typename LocalType, typename Reader, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Reader&& reader,
                               ReadDest&& read_dest,
                               decltype(CustomReadMessage(invoke_context, reader.get(),
                                                          std::declval<LocalType&>()))* enable = nullptr)
{
    return read_dest.update([&](auto& value) { if (reader.has()) CustomReadMessage(invoke_context, reader.get(), value); });
}

//! Helper for CustomPassField below. Call Accessor::init method if it has one,
//! otherwise do nothing.
template <typename Accessor, typename Message>
decltype(auto) MaybeInit(Message&& message, decltype(Accessor::get(message))* enable = nullptr)
{
    return Accessor::init(message);
}

template <typename Accessor>
::capnp::Void MaybeInit(...)
{
    return {};
}

//! Overload CustomPassField to serialize objects that have CustomPassMessage
//! overloads. Defining a CustomPassMessage overload is simpler than defining a
//! CustomPassField overload because it only requires defining a normal
//! function, not a template function, but less flexible.
template <typename Accessor, typename... LocalTypes, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<LocalTypes...>, ServerContext& server_context, Fn&& fn, Args&&... args)
    -> decltype(CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                                  MaybeGet<Accessor>(server_context.call_context.getResults()), nullptr))
{
    CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                      MaybeInit<Accessor>(server_context.call_context.getResults()),
                      [&](LocalTypes... param) { fn.invoke(server_context, std::forward<Args>(args)..., param...); });
}
} // namespace mp

#endif // MP_PROXY_TYPE_MESSAGE_H
