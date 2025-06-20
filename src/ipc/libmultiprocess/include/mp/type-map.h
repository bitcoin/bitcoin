// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_MAP_H
#define MP_PROXY_TYPE_MAP_H

#include <mp/proxy-types.h>
#include <mp/type-pair.h>
#include <mp/util.h>

namespace mp {
template <typename KeyLocalType, typename ValueLocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::map<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    // FIXME dededup with vector handler above
    auto list = output.init(value.size());
    size_t i = 0;
    for (const auto& elem : value) {
        BuildField(TypeList<std::pair<KeyLocalType, ValueLocalType>>(), invoke_context,
            ListOutput<typename decltype(list)::Builds>(list, i), elem);
        ++i;
    }
}

template <typename KeyLocalType, typename ValueLocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::map<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        for (auto item : data) {
            ReadField(TypeList<std::pair<const KeyLocalType, ValueLocalType>>(), invoke_context,
                Make<ValueField>(item),
                ReadDestEmplace(
                    TypeList<std::pair<const KeyLocalType, ValueLocalType>>(), [&](auto&&... args) -> auto& {
                        return *value.emplace(std::forward<decltype(args)>(args)...).first;
                    }));
        }
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_MAP_H
