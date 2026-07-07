// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_UNORDERED_SET_H
#define MP_PROXY_TYPE_UNORDERED_SET_H

#include <mp/proxy-types.h>
#include <mp/util.h>
#include <unordered_set>

namespace mp {
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::unordered_set<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    BuildList(TypeList<LocalType>(), invoke_context, output, value);
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::unordered_set<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return ReadList(
        TypeList<LocalType>(), invoke_context, input, read_dest,
        [&](auto& value, size_t size) {
            value.clear();
            value.reserve(size);
        },
        [&](auto& value, auto&&... args) -> auto& {
            return *value.emplace(std::forward<decltype(args)>(args)...).first;
        });
}
} // namespace mp

#endif // MP_PROXY_TYPE_UNORDERED_SET_H