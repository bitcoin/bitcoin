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

//! The `Result` class provides a standard way for functions to return error and
//! warning strings in addition to optional result values.
//!
//! The easiest way to understand `Result<T>` is to think of it as a substitute
//! for `std::optional<T>`, that just contains error and warning strings in
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

//! Wrapper types to pass error and warning strings to Result constructors.
struct Error {
    bilingual_str message;
};
struct Warning {
    bilingual_str message;
};

//! Wrapper type to move error and warning strings from an existing Result object to a new Result constructor.
template <typename Result>
struct MoveMessages {
    MoveMessages(Result&& result) : m_result(result) {}
    Result& m_result;
};

//! Template deduction guide for MoveMessages class.
template <class Result>
MoveMessages(Result&& result) -> MoveMessages<Result>;

namespace detail {
//! Empty string list
const std::vector<bilingual_str> EMPTY_LIST{};

//! Helper function to join messages in space separated string.
bilingual_str JoinMessages(const std::vector<bilingual_str>& errors, const std::vector<bilingual_str>& warnings);

//! Helper function to move messages from one vector to another.
void MoveMessages(std::vector<bilingual_str>& src, std::vector<bilingual_str>& dest);

//! Substitute for std::monostate that doesn't depend on std::variant.
struct MonoState{};

//! Error information only allocated if there are errors or warnings.
template <typename F>
struct ErrorInfo {
    std::optional<std::conditional_t<std::is_same_v<F, void>, MonoState, F>> failure{};
    std::vector<bilingual_str> errors{};
    std::vector<bilingual_str> warnings{};
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

    ErrorInfo<F>& Info() LIFETIMEBOUND
    {
        if (!m_info) m_info = std::make_unique<ErrorInfo<F>>();
        return *m_info;
    }

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
            result.Info().failure.emplace(std::forward<Args>(args)...);
        } else {
            result.ConstructValue(std::forward<Args>(args)...);
        }
    }

    //! ConstructResult() overload peeling off a util::Error constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void ConstructResult(Result& result, util::Error error, Args&&... args)
    {
        result.AddError(std::move(error.message));
        ConstructResult</*Failure=*/true>(result, std::forward<Args>(args)...);
    }

    //! ConstructResult() overload peeling off a util::Warning constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void ConstructResult(Result& result, util::Warning warning, Args&&... args)
    {
        result.AddWarning(std::move(warning.message));
        ConstructResult<Failure>(result, std::forward<Args>(args)...);
    }

    //! ConstructResult() overload peeling off a util::MoveMessages constructor argument.
    template <bool Failure, typename Result, typename R, typename... Args>
    static void ConstructResult(Result& result, util::MoveMessages<R> messages, Args&&... args)
    {
        result.MoveMessages(std::move(messages.m_result));
        ConstructResult<Failure>(result, std::forward<Args>(args)...);
    }

    //! Helper function to move success or failure values and any error and
    //! warning messages from one result object to another. The two result
    //! objects must have compatible success and failure types.
    template <bool Constructed, typename DstResult, typename SrcResult>
    static void MoveResult(DstResult& dst, SrcResult&& src)
    {
        if constexpr (Constructed) {
            if (dst) {
                dst.DestroyValue();
            } else {
                dst.m_info->failure.reset();
            }
        }
        dst.MoveMessages(src);
        if (src) {
            dst.MoveValue(std::move(src));
        } else {
            dst.Info().failure = std::move(src.m_info->failure);
        }
    }

    //! Disallow potentially dangerous assignment operators which might erase
    //! error and warning messages. The Result::Set() method can be used instead
    //! of operator= to assign result values while keeping any existing errors
    //! and warnings.
    template <typename Result>
    ResultBase& operator=(Result&&) = delete;

    template <typename, typename>
    friend class ResultBase;

