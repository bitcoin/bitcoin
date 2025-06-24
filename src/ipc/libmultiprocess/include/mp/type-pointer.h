// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_POINTER_H
#define MP_PROXY_TYPE_POINTER_H

#include <mp/util.h>

namespace mp {
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType*>, Priority<3>, InvokeContext& invoke_context, Value&& value, Output&& output)
{
    if (value) {
        BuildField(TypeList<LocalType>(), invoke_context, output, *value);
    }
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::shared_ptr<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    if (value) {
        BuildField(TypeList<LocalType>(), invoke_context, output, *value);
    }
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType*>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        if (value) {
            ReadField(TypeList<LocalType>(), invoke_context, std::forward<Input>(input), ReadDestUpdate(*value));
        }
    });
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::shared_ptr<LocalType>>,
    Priority<0>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) {
            value.reset();
        } else if (value) {
            ReadField(TypeList<LocalType>(), invoke_context, input, ReadDestUpdate(*value));
        } else {
            ReadField(TypeList<LocalType>(), invoke_context, input,
                ReadDestEmplace(TypeList<LocalType>(), [&](auto&&... args) -> auto& {
                    value = std::make_shared<LocalType>(std::forward<decltype(args)>(args)...);
                    return *value;
                }));
        }
    });
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::shared_ptr<const LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) {
            value.reset();
            return;
        }
        ReadField(TypeList<LocalType>(), invoke_context, std::forward<Input>(input),
            ReadDestEmplace(TypeList<LocalType>(), [&](auto&&... args) -> auto& {
                value = std::make_shared<LocalType>(std::forward<decltype(args)>(args)...);
                return *value;
            }));
    });
}

//! PassField override for C++ pointer arguments.
template <typename Accessor, typename LocalType, typename ServerContext, typename Fn, typename... Args>
void PassField(Priority<1>, TypeList<LocalType*>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    const auto& params = server_context.call_context.getParams();
    const auto& input = Make<StructField, Accessor>(params);

    if (!input.want()) {
        fn.invoke(server_context, std::forward<Args>(args)..., nullptr);
        return;
    }

    InvokeContext& invoke_context = server_context;
    Decay<LocalType> param;

    MaybeReadField(std::integral_constant<bool, Accessor::in>(), TypeList<LocalType>(), invoke_context, input,
        ReadDestUpdate(param));

    fn.invoke(server_context, std::forward<Args>(args)..., &param);

    auto&& results = server_context.call_context.getResults();
    MaybeBuildField(std::integral_constant<bool, Accessor::out>(), TypeList<LocalType>(), invoke_context,
        Make<StructField, Accessor>(results), param);
}
} // namespace mp

#endif // MP_PROXY_TYPE_POINTER_H
