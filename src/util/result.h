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
//! Default MessagesType, simple list of errors and warnings.
struct Messages {
    std::vector<bilingual_str> errors{};
    std::vector<bilingual_str> warnings{};
};

//! The Result<SuccessType, FailureType, MessagesType> class provides
//! an efficient way for functions to return structured result information, as
//! well as descriptive error and warning messages.
//!
//! Logically, a result object is equivalent to:
//!
//!     tuple<variant<SuccessType, FailureType>, MessagesType>
//!
//! But the physical representation is more efficient because it avoids
//! allocating memory for FailureType and MessagesType fields unless there is an
//! error.
//!
//! Result<SuccessType> objects support the same operators as
//! std::optional<SuccessType>, such as !result, *result, result->member, to
//! make SuccessType values easy to access. They also provide
//! result.GetFailure() and result.GetMessages() methods to
//! access other parts of the result. A simple usage example is:
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
//! Usage examples can be found in \example ../test/result_tests.cpp.
template <typename SuccessType = void, typename FailureType = void, typename MessagesType = Messages>
class Result;

//! Wrapper object to pass an error string to the Result constructor.
struct Error {
    bilingual_str message;
};
//! Wrapper object to pass a warning string to the Result constructor.
struct Warning {
    bilingual_str message;
};

//! Type trait that can be specialized to control the way SuccessType /
//! FailureType / MessagesType values are combined. Default behavior
//! is for new values to overwrite existing ones, but this can be specialized
//! for custom behavior when Result::Update() method is called or << operator is
//! used. For example, this is specialized for Messages struct below to append
//! error and warning messages instead of overwriting them. It can also be used,
//! for example, to merge FailureType values when a function can return multiple
//! failures.
template<typename T>
struct ResultTraits {
    template<typename O>
    static void Update(O& dst, T& src)
    {
        dst = std::move(src);
    }
};

//! Type trait that can be specialized to let the Result class work with custom
//! MessagesType types holding error and warning messages.
template<typename MessagesType>
struct MessagesTraits;

//! ResultTraits specialization for Messages struct.
template<>
struct ResultTraits<Messages> {
    static void Update(Messages& dst, Messages& src);
};

//! MessagesTraits specialization for Messages struct.
template<>
struct MessagesTraits<Messages> {
    static void AddError(Messages& messages, bilingual_str error)
    {
        messages.errors.emplace_back(std::move(error));
    }
    static void AddWarning(Messages& messages, bilingual_str warning)
    {
        messages.warnings.emplace_back(std::move(warning));
    }
    static bool HasMessages(const Messages& messages)
    {
        return messages.errors.size() || messages.warnings.size();
    }
};

namespace detail {
//! Helper function to join messages in space separated string.
bilingual_str JoinMessages(const Messages& messages);

//! Substitute for std::monostate that doesn't depend on std::variant.
struct Monostate{};

//! Implemention note: Result class inherits from a FailDataHolder class holding
//! a unique_ptr to FailureType and MessagesTypes values, and a SuccessHolder
//! class holding a SuccessType value in an anonymous union.
//! @{
//! Container for FailureType and MessagesType, providing public operator
//! bool(), GetFailure(), GetMessages(), and EnsureMessages() methods.
template <typename FailureType, typename MessagesType>
class FailDataHolder
{
protected:
    struct FailData {
        std::optional<std::conditional_t<std::is_same_v<FailureType, void>, Monostate, FailureType>> failure{};
        MessagesType messages{};
    };
    std::unique_ptr<FailData> m_fail_data;

    // Private accessor, create FailData if it doesn't exist.
    FailData& EnsureFailData() LIFETIMEBOUND
    {
        if (!m_fail_data) m_fail_data = std::make_unique<FailData>();
        return *m_fail_data;
    }

public:
    // Public accessors.
    explicit operator bool() const { return !m_fail_data || !m_fail_data->failure; }
    const auto& GetFailure() const LIFETIMEBOUND { assert(!*this); return *m_fail_data->failure; }
    auto& GetFailure() LIFETIMEBOUND { assert(!*this); return *m_fail_data->failure; }
    const auto* GetMessages() const LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto* GetMessages() LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto& EnsureMessages() LIFETIMEBOUND { return EnsureFailData().messages; }
};

//! Container for SuccessType, providing public accessor methods similar to
//! std::optional methods to access the success value.
template <typename SuccessType, typename FailureType, typename MessagesType>
class SuccessHolder : public FailDataHolder<FailureType, MessagesType>
{
protected:
    //! Success value embedded in an anonymous union so it doesn't need to be
    //! constructed if the result is holding a failure value,
    union { SuccessType m_success; };

