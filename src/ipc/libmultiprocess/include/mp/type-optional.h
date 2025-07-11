// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_OPTIONAL_H
#define MP_PROXY_TYPE_OPTIONAL_H

#include <mp/util.h>

namespace mp {
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<std::optional<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    if (value) {
        output.setHas();
        // FIXME: should std::move value if destvalue is rref?
        BuildField(TypeList<LocalType>(), invoke_context, output, *value);
    }
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::optional<LocalType>>,
    Priority<1>,
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
                    value.emplace(std::forward<decltype(args)>(args)...);
                    return *value;
                }));
        }
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_OPTIONAL_H
