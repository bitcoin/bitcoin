// Copyright (c) The Bitcoin Core developers
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
    kj::Promise<void> makePool(MakePoolContext context) override;
    Connection& m_connection;
};

template <typename Output>
    requires FieldTypeIs<Output, ThreadMap::Client>
void CustomBuildField(TypeList<>,
    Priority<1>,
    InvokeContext& invoke_context,
    Output&& output)
{
    output.set(kj::heap<ProxyServer<ThreadMap>>(invoke_context.connection));
}

template <typename Input>
    requires FieldTypeIs<Input, ThreadMap::Client>
decltype(auto) CustomReadField(TypeList<>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input)
{
    invoke_context.connection.m_thread_map = input.get();
}
} // namespace mp

#endif // MP_PROXY_TYPE_THREADMAP_H