    //! Empty constructor that needs to be declared because the class contains a union.
    SuccessHolder() {}
    ~SuccessHolder() { if (*this) m_success.~SuccessType(); }

public:
    // Public accessors.
    bool has_value() const { return bool{*this}; }
    const SuccessType& value() const LIFETIMEBOUND { assert(has_value()); return m_success; }
    SuccessType& value() LIFETIMEBOUND { assert(has_value()); return m_success; }
    template <class U>
    SuccessType value_or(U&& default_value) const&
    {
        return has_value() ? value() : std::forward<U>(default_value);
    }
    template <class U>
    SuccessType value_or(U&& default_value) &&
    {
        return has_value() ? std::move(value()) : std::forward<U>(default_value);
    }
    const SuccessType* operator->() const LIFETIMEBOUND { return &value(); }
    const SuccessType& operator*() const LIFETIMEBOUND { return value(); }
    SuccessType* operator->() LIFETIMEBOUND { return &value(); }
    SuccessType& operator*() LIFETIMEBOUND { return value(); }
};

//! Specialization of SuccessHolder when SuccessType is void.
template <typename FailureType, typename MessagesType>
class SuccessHolder<void, FailureType, MessagesType> : public FailDataHolder<FailureType, MessagesType>
{
};
//! @}
} // namespace detail

// Result type class, documented at the top of this file.
template <typename SuccessType_, typename FailureType_, typename MessagesType_>
class Result : public detail::SuccessHolder<SuccessType_, FailureType_, MessagesType_>
{
public:
    using SuccessType = SuccessType_;
    using FailureType = FailureType_;
    using MessagesType = MessagesType_;
    static constexpr bool is_result{true};

    //! Construct a Result object setting a success or failure value and
    //! optional warning and error messages. Initial util::Error and
    //! util::Warning arguments are processed first to add warning and error
    //! messages. Then, any remaining arguments are passed to the SuccessType
    //! constructor and used to construct a success value in the success case.
    //! In the failure case, if any util::Error arguments were passed, any
    //! remaining arguments are passed to the FailureType constructor and used
    //! to construct a failure value.
    template <typename... Args>
    Result(Args&&... args)
    {
        Construct</*Failure=*/false>(*this, std::forward<Args>(args)...);
    }

    //! Move-construct a Result object from another Result object, moving the
    //! success or failure value and any error or warning messages.
    template <typename O>
    requires (std::decay_t<O>::is_result)
    Result(O&& other)
    {
        Move</*Constructed=*/false>(*this, other);
    }

    //! Update this result by moving from another result object. Existing
    //! success, failure, and messages values are updated (using
    //! ResultTraits::Update specializations), so errors and warning messages
    //! get appended instead of overwriting existing ones.
    Result& Update(Result&& other) LIFETIMEBOUND
    {
        Move</*Constructed=*/true>(*this, other);
        return *this;
    }

    //! Disallow operator= and require explicit Result::Update() calls to avoid
    //! accidentally clearing error and warning messages when combining results.
    Result& operator=(Result&&) = delete;

    void AddError(bilingual_str error)
    {
        if (!error.empty()) MessagesTraits<MessagesType>::AddError(this->EnsureFailData().messages, std::move(error));
    }
    void AddWarning(bilingual_str warning)
    {
        if (!warning.empty()) MessagesTraits<MessagesType>::AddWarning(this->EnsureFailData().messages, std::move(warning));
    }

protected:
    template <typename, typename, typename>
    friend class Result;

    //! Helper function to construct a new success or failure value using the
    //! arguments provided.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, Args&&... args)
    {
        if constexpr (Failure) {
            static_assert(sizeof...(args) > 0 || !std::is_scalar_v<FailureType>,
                "Refusing to default-construct failure value with int, float, enum, or pointer type, please specify an explicit failure value.");
            result.EnsureFailData().failure.emplace(std::forward<Args>(args)...);
        } else {
            if constexpr (!std::is_same_v<typename Result::SuccessType, void>) {
                new (&result.m_success) typename Result::SuccessType{std::forward<Args>(args)...};
            }
        }
    }

    //! Construct() overload peeling off a util::Error constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, util::Error error, Args&&... args)
    {
        result.AddError(std::move(error.message));
        Construct</*Failure=*/true>(result, std::forward<Args>(args)...);
    }

