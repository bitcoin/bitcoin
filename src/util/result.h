// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RESULT_H
#define BITCOIN_UTIL_RESULT_H

#include <util/translation.h>

#include <optional>

namespace util {

//! Function result type intended for high-level functions that return error and
//! warning strings in addition to normal result types.
//!
//! The Result<T> class is meant to be a drop-in replacement for
//! std::optional<T> except it has additional methods to return error and
//! warning strings for error reporting. Also, unlike std::optional, in order to
//! support error handling in cases where callees need to pass additional
//! information about failures back to callers, Result<T> objects can always be
//! dereferenced regardless of the function's success or failure.
//!
//! This class is not intended to be used by low-level functions that do not
//! return error or warning strings. These functions should use plain
//! std::optional or std::variant types instead.
//!
//! See unit tests in result_tests.cpp for example usages.
template<typename T>
class Result;

//! Tag types for result constructors.
struct Error{};
struct ErrorChain{};

//! Result<void> specialization. Only returns errors and warnings, no values.
template<>
class Result<void>
{
protected:
    std::vector<bilingual_str> m_errors;
    std::vector<bilingual_str> m_warnings;

public:
    //! Success case constructor.
    Result() = default;

    //! Error case constructor for a single error.
    Result(Error, bilingual_str error)
    {
        m_errors.emplace_back(std::move(error));
    }

    //! Error case constructor for a chained error.
    Result(ErrorChain, bilingual_str error, Result<void>&& previous) : m_errors{std::move(previous.m_errors)}, m_warnings{std::move(previous.m_warnings)}
    {
        m_errors.emplace_back(std::move(error));
    }

    //! Success check.
    operator bool() const { return m_errors.empty(); }

    //! Error retrieval.
    const std::vector<bilingual_str>& GetErrors() const { return m_errors; }
    std::tuple<const std::vector<bilingual_str>&, const std::vector<bilingual_str>&> GetErrorsAndWarnings() const { return {m_errors, m_warnings}; }
};

template<typename T>
class Result : public Result<void>
{
protected:
    T m_result;

public:
    //! Constructors that forward to the base class and pass additional arguments to m_result.
    template<typename... Args>
    Result(Args&&... args) : m_result{std::forward<Args>(args)...} {}
    template<typename Str, typename...Args>
    Result(Error, Str&& str, Args&&... args) : Result<void>{Error{}, std::forward<Str>(str)}, m_result{std::forward<Args>(args)...} {};
    template<typename Str, typename Prev, typename...Args>
    Result(ErrorChain, Str&& str, Prev&& prev, Args&&... args) : Result<void>{ErrorChain{}, std::forward<Str>(str), std::forward<Prev>(prev)}, m_result{std::forward<Args>(args)...} {};

    //! std::optional methods, so Result<T> can be easily swapped for
    //! std::optional<T> to add error reporting to existing code or remove it if
    //! it is no longer needed.
    bool has_value() const { return m_errors.empty(); }
    const T& value() const { return m_result; }
    T& value() { return m_result; }
    template<typename U> T value_or(const U& default_value) const
    {
        return has_value() ? value() : default_value;
    }
    const T* operator->() const { return &value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    T& operator*() { return value(); }
};

//! Helper method to retrieve a simple error string from Result<T> or
//! Result<void>.
bilingual_str ErrorDescription(const Result<void>& result);
} // namespace util

/**
 * Backwards-compatible interface for util::Result class. New code should prefer
 * util::Result class which supports returning error information along with
 * result information and supports returing `void` and `bilingual_str` results.
*/
template<class T>
class BResult {
private:
    util::Result<std::optional<T>> m_result;

public:
    BResult() : m_result{util::Error{}, Untranslated("")} {};
    BResult(const T& value) : m_result{value} {}
    BResult(T&& value) : m_result{std::move(value)} {}
    BResult(const bilingual_str& error) : m_result{util::Error{}, error} {}
    bool HasRes() const { return m_result.has_value(); }
    const T& GetObj() const { assert(HasRes()); return *m_result.value(); }
    T ReleaseObj() { assert(HasRes()); return std::move(*m_result.value()); }
    const bilingual_str& GetError() const { assert(!HasRes()); return m_result.GetErrors().back(); }
    explicit operator bool() const { return HasRes(); }
};

#endif // BITCOIN_UTIL_RESULT_H
