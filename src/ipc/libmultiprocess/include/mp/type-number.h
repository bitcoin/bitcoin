// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_NUMBER_H
#define MP_PROXY_TYPE_NUMBER_H

#include <mp/util.h>

namespace mp {
template <typename LocalType, typename Value>
LocalType BuildPrimitive(InvokeContext& invoke_context,
    const Value& value,
    TypeList<LocalType>,
    typename std::enable_if<std::is_enum<Value>::value>::type* enable = nullptr)
{
    using E = std::make_unsigned_t<std::underlying_type_t<Value>>;
    using T = std::make_unsigned_t<LocalType>;
    static_assert(std::numeric_limits<T>::max() >= std::numeric_limits<E>::max(), "mismatched integral/enum types");
    return static_cast<LocalType>(value);
}

template <typename LocalType, typename Value>
LocalType BuildPrimitive(InvokeContext& invoke_context,
    const Value& value,
    TypeList<LocalType>,
    typename std::enable_if<std::is_integral<Value>::value, int>::type* enable = nullptr)
{
    static_assert(
        std::numeric_limits<LocalType>::lowest() <= std::numeric_limits<Value>::lowest(), "mismatched integral types");
    static_assert(
        std::numeric_limits<LocalType>::max() >= std::numeric_limits<Value>::max(), "mismatched integral types");
    return value;
}

template <typename LocalType, typename Value>
LocalType BuildPrimitive(InvokeContext& invoke_context,
    const Value& value,
    TypeList<LocalType>,
    typename std::enable_if<std::is_floating_point<Value>::value>::type* enable = nullptr)
{
    static_assert(std::is_same<Value, LocalType>::value,
        "mismatched floating point types. please fix message.capnp type declaration to match wrapped interface");
    return value;
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename std::enable_if<std::is_enum<LocalType>::value>::type* enable = nullptr)
{
    // Disable clang-tidy out-of-range enum value check which triggers when
    // using an enum type that does not have a 0 value. The check correctly
    // triggers when it detects that Cap'n Proto returns 0 when reading an
    // integer field that is unset. But the warning is spurious because the
    // corresponding BuildField call should never leave the field unset.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return read_dest.construct(static_cast<LocalType>(input.get()));
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename std::enable_if<std::is_integral<LocalType>::value>::type* enable = nullptr)
{
    auto value = input.get();
    if (value < std::numeric_limits<LocalType>::min() || value > std::numeric_limits<LocalType>::max()) {
        throw std::range_error("out of bound int received");
    }
    return read_dest.construct(static_cast<LocalType>(value));
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename std::enable_if<std::is_floating_point<LocalType>::value>::type* enable = 0)
{
    auto value = input.get();
    static_assert(std::is_same<LocalType, decltype(value)>::value, "floating point type mismatch");
    return read_dest.construct(value);
}
} // namespace mp

#endif // MP_PROXY_TYPE_NUMBER_H