    //! Construct() overload peeling off a util::Warning constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, util::Warning warning, Args&&... args)
    {
        result.AddWarning(std::move(warning.message));
        Construct<Failure>(result, std::forward<Args>(args)...);
    }

    //! Move success, failure, and messages from source Result object to
    //! destination object. Existing values are updated (using
    //! ResultTraits::Update specializations), so destination errors and warning
    //! messages get appended to instead of overwritten. The source and
    //! destination results are not required to have the same types, and
    //! assigning void source types to non-void destinations type is allowed,
    //! since no source information is lost. But assigning non-void source types
    //! to void destination types is not allowed, since this would discard
    //! source information.
    template <bool DstConstructed, typename DstResult, typename SrcResult>
    static void Move(DstResult& dst, SrcResult& src)
    {
        // Use operator>> to move messages value first, then move
        // success or failure value below.
        src >> dst;
        // If DstConstructed is true, it means dst has either a success value or
        // a failure value set, which needs to be updated or replaced. If
        // DstConstructed is false, then dst is being constructed now and has no
        // values set.
        if constexpr (DstConstructed) {
            if (dst && src) {
                // dst and src both hold success values, so update dst from src and return
                if constexpr (!std::is_same_v<typename SrcResult::SuccessType, void>) {
                    ResultTraits<typename SrcResult::SuccessType>::Update(*dst, *src);
                }
                return;
            } else if (!dst && !src) {
                // dst and src both hold failure values, so update dst from src and return
                if constexpr (!std::is_same_v<typename SrcResult::FailureType, void>) {
                    ResultTraits<typename SrcResult::FailureType>::Update(dst.GetFailure(), src.GetFailure());
                }
                return;
            } else if (dst) {
                // dst has a success value, so destroy it before replacing it with src failure value
                if constexpr (!std::is_same_v<typename DstResult::SuccessType, void>) {
                    using DstSuccess = typename DstResult::SuccessType;
                    dst.m_success.~DstSuccess();
                }
            } else {
                // dst has a failure value, so reset it before replacing it with src success value
                dst.m_fail_data->failure.reset();
            }
        }
        // At this point dst has no success or failure value set. Can assert
        // there is no failure value.
        assert(dst);
        if (src) {
            // src has a success value, so move it to dst. If the src success
            // type is void and the dst success type is non-void, just
            // initialize the dst success value by default initialization.
            if constexpr (!std::is_same_v<typename SrcResult::SuccessType, void>) {
               new (&dst.m_success) typename DstResult::SuccessType{std::move(src.m_success)};
            } else if constexpr (!std::is_same_v<typename DstResult::SuccessType, void>) {
               new (&dst.m_success) typename DstResult::SuccessType{};
            }
            assert(dst);
        } else {
            // src has a failure value, so move it to dst. If the src failure
            // type is void, just initialize the dst failure value by default
            // initialization.
            if constexpr (!std::is_same_v<typename SrcResult::FailureType, void>) {
                dst.EnsureFailData().failure.emplace(std::move(src.GetFailure()));
            } else {
                dst.EnsureFailData().failure.emplace();
            }
            assert(!dst);
        }
    }
};

//! Move information from a source Result object to a destination object. It
//! only moves MessagesType values without affecting SuccessType or
//! FailureType values of either Result object.
//!
//! This is useful for combining error and warning messages from multiple result
//! objects into a single object, e.g.:
//!
//!    util::Result<void> result;
//!    auto r1 = DoSomething() >> result;
//!    auto r2 = DoSomethingElse() >> result;
//!    ...
//!    return result;
//!
template <typename SrcResult, typename DstResult>
requires (std::decay_t<SrcResult>::is_result)
decltype(auto) operator>>(SrcResult&& src LIFETIMEBOUND, DstResult&& dst)
{
    using SrcType = std::decay_t<SrcResult>;
    if (src.GetMessages() && MessagesTraits<typename SrcType::MessagesType>::HasMessages(*src.GetMessages())) {
        ResultTraits<typename SrcType::MessagesType>::Update(dst.EnsureMessages(), *src.GetMessages());
    }
    return static_cast<SrcType&&>(src);
}

//! Join error and warning messages in a space separated string. This is
//! intended for simple applications where there's probably only one error or
//! warning message to report, but multiple messages should not be lost if they
//! are present. More complicated applications should use Result::GetMessages()
//! method directly.
template <typename Result>
bilingual_str ErrorString(const Result& result)
{
    const auto* messages{result.GetMessages()};
    return messages ? detail::JoinMessages(*messages) : bilingual_str{};
}
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
