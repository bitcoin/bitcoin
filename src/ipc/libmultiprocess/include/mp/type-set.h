// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_SET_H
#define MP_PROXY_TYPE_SET_H

#include <mp/proxy-types.h>
#include <mp/util.h>

namespace mp {
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::set<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    // FIXME dededup with vector handler above
    auto list = output.init(value.size());
    size_t i = 0;
    for (const auto& elem : value) {
        BuildField(TypeList<LocalType>(), invoke_context, ListOutput<typename decltype(list)::Builds>(list, i), elem);
        ++i;
    }
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::set<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        for (auto item : data) {
            ReadField(TypeList<LocalType>(), invoke_context, Make<ValueField>(item),
                ReadDestEmplace(TypeList<const LocalType>(), [&](auto&&... args) -> auto& {
                    return *value.emplace(std::forward<decltype(args)>(args)...).first;
                }));
        }
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_SET_H
