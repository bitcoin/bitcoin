// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_STRUCT_H
#define MP_PROXY_TYPE_STRUCT_H

#include <mp/util.h>

namespace mp {
template <size_t index, typename LocalType, typename Value, typename Output>
void BuildOne(TypeList<LocalType> param,
    InvokeContext& invoke_context,
    Output&& output,
    Value&& value,
    typename std::enable_if < index<ProxyType<LocalType>::fields>::type * enable = nullptr)
{
    using Index = std::integral_constant<size_t, index>;
    using Struct = typename ProxyType<LocalType>::Struct;
    using Accessor = typename std::tuple_element<index, typename ProxyStruct<Struct>::Accessors>::type;
    auto&& field_output = Make<StructField, Accessor>(output);
    auto&& field_value = value.*ProxyType<LocalType>::get(Index());
    BuildField(TypeList<Decay<decltype(field_value)>>(), invoke_context, field_output, field_value);
    BuildOne<index + 1>(param, invoke_context, output, value);
}

template <size_t index, typename LocalType, typename Value, typename Output>
void BuildOne(TypeList<LocalType> param,
    InvokeContext& invoke_context,
    Output&& output,
    Value&& value,
    typename std::enable_if<index == ProxyType<LocalType>::fields>::type* enable = nullptr)
{
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType> local_type,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename ProxyType<LocalType>::Struct* enable = nullptr)
{
    BuildOne<0>(local_type, invoke_context, output.init(), value);
}

template <size_t index, typename LocalType, typename Input, typename Value>
void ReadOne(TypeList<LocalType> param,
    InvokeContext& invoke_context,
    Input&& input,
    Value&& value,
    typename std::enable_if<index != ProxyType<LocalType>::fields>::type* enable = nullptr)
{
    using Index = std::integral_constant<size_t, index>;
    using Struct = typename ProxyType<LocalType>::Struct;
    using Accessor = typename std::tuple_element<index, typename ProxyStruct<Struct>::Accessors>::type;
    const auto& struc = input.get();
    auto&& field_value = value.*ProxyType<LocalType>::get(Index());
    ReadField(TypeList<RemoveCvRef<decltype(field_value)>>(), invoke_context, Make<StructField, Accessor>(struc),
        ReadDestUpdate(field_value));
    ReadOne<index + 1>(param, invoke_context, input, value);
}

template <size_t index, typename LocalType, typename Input, typename Value>
void ReadOne(TypeList<LocalType> param,
    InvokeContext& invoke_context,
    Input& input,
    Value& value,
    typename std::enable_if<index == ProxyType<LocalType>::fields>::type* enable = nullptr)
{
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType> param,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename ProxyType<LocalType>::Struct* enable = nullptr)
{
    return read_dest.update([&](auto& value) { ReadOne<0>(param, invoke_context, input, value); });
}
} // namespace mp

#endif // MP_PROXY_TYPE_STRUCT_H
