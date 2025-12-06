// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RESULT_H
#define BITCOIN_UTIL_RESULT_H

#include <attributes.h>
#include <util/translation.h>

#include <variant>

namespace util {

struct Error {
    bilingual_str message;
};

//! The util::Result class provides a standard way for functions to return
//! either error messages or result values.
//!
//! It is intended for high-level functions that need to report error strings to
//! end users. Lower-level functions that don't need this error-reporting and
//! that only need error-handling should avoid util::Result and instead use
//! util::Expected, std::optional, std::variant, or custom structs
//! and enum types to return function results.
//!
//! Usage examples can be found in \example ../test/result_tests.cpp, but in
//! general code returning `util::Result<T>` values is very similar to code
//! returning `std::optional<T>` values. Existing functions returning
//! `std::optional<T>` can be updated to return `util::Result<T>` and return
//! error strings usually just replacing `return std::nullopt;` with `return
//! util::Error{error_string};`.
template <class M>
class Result
{
private:
    using T = std::conditional_t<std::is_same_v<M, void>, std::monostate, M>;

    std::variant<bilingual_str, T> m_variant;

    //! Disallow copy constructor, require Result to be moved for efficiency.
    Result(const Result&) = delete;

    //! Disallow operator= to avoid confusion in the future when the Result
    //! class gains support for richer error reporting, and callers should have
    //! ability to set a new result value without clearing existing error
    //! messages.
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) = delete;

    template <typename FT>
    friend bilingual_str ErrorString(const Result<FT>& result);

public:
    Result() : m_variant{std::in_place_index_t<1>{}, std::monostate{}} {}  // constructor for void
    Result(T obj) : m_variant{std::in_place_index_t<1>{}, std::move(obj)} {}
    Result(Error error) : m_variant{std::in_place_index_t<0>{}, std::move(error.message)} {}
    Result(Result&&) = default;
    ~Result() = default;

    //! std::optional methods, so functions returning optional<T> can change to
    //! return Result<T> with minimal changes to existing code, and vice versa.
    bool has_value() const noexcept { return m_variant.index() == 1; }
    const T& value() const LIFETIMEBOUND
    {
        assert(has_value());
        return std::get<1>(m_variant);
    }
    T& value() LIFETIMEBOUND
    {
        assert(has_value());
        return std::get<1>(m_variant);
    }
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
    explicit operator bool() const noexcept { return has_value(); }
    const T* operator->() const LIFETIMEBOUND { return &value(); }
    const T& operator*() const LIFETIMEBOUND { return value(); }
    T* operator->() LIFETIMEBOUND { return &value(); }
    T& operator*() LIFETIMEBOUND { return value(); }
};

template <typename T>
bilingual_str ErrorString(const Result<T>& result)
{
    return result ? bilingual_str{} : std::get<0>(result.m_variant);
}
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
