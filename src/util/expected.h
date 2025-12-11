// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_UTIL_EXPECTED_H
#define BITCOIN_UTIL_EXPECTED_H

#include <attributes.h>
#include <util/check.h>

#include <cassert>
#include <exception>
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
    std::variant<T, E> m_data;

public:
    constexpr Expected() : m_data{std::in_place_index<0>, T{}} {}
    constexpr Expected(T v) : m_data{std::in_place_index<0>, std::move(v)} {}
    template <class Err>
    constexpr Expected(Unexpected<Err> u) : m_data{std::in_place_index<1>, std::move(u).error()}
    {
    }

    constexpr bool has_value() const noexcept { return m_data.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr const T& value() const& LIFETIMEBOUND
    {
        if (!has_value()) {
            throw BadExpectedAccess{};
        }
        return std::get<0>(m_data);
    }
    constexpr T& value() & LIFETIMEBOUND
    {
        if (!has_value()) {
            throw BadExpectedAccess{};
        }
        return std::get<0>(m_data);
    }
    constexpr T&& value() && LIFETIMEBOUND { return std::move(value()); }

    template <class U>
    T value_or(U&& default_value) const&
    {
        return has_value() ? value() : std::forward<U>(default_value);
    }
    template <class U>
    T value_or(U&& default_value) &&
    {
        return has_value() ? std::move(value()) : std::forward<U>(default_value);
    }

    constexpr const E& error() const& noexcept LIFETIMEBOUND { return *Assert(std::get_if<1>(&m_data)); }
    constexpr E& error() & noexcept LIFETIMEBOUND { return *Assert(std::get_if<1>(&m_data)); }
    constexpr E&& error() && noexcept LIFETIMEBOUND { return std::move(error()); }

    constexpr void swap(Expected& other) noexcept { m_data.swap(other.m_data); }

    constexpr T& operator*() & noexcept LIFETIMEBOUND { return value(); }
    constexpr const T& operator*() const& noexcept LIFETIMEBOUND { return value(); }
    constexpr T&& operator*() && noexcept LIFETIMEBOUND { return std::move(value()); }

    constexpr T* operator->() noexcept LIFETIMEBOUND { return &value(); }
    constexpr const T* operator->() const noexcept LIFETIMEBOUND { return &value(); }
};

template <class E>
class Expected<void, E>
{
private:
    std::variant<std::monostate, E> m_data;

public:
    constexpr Expected() : m_data{std::in_place_index<0>, std::monostate{}} {}
    template <class Err>
    constexpr Expected(Unexpected<Err> u) : m_data{std::in_place_index<1>, std::move(u).error()}
    {
    }

    constexpr bool has_value() const noexcept { return m_data.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr void operator*() const noexcept { return value(); }
    constexpr void value() const
    {
        if (!has_value()) {
            throw BadExpectedAccess{};
        }
    }

    constexpr const E& error() const& noexcept LIFETIMEBOUND { return *Assert(std::get_if<1>(&m_data)); }
    constexpr E& error() & noexcept LIFETIMEBOUND { return *Assert(std::get_if<1>(&m_data)); }
    constexpr E&& error() && noexcept LIFETIMEBOUND { return std::move(error()); }
};

} // namespace util

#endif // BITCOIN_UTIL_EXPECTED_H
