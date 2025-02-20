// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_VECTOR_H
#define MP_PROXY_TYPE_VECTOR_H

#include <mp/proxy-types.h>
#include <mp/util.h>

namespace mp {
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::vector<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    // FIXME dedup with set handler below
    auto list = output.init(value.size());
    size_t i = 0;
    for (auto it = value.begin(); it != value.end(); ++it, ++i) {
        BuildField(TypeList<LocalType>(), invoke_context, ListOutput<typename decltype(list)::Builds>(list, i), *it);
    }
}

inline static bool BuildPrimitive(InvokeContext& invoke_context, std::vector<bool>::const_reference value, TypeList<bool>)
{
    return value;
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::vector<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        value.reserve(data.size());
        for (auto item : data) {
            ReadField(TypeList<LocalType>(), invoke_context, Make<ValueField>(item),
                ReadDestEmplace(TypeList<LocalType>(), [&](auto&&... args) -> auto& {
                    value.emplace_back(std::forward<decltype(args)>(args)...);
                    return value.back();
                }));
        }
    });
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::vector<bool>>,
                               Priority<1>,
                               InvokeContext& invoke_context,
                               Input&& input,
                               ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        value.reserve(data.size());
        for (auto item : data) {
            value.push_back(ReadField(TypeList<bool>(), invoke_context, Make<ValueField>(item), ReadDestTemp<bool>()));
        }
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_VECTOR_H