public:
    //! Success check.
    explicit operator bool() const { return !m_info || !m_info->failure; }

    //! Error retrieval.
    const auto& GetFailure() const LIFETIMEBOUND { assert(!*this); return *m_info->failure; }
    const std::vector<bilingual_str>& GetErrors() const LIFETIMEBOUND { return m_info ? m_info->errors : EMPTY_LIST; }
    const std::vector<bilingual_str>& GetWarnings() const LIFETIMEBOUND { return m_info ? m_info->warnings : EMPTY_LIST; }

    //! Add error and warning messages.
    void AddError(bilingual_str error)
    {
        if (!error.empty()) this->Info().errors.emplace_back(std::move(error));
    }
    void AddWarning(bilingual_str warning)
    {
        if (!warning.empty()) this->Info().warnings.emplace_back(std::move(warning));
    }

    //! Move error and warning messages from another result to this one.
    template <typename Result>
    void MoveMessages(Result&& other)
    {
        if (other.m_info) {
            // Check that errors and warnings are empty before calling
            // MoveMessages to avoid allocating memory in this->Info() in the
            // typical case when there are no errors or warnings.
            if (!other.m_info->errors.empty()) detail::MoveMessages(other.m_info->errors, this->Info().errors);
            if (!other.m_info->warnings.empty()) detail::MoveMessages(other.m_info->warnings, this->Info().warnings);
        }
    }

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
    //! Construct a Result object setting a success or failure value and
    //! optional warning and error messages. Initial util::Error, util::Warning,
    //! and util::MoveMessages arguments are processed first to add warning and
    //! error messages. Then, any remaining arguments are passed to the type `T`
    //! constructor and used to construct a success value in the success case.
    //! In the failure case, any remaining arguments are passed to the type `F`
    //! constructor and used to construct a failure value.
    template <typename... Args>
    Result(Args&&... args)
    {
        Base::template ConstructResult</*Failure=*/false>(*this, std::forward<Args>(args)...);
    }

    //! Move-construct a Result object from another Result object, moving the
    //! success or failure value and any error or warning messages.
    template <typename O>
    requires (std::decay_t<O>::is_result)
    Result(O&& other)
    {
        Base::template MoveResult</*Constructed=*/false>(*this, std::move(other));
    }

    //! Move success or failure values from another Result object to this
    //! object. Also move any error and warning messages from the other Result
    //! object to this one. If this Result object has an existing success or
    //! failure value, it is cleared and replaced by the other value. If this
    //! Result object has any error or warning messages, they are kept and
    //! appended to.
    Result& Set(Result&& other) LIFETIMEBOUND
    {
        Base::template MoveResult</*Constructed=*/true>(*this, std::move(other));
        return *this;
    }
};

//! Operator moving warning and error messages from a source result object
//! to a destination result object. Only moves message strings, does not
//! change success or failure values of either Result object.
//!
//! This is useful for combining error and warning messages from multiple
//! result objects into a single object, e.g.:
//!
//!    util::Result<void> result;
//!    auto r1 = DoSomething() >> result;
//!    auto r2 = DoSomethingElse() >> result;
//!    ...
//!    return result;
//!
template <typename S, typename D>
requires (std::decay_t<S>::is_result)
S&& operator>>(S&& src LIFETIMEBOUND, D&& dst)
{
    dst.MoveMessages(src);
    return std::move(src);
}

//! Wrapper around util::Result that is less awkward to use with pointer types.
//!
//! It overloads pointer and bool operators so it isn't necessary to dereference
//! the result object twice to access the result value, so it possible to call
//! methods with `result->Method()` rather than `(*result)->Method()` and check
//! whether the pointer is null with `if (result)` rather than `if (result &&
//! *result)`.
//!
//! The `ResultPtr` class just adds syntax sugar to `Result` class. It is still
//! possible to access the result pointer directly using `ResultPtr` `value()`
//! and `has_value()` methods.
template <typename T, typename F = void>
class ResultPtr : public Result<T, F>
{
public:
    // Inherit Result constructors (excluding copy and move constructors).
    using Result<T, F>::Result;

    // Inherit Result copy and move constructors.
    template<typename O>
    ResultPtr(O&& other) : Result<T, F>{std::forward<O>(other)} {}

    explicit operator bool() const noexcept { return this->has_value() && this->value(); }
    auto* operator->() const { assert(this->value()); return &*this->value(); }
    auto& operator*() const { assert(this->value()); return *this->value(); }
};

//! Join error and warning messages in a space separated string. This is
//! intended for simple applications where there's probably only one error or
//! warning message to report, but multiple messages should not be lost if they
//! are present. More complicated applications should use GetErrors() and
//! GetWarning() methods directly.
template <typename T, typename F>
bilingual_str ErrorString(const Result<T, F>& result) { return detail::JoinMessages(result.GetErrors(), result.GetWarnings()); }
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
