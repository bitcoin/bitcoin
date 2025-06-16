// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_PAIR_H
#define MP_PROXY_TYPE_PAIR_H

#include <mp/util.h>

namespace mp {
// FIXME: Overload on output type instead of value type and switch to std::get and merge with next overload
template <typename KeyLocalType, typename ValueLocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::pair<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    auto pair = output.init();
    using Accessors = typename ProxyStruct<typename decltype(pair)::Builds>::Accessors;
    BuildField(TypeList<KeyLocalType>(), invoke_context, Make<StructField, std::tuple_element_t<0, Accessors>>(pair), value.first);
    BuildField(TypeList<ValueLocalType>(), invoke_context, Make<StructField, std::tuple_element_t<1, Accessors>>(pair), value.second);
}

template <typename KeyLocalType, typename ValueLocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::pair<KeyLocalType, ValueLocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    const auto& pair = input.get();
    using Accessors = typename ProxyStruct<typename Decay<decltype(pair)>::Reads>::Accessors;

    ReadField(TypeList<KeyLocalType>(), invoke_context, Make<StructField, std::tuple_element_t<0, Accessors>>(pair),
        ReadDestEmplace(TypeList<KeyLocalType>(), [&](auto&&... key_args) -> auto& {
            KeyLocalType* key = nullptr;
            ReadField(TypeList<ValueLocalType>(), invoke_context, Make<StructField, std::tuple_element_t<1, Accessors>>(pair),
                ReadDestEmplace(TypeList<ValueLocalType>(), [&](auto&&... value_args) -> auto& {
                    auto& ret = read_dest.construct(std::piecewise_construct, std::forward_as_tuple(key_args...),
                        std::forward_as_tuple(value_args...));
                    key = &ret.first;
                    return ret.second;
                }));
            return *key;
        }));
}
} // namespace mp

#endif // MP_PROXY_TYPE_PAIR_H
