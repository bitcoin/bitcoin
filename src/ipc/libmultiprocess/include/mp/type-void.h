// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_VOID_H
#define MP_PROXY_TYPE_VOID_H

#include <mp/util.h>

namespace mp {
template <typename Value>
::capnp::Void BuildPrimitive(InvokeContext& invoke_context, Value&&, TypeList<::capnp::Void>)
{
    return {};
}

template <typename LocalType, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, ::capnp::Void, Output&& output)
{
}
} // namespace mp

#endif // MP_PROXY_TYPE_VOID_H
