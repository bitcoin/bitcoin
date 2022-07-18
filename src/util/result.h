// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RESULT_H
#define BITCOIN_UTIL_RESULT_H

#include <attributes.h>
#include <util/translation.h>

#include <cassert>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace util {

//! The `Result` class provides a standard way for functions to return an error
//! string in addition to optional result values.
//!
//! The easiest way to understand `Result<T>` is to think of it as a substitute
//! for `std::optional<T>`, that just contains an error string in
//! addition to the optional return value. For example:
//!
//!    util::Result<int> AddNumbers(int a, int b)
//!    {
//!        if (b == 0) return util::Error{_("Not adding 0, that's dumb.")};
//!        return a + b;
//!    }
//!
//!    void TryAddNumbers(int a, int b)
//!    {
//!        if (auto result = AddNumbers(a, b)) {
//!            LogPrintf("%i + %i = %i\n", a, b, *result);
//!        } else {
//!            LogPrintf("Error: %s\n", util::ErrorString(result).translated);
//!        }
//!    }
//!
//! The `Result` class is intended to be used for high-level functions that need
//! to report error messages to end users. Low-level functions that don't need
//! error-reporting and only need error-handling should avoid `Result` and
//! instead use standard classes like `std::optional`, `std::variant`,
//! `std::expected`, and `std::tuple`, or custom structs and enum types to
//! return function results.
//!
//! Usage examples can be found in \example ../test/result_tests.cpp, but in
//! general code using `Result<T>` return values is similar to code using
//! `std::optional<T>` return values. Existing functions returning
//! `std::optional<T>` can be updated to return `Result<T>` usually just
//! replacing `return std::nullopt;` with `return util::Error{error_string};`.
//!
//! An optional failure type `F` can be passed as a `Result<T, F>` template
//! argument. The default type `F = void` is sufficient for most code that just
//! needs an error description without more complicated error handling. But code
//! that does need more fine-grained failure information can set custom error
//! values of type `F` and retrieve them with the `GetFailure()` method.
template <typename T, typename F = void>
class Result;

//! Wrapper type to pass error strings to Result constructors.
struct Error {
    bilingual_str message;
};

namespace detail {
//! Substitute for std::monostate that doesn't depend on std::variant.
struct MonoState{};

//! Error information only allocated on failure.
template <typename F>
struct ErrorInfo {
    std::conditional_t<std::is_same_v<F, void>, MonoState, F> failure;
    bilingual_str error;
};

//! Result base class which is inherited by Result<T, F>.
//! T is the type of the success return value, or void if there is none.
//! F is the type of the failure return value, or void if there is none.
template <typename T, typename F>
class ResultBase;

//! Result base specialization for empty (T=void) value type. Holds error
//! information and provides accessor methods.
template <typename F>
class ResultBase<void, F>
{
protected:
    std::unique_ptr<ErrorInfo<F>> m_info;

    //! Value setter methods which do nothing because this ResultBase class is
    //! specialized for type T=void, so there is no result value for it to hold.
    //! The other ResultBase specialization below for non-void T overrides these
    //! methods to manage the result value it contains.
    void ConstructValue() {}
    template <typename O>
    void MoveValue(O&& other) {}
    void DestroyValue() {}

    //! Helper function to construct a new Result success value or failure value
    //! using the arguments provided.
    template <bool Failure, typename Result, typename... Args>
    static void ConstructResult(Result& result, Args&&... args)
    {
        if constexpr (Failure) {
            static_assert(sizeof...(args) > 0 || !std::is_scalar_v<F>,
                "Refusing to default-construct failure value with int, float, enum, or pointer type, please specify an explicit failure value.");
            result.m_info.reset(new detail::ErrorInfo<F>{.failure{std::forward<Args>(args)...}, .error{}});
        } else {
            result.ConstructValue(std::forward<Args>(args)...);
        }
    }

    //! ConstructResult() overload peeling off a util::Error constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void ConstructResult(Result& result, util::Error error, Args&&... args)
    {
        ConstructResult</*Failure=*/true>(result, std::forward<Args>(args)...);
        result.m_info->error = std::move(error.message);
    }

