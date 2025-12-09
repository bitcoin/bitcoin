// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_UTIL_EXPECTED_H
#define BITCOIN_UTIL_EXPECTED_H

#include <attributes.h>

#include <cassert>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace util {

/// The util::Unexpected class represents an unexpected value stored in
/// util::Expected.
template <class E>
class Unexpected
{
public:
    constexpr explicit Unexpected(E e) : m_error(std::move(e)) {}

    constexpr const E& error() const& noexcept LIFETIMEBOUND { return m_error; }
    constexpr E& error() & noexcept LIFETIMEBOUND { return m_error; }
    constexpr E&& error() && noexcept LIFETIMEBOUND { return std::move(m_error); }

private:
    E m_error;
};

struct BadExpectedAccess : std::exception {
    const char* what() const noexcept override { return "Bad util::Expected access"; }
};

/// The util::Expected class provides a standard way for low-level functions to
/// return either error values or result values.
///
/// It provides a smaller version of std::expected from C++23. Missing features
/// can be added, if needed.
template <class T, class E>
class Expected
{
private:
    using ValueType = std::conditional_t<std::is_same_v<T, void>, std::monostate, T>;
    std::variant<ValueType, E> m_data;

public:
    constexpr Expected() : m_data{std::in_place_index_t<0>{}, ValueType{}} {}
    constexpr Expected(ValueType v) : m_data{std::in_place_index_t<0>{}, std::move(v)} {}
    template <class Err>
    constexpr Expected(Unexpected<Err> u) : m_data{std::in_place_index_t<1>{}, std::move(u).error()}
    {
    }

    constexpr bool has_value() const noexcept { return m_data.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr const ValueType& value() const LIFETIMEBOUND
    {
        if (!has_value()) {
            throw BadExpectedAccess{};
        }
        return std::get<0>(m_data);
    }
    constexpr ValueType& value() LIFETIMEBOUND
    {
        if (!has_value()) {
            throw BadExpectedAccess{};
        }
        return std::get<0>(m_data);
    }

    template <class U>
    ValueType value_or(U&& default_value) const&
    {
        return has_value() ? value() : std::forward<U>(default_value);
    }
    template <class U>
    ValueType value_or(U&& default_value) &&
    {
        return has_value() ? std::move(value()) : std::forward<U>(default_value);
    }

    constexpr const E& error() const LIFETIMEBOUND
    {
        assert(!has_value());
        return std::get<1>(m_data);
    }
    constexpr E& error() LIFETIMEBOUND
    {
        assert(!has_value());
        return std::get<1>(m_data);
    }

    constexpr ValueType& operator*() noexcept LIFETIMEBOUND { return value(); }
    constexpr const ValueType& operator*() const noexcept LIFETIMEBOUND { return value(); }

    constexpr ValueType* operator->() noexcept LIFETIMEBOUND { return &value(); }
    constexpr const ValueType* operator->() const noexcept LIFETIMEBOUND { return &value(); }
};

} // namespace util

#endif // BITCOIN_UTIL_EXPECTED_H
