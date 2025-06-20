// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_THREADMAP_H
#define MP_PROXY_TYPE_THREADMAP_H

#include <mp/util.h>

namespace mp {
template <>
struct ProxyServer<ThreadMap> final : public virtual ThreadMap::Server
{
public:
    ProxyServer(Connection& connection);
    kj::Promise<void> makeThread(MakeThreadContext context) override;
    Connection& m_connection;
};

template <typename Output>
void CustomBuildField(TypeList<>,
    Priority<1>,
    InvokeContext& invoke_context,
    Output&& output,
    typename std::enable_if<std::is_same<decltype(output.get()), ThreadMap::Client>::value>::type* enable = nullptr)
{
    output.set(kj::heap<ProxyServer<ThreadMap>>(invoke_context.connection));
}

template <typename Input>
decltype(auto) CustomReadField(TypeList<>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    typename std::enable_if<std::is_same<decltype(input.get()), ThreadMap::Client>::value>::type* enable = nullptr)
{
    invoke_context.connection.m_thread_map = input.get();
}
} // namespace mp

#endif // MP_PROXY_TYPE_THREADMAP_H
