// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_STRING_H
#define MP_PROXY_TYPE_STRING_H

#include <mp/util.h>

namespace mp {
template <typename Value, typename Output>
void CustomBuildField(TypeList<std::string>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    auto result = output.init(value.size());
    memcpy(result.begin(), value.data(), value.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::string>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    auto data = input.get();
    return read_dest.construct(CharCast(data.begin()), data.size());
}
} // namespace mp

#endif // MP_PROXY_TYPE_STRING_H
