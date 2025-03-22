// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_CHAR_H
#define MP_PROXY_TYPE_CHAR_H

#include <mp/util.h>

namespace mp {
template <typename Output, size_t size>
void CustomBuildField(TypeList<const unsigned char*>,
    Priority<3>,
    InvokeContext& invoke_context,
    const unsigned char (&value)[size],
    Output&& output)
{
    auto result = output.init(size);
    memcpy(result.begin(), value, size);
}

template <size_t size, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<unsigned char[size]>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        memcpy(value, data.begin(), size);
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_CHAR_H
