// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_DATA_H
#define MP_PROXY_TYPE_DATA_H

#include <mp/util.h>

namespace mp {
template <typename T, typename U>
concept IsSpanOf =
    std::convertible_to<T, std::span<const U>> &&
    std::constructible_from<T, const U*, const U*>;

template <typename T>
concept IsByteSpan =
    IsSpanOf<T, std::byte> ||
    IsSpanOf<T, char> ||
    IsSpanOf<T, unsigned char> ||
    IsSpanOf<T, signed char>;

//! Generic ::capnp::Data field builder for any C++ type that can be converted
//! to a span of bytes, like std::vector<char> or std::array<uint8_t>, or custom
//! blob types like uint256 or PKHash with data() and size() methods pointing to
//! bytes.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output)
requires (std::is_same_v<decltype(output.get()), ::capnp::Data::Builder> && IsByteSpan<LocalType>)
{
    auto data = std::span{value};
    auto result = output.init(data.size());
    memcpy(result.begin(), data.data(), data.size());
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
requires (std::is_same_v<decltype(input.get()), ::capnp::Data::Reader> && IsByteSpan<LocalType>)
{
    using ByteType = decltype(std::span{std::declval<LocalType>().begin(), std::declval<LocalType>().end()})::element_type;
    const kj::byte *begin{input.get().begin()}, *end{input.get().end()};
    return read_dest.construct(reinterpret_cast<const ByteType*>(begin), reinterpret_cast<const ByteType*>(end));
}
} // namespace mp

#endif // MP_PROXY_TYPE_DATA_H
