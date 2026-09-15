// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_CHAR_H
#define MP_PROXY_TYPE_CHAR_H

#include <mp/util.h>

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace mp {
template <typename Output, size_t size>
void CustomBuildField(TypeList<const unsigned char*>,
    Priority<3>,
    InvokeContext& invoke_context,
    const unsigned char (&value)[size],
    Output&& output)
{
    auto result = output.init(size);
    std::ranges::copy(value, result.begin());
}

template <size_t size, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<unsigned char[size]>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest)
{
    auto data = input.get();
    if (data.size() != size) throw std::range_error("unexpected field size");

    return read_dest.update([&](auto& value) {
        std::ranges::copy(input.get(), std::ranges::begin(value));
    });
}
} // namespace mp

#endif // MP_PROXY_TYPE_CHAR_H