    //! Helper function to move success or failure values and any error
    //! message from one result object to another. The two result
    //! objects must have compatible success and failure types.
    template <bool Constructed, typename DstResult, typename SrcResult>
    static void MoveResult(DstResult& dst, SrcResult&& src)
    {
        if constexpr (Constructed) {
            if (dst) {
                dst.DestroyValue();
            } else {
                dst.m_info.reset();
            }
        }
        if (src) {
            dst.MoveValue(std::move(src));
        } else {
            dst.m_info.reset(new detail::ErrorInfo<F>{std::move(*src.m_info)});
        }
    }

    //! Disallow operator= and require explicit Result::Set() calls to avoid
    //! confusion in the future when the Result class gains support for richer
    //! error reporting, and callers should have ability to set a new result
    //! value without clearing existing error messages.
    template <typename Result>
    ResultBase& operator=(Result&&) = delete;

    template <typename, typename>
    friend class ResultBase;

public:
    //! Success check.
    explicit operator bool() const { return !m_info; }

    //! Error retrieval.
    const auto& GetFailure() const LIFETIMEBOUND { assert(!*this); return m_info->failure; }

    static constexpr bool is_result{true};
};

//! Result base class for T value type. Holds value and provides accessor methods.
template <typename T, typename F>
class ResultBase : public ResultBase<void, F>
{
protected:
    //! Result success value. Uses anonymous union so success value is never
    //! constructed in failure case.
    union { T m_value; };

    template <typename... Args>
    void ConstructValue(Args&&... args) { new (&m_value) T{std::forward<Args>(args)...}; }
    template <typename O>
    void MoveValue(O&& other) { new (&m_value) T{std::move(other.m_value)}; }
    void DestroyValue() { m_value.~T(); }

    //! Empty constructor that needs to be declared because the class contains a union.
    ResultBase() {}
    ~ResultBase() { if (*this) DestroyValue(); }

    template <typename, typename>
    friend class ResultBase;

public:
    //! std::optional methods, so functions returning optional<T> can change to
    //! return Result<T> with minimal changes to existing code, and vice versa.
    bool has_value() const { return bool{*this}; }
    const T& value() const LIFETIMEBOUND { assert(has_value()); return m_value; }
    T& value() LIFETIMEBOUND { assert(has_value()); return m_value; }
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
    const T* operator->() const LIFETIMEBOUND { return &value(); }
    const T& operator*() const LIFETIMEBOUND { return value(); }
    T* operator->() LIFETIMEBOUND { return &value(); }
    T& operator*() LIFETIMEBOUND { return value(); }
};
} // namespace detail

template <typename T, typename F>
class Result : public detail::ResultBase<T, F>
{
protected:
    using Base = detail::ResultBase<T, F>;

public:
    //! Construct a Result object setting a success or failure value. An
    //! optional util::Error argument is processed first to set an error
    //! message. Then, any remaining arguments are passed to the type `T`
    //! constructor and used to construct a success value in the success case.
    //! In the failure case, any remaining arguments are passed to the type `F`
    //! constructor and used to construct a failure value.
    template <typename... Args>
    Result(Args&&... args)
    {
        Base::template ConstructResult</*Failure=*/false>(*this, std::forward<Args>(args)...);
    }

    //! Move-construct a Result object from another Result object, moving the
    //! success or failure value and any error message.
    template <typename O>
    requires (std::decay_t<O>::is_result)
    Result(O&& other)
    {
        Base::template MoveResult</*Constructed=*/false>(*this, std::move(other));
    }

    //! Move success or failure values and any error message from another Result
    //! object to this object.
    Result& Set(Result&& other) LIFETIMEBOUND
    {
        Base::template MoveResult</*Constructed=*/true>(*this, std::move(other));
        return *this;
    }

    inline friend bilingual_str _ErrorString(const Result& result)
    {
        return result ? bilingual_str{} : result.m_info->error;
    }
};

template <typename T, typename F>
bilingual_str ErrorString(const Result<T, F>& result) { return _ErrorString(result); }
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
