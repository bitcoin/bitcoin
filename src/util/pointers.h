// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_UTIL_POINTERS_H
#define BITCOIN_UTIL_POINTERS_H

#include <util/check.h>

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace util {

/// NotNull is intended to be used to denote that a smart pointer type can not
/// hold a nullptr value.
///
/// The type is more useful, the earlier in the lifetime of a pointer it is
/// used. So instead of passing an existing pointer into NotNull, it is
/// recommended to use the type when creating the smart pointer. E.g.:
///
/// util::NotNull p{std::make_shared<Thing>()};
///
/// This makes it obvious that a pointer really is not null and that the
/// constructor will not fail.
///
/// NotNull can *not* be used to denote a raw pointer can not be nullptr.
/// The C++ language provides raw references for this use case, and the C++
/// standard library provides std::reference_wrapper, where raw references can
/// not be used.
///
/// This type is movable, meaning that the inner pointer *is* null after a
/// move. Clang-tidy static analysis is used to enforce use-after-move
/// violations, so that the null can never be observed. Moreover, the runtime
/// Assume check in the get() member function will ensure the same.
template <class T>
    requires(!std::is_pointer_v<T>) && requires(T t) { { t != nullptr } -> std::convertible_to<bool>; }
class NotNull
{
public:
    template <std::convertible_to<T> U>
    constexpr explicit NotNull(U&& u) noexcept(std::is_nothrow_constructible_v<T, U&&>)
        : m_ptr(std::forward<U>(u))
    {
        Assert(m_ptr != nullptr);
    }

    template <std::convertible_to<T> U>
    constexpr NotNull(NotNull<U>&& other) noexcept(std::is_nothrow_constructible_v<T, U&&>)
        : m_ptr(std::move(other).get())
    {
    }

    template <std::convertible_to<T> U>
    constexpr NotNull(const NotNull<U>& other) noexcept(std::is_nothrow_constructible_v<T, const U&>)
        : m_ptr(other.get())
    {
    }

    NotNull(std::nullptr_t) = delete;
    NotNull& operator=(std::nullptr_t) = delete;

    constexpr NotNull(NotNull&&) = default;
    constexpr NotNull& operator=(NotNull&&) = default;
    constexpr NotNull(const NotNull&) = default;
    constexpr NotNull& operator=(const NotNull&) = default;

    constexpr const T& get() const& noexcept { return Assume(m_ptr); }
    constexpr T&& get() && noexcept { return std::move(Assume(m_ptr)); }

    constexpr decltype(auto) operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }

    constexpr operator T() const& { return get(); }
    constexpr operator T() && noexcept { return std::move(*this).get(); }

    void swap(NotNull& other) noexcept { std::swap(m_ptr, other.m_ptr); }

private:
    T m_ptr;
};

template <class T>
NotNull(T) -> NotNull<T>;

template <class T>
void swap(NotNull<T>& a, NotNull<T>& b) noexcept
{
    a.swap(b);
}

template <class T, class U>
constexpr auto operator<=>(const NotNull<T>& a, const NotNull<U>& b)
{
    return std::compare_three_way{}(a.get(), b.get());
}

template <class T, class U>
constexpr bool operator==(const NotNull<T>& a, const NotNull<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename Deleter = std::default_delete<T>>
using NotNullUniquePtr = NotNull<std::unique_ptr<T, Deleter>>;

template <typename T>
using NotNullSharedPtr = NotNull<std::shared_ptr<T>>;

} // namespace util

template <class T>
struct std::hash<util::NotNull<T>> {
    std::size_t operator()(const util::NotNull<T>& v) const noexcept
    {
        return std::hash<T>{}(v.get());
    }
};

#endif // BITCOIN_UTIL_POINTERS_H
