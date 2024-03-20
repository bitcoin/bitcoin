// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RESULT_H
#define BITCOIN_UTIL_RESULT_H

#include <attributes.h>
#include <util/messages.h> // IWYU pragma: export
#include <util/translation.h>

#include <cassert>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace util {
//! The Result<SuccessType, ErrorType, InfoType, MessagesType> class provides
//! an efficient way for functions to return structured result information, as
//! well as descriptive error and warning messages.
//!
//! Logically, a result object is equivalent to:
//!
//!     tuple<variant<SuccessType, ErrorType>, InfoType, MessagesType>
//!
//! But the physical representation is more efficient because it avoids
//! allocating memory for ErrorType and MessagesType fields unless there is an
//! error.
//!
//! Result<SuccessType> objects support the same operators as
//! std::optional<SuccessType>, such as !result, *result, result->member, to
//! make SuccessType values easy to access. They also provide
//! result.error(), result.info(), and result.messages() methods to
//! access other parts of the result. A simple usage example is:
//!
//!    Result<int> AddNumbers(int a, int b)
//!    {
//!        if (b == 0) return util::Error{_("Not adding 0, that's dumb.")};
//!        return a + b;
//!    }
//!
//!    void TryAddNumbers(int a, int b)
//!    {
//!        if (auto result = AddNumbers(a, b)) {
//!            LogInfo("%i + %i = %i\n", a, b, *result);
//!        } else {
//!            LogInfo("Error: %s\n", util::ErrorString(result).translated);
//!        }
//!    }
//!
//! The `Result` class is superset of `std::expected`, providing additional
//! features for combining results from different functions and returning error
//! messages for users instead of just error values for callers.
//!
//! It provides an >> operator to move messages between results without
//! affecting success and error values.
//!
//!    Result<void> result;
//!    auto r1 = DoSomething() >> result;
//!    auto r2 = DoSomethingElse() >> result;
//!    ...
//!    return result;
//!
//! It also provides an update() method which is useful for returning early on
//! errors and for combining results of the same type.
//!
//!    Result<Value> result;
//!    if (!(DoSomething() >> result)) {
//!        result.update(Error{_("DoSomething failed.")});
//!        return result;
//!    }
//!    if (!result.update(DoSomethingElse())) {
//!        result.update(Error{_("DoSomethingElse failed.")});
//!        return result;
//!    }
//!    result.update(...);
//!    return result;
//!
//! Semantically, the update() method merges state from different results,
//! appending data instead of overwriting it where possible.
template <typename SuccessType = void, typename ErrorType = void, typename InfoType = void, typename MessagesType = Messages>
class Result;

//! Traits class that can be specialized to control the way result types are
//! combined and constructed. Specializations can provide an update() method
//! merging information from a source object to a destination object. They can
//! also define and empty() method to avoid updates if a source object contains
//! no data.  They can also define a construct() method to be accepted as
//! special arguments to the Result constructor, and a construct_error bool
//! member indicating whether to construct error values instead of success
//! values on use.
template<typename T>
struct ResultTraits {};

namespace detail {
//! Helper to use ResultTraits and update dst if src is nonempty.
template<typename T>
void ResultUpdate(auto&& dst_fn, T& src)
{
    using Traits = ResultTraits<T>;
    if constexpr (requires { Traits::empty; }) {
        if (Traits::empty(src)) return;
    }
    if constexpr (requires { Traits::update; }) {
        Traits::update(dst_fn(), std::move(src));
    } else {
        dst_fn() = std::move(src);
    }
}

//! Substitute for std::monostate that doesn't depend on std::variant.
struct Monostate{};

//! Implementation note: Result class inherits from an InfoHolder class holding an
//! IntoType value, a FailDataHolder class holding a unique_ptr to ErrorType
//! and MessagesTypes values, and a SuccessHolder class holding a SuccessType
//! value in an anonymous union.
//!
//! To take advantage of the Empty Base Optimization, inheritance is linear with
//! FailDataHolder inheriting from InfoHolder, SuccessHolder inheriting from
//! FailDataHolder, and Holder classes specializing for void so no space is used
//! when void types are specified.
//! @{
//! Container for InfoType, providing public info() method.
template <typename InfoType>
class InfoHolder
{
protected:
    InfoType m_info{};
public:
    // Public accessors.
    const InfoType& info() const LIFETIMEBOUND { return m_info; }
    InfoType& info() LIFETIMEBOUND { return m_info; }
};

//! Specialization of InfoHolder when InfoType is void.
template <>
class InfoHolder<void> {};

//! Container for ErrorType and MessagesType, providing public operator
//! bool(), error(), messages_ptr(), and messages() methods.
template <typename ErrorType, typename InfoType, typename MessagesType>
class FailDataHolder : public InfoHolder<InfoType>
{
protected:
    struct FailData {
        std::optional<std::conditional_t<std::is_same_v<ErrorType, void>, Monostate, ErrorType>> error{};
        MessagesType messages{};
    };
    std::unique_ptr<FailData> m_fail_data;

    // Private accessor, create FailData if it doesn't exist.
    FailData& fail_data() LIFETIMEBOUND
    {
        if (!m_fail_data) m_fail_data = std::make_unique<FailData>();
        return *m_fail_data;
    }

public:
    // Public accessors.
    explicit operator bool() const { return !m_fail_data || !m_fail_data->error; }
    const auto& error() const LIFETIMEBOUND { assert(!*this); return *m_fail_data->error; }
    auto& error() LIFETIMEBOUND { assert(!*this); return *m_fail_data->error; }
    const auto* messages_ptr() const LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto* messages_ptr() LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto& messages() LIFETIMEBOUND { return fail_data().messages; }
};

//! Container for SuccessType, providing public accessor methods similar to
//! std::optional methods to access the success value. There is a specialization
//! of this class for the void success type.
template <typename SuccessType, typename ErrorType, typename InfoType, typename MessagesType>
class SuccessHolder : public FailDataHolder<ErrorType, InfoType, MessagesType>
{
protected:
    //! Success value embedded in an anonymous union so it doesn't need to be
    //! constructed if the result is holding a error value,
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
template <typename ErrorType, typename InfoType, typename MessagesType>
class SuccessHolder<void, ErrorType, InfoType, MessagesType> : public FailDataHolder<ErrorType, InfoType, MessagesType>
{
};
//! @}
} // namespace detail

// Result type class, documented at the top of this file.
template <typename SuccessType_, typename ErrorType_, typename InfoType_, typename MessagesType_>
class Result : public detail::SuccessHolder<SuccessType_, ErrorType_, InfoType_, MessagesType_>
{
public:
    using SuccessType = SuccessType_;
    using ErrorType = ErrorType_;
    using InfoType = InfoType_;
    using MessagesType = MessagesType_;
    static constexpr bool is_result{true};

    //! Construct a Result object setting a success or error value. Initial
    //! special arguments with ResultTraits::constuct methods are processed
    //! first, and then remaining arguments are passed to the SuccessType
    //! constructor if this is a successful result or to the FailType
    //! constructor otherwise.
    template <typename... Args>
    Result(Args&&... args)
    {
        construct</*Error=*/false>(*this, std::forward<Args>(args)...);
    }

    //! Move-construct a Result object from another Result object, moving the
    //! success or error value and messages.
    template <typename O>
    requires (std::decay_t<O>::is_result)
    Result(O&& other)
    {
        move</*DstConstructed=*/false>(*this, other);
    }

    //! Update this result by moving from another result object. Existing
    //! success, error, info, and messages values are updated using
    //! ResultTraits::update calls so state can be combined instead of
    //! overwritten.
    Result& update(Result&& other) LIFETIMEBOUND
    {
        move</*DstConstructed=*/true>(*this, other);
        return *this;
    }

    //! Disallow operator= and require explicit Result::update() calls to avoid
    //! accidentally clearing messages when combining results.
    Result& operator=(Result&&) = delete;

protected:
    template <typename, typename, typename, typename>
    friend class Result;

    //! Helper function to construct a new success or error value using the
    //! arguments provided.
    template <bool Error, typename Result, typename... Args>
    static void construct(Result& result, Args&&... args)
    {
        if constexpr (Error) {
            static_assert(sizeof...(args) > 0 || !std::is_scalar_v<ErrorType>,
                "Refusing to default-construct error value with int, float, enum, or pointer type, please specify an explicit error value.");
            result.fail_data().error.emplace(std::forward<Args>(args)...);
        } else if constexpr (std::is_same_v<typename Result::SuccessType, void>) {
            static_assert(sizeof...(args) == 0, "Error: Arguments cannot be passed to a Result<void> constructor.");
        } else {
            new (&result.m_success) typename Result::SuccessType{std::forward<Args>(args)...};
        }
    }

    //! Overload for construct() for special arguments with
    //! ResultTraits::construct method defined.
    template <bool Error, typename Result, typename T, typename... Args>
    requires requires { ResultTraits<std::decay_t<T>>::construct(std::declval<Result&>(), std::declval<T&&>()); }
    static void construct(Result& result, T&& arg, Args&&... args)
    {
        using Traits = ResultTraits<std::decay_t<T>>;
        constexpr bool construct_error{Error || []{
            if constexpr (requires { Traits::construct_error; }) return Traits::construct_error;
            return false;
        }()};
        Traits::construct(result, std::forward<T>(arg));
        construct<construct_error>(result, std::forward<Args>(args)...);
    }

    //! Move success, error, info, and message values source Result object to
    //! destination object. Existing values are updated using
    //! ResultTraits::update calls so state can be combined instead of
    //! overwritten.
    //! The source and destination results are not required to have the same
    //! types, and assigning void source types to non-void destinations type is
    //! allowed, since no source information is lost. But assigning non-void
    //! source types to void destination types is not allowed, since this would
    //! discard source information.
    template <bool DstConstructed, typename DstResult, typename SrcResult>
    static void move(DstResult& dst, SrcResult& src)
    {
        // Use operator>> to move info and messages values first, then move
        // success or error value below.
        src >> dst;
        // If DstConstructed is true, it means dst has either a success value or
        // a error value set, which needs to be updated or replaced. If
        // DstConstructed is false, then dst is being constructed now and has no
        // values set.
        if constexpr (DstConstructed) {
            if (dst && src) {
                // dst and src both hold success values, so update dst from src and return
                if constexpr (!std::is_same_v<typename DstResult::SuccessType, void>) {
                    detail::ResultUpdate([&]()->auto& { return *dst; }, *src);
                }
                return;
            } else if (!dst && !src) {
                // dst and src both hold error values, so update dst from src and return
                if constexpr (!std::is_same_v<typename DstResult::ErrorType, void>) {
                    detail::ResultUpdate([&]()->auto& { return dst.error(); }, src.error());
                }
                return;
            } else if (dst) {
                // dst has a success value, so destroy it before replacing it with src error value
                if constexpr (!std::is_same_v<typename DstResult::SuccessType, void>) {
                    using DstSuccess = typename DstResult::SuccessType;
                    dst.m_success.~DstSuccess();
                }
            } else {
                // dst has a error value, so reset it before replacing it with src success value
                dst.m_fail_data->error.reset();
            }
        }
        // At this point dst has no success or error value set. Can assert
        // there is no error value.
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
            // src has a error value, so move it to dst. If the src error
            // type is void, just initialize the dst error value by default
            // initialization.
            if constexpr (!std::is_same_v<typename SrcResult::ErrorType, void>) {
                dst.fail_data().error.emplace(std::move(src.error()));
            } else {
                dst.fail_data().error.emplace();
            }
            assert(!dst);
        }
    }
};

//! Move information from a source Result object to a destination object. It
//! only moves InfoType and MessagesType values without affecting SuccessType or
//! ErrorType values of either Result object.
template <typename SrcResult, typename DstResult>
requires (std::decay_t<SrcResult>::is_result)
decltype(auto) operator>>(SrcResult&& src LIFETIMEBOUND, DstResult&& dst)
{
    using SrcType = std::decay_t<SrcResult>;
    if constexpr (!std::is_same_v<typename SrcType::InfoType, void>) {
        detail::ResultUpdate([&]()->auto& { return dst.info(); }, src.info());
    }
    if (src.messages_ptr()) {
        detail::ResultUpdate([&]()->auto& { return dst.messages(); }, src.messages());
    }
    return static_cast<SrcType&&>(src);
}

//! ResultTraits specialization for Messages struct.
template<>
struct ResultTraits<Messages> {
    static void update(Messages& dst, Messages&& src)
    {
        MoveMessages(dst, src);
    }
    static bool empty(const Messages& src)
    {
        return src.errors.empty() && src.warnings.empty();
    }
};

//! ResultTraits specialization for Error struct.
template<>
struct ResultTraits<Error> {
    static constexpr bool construct_error{true};

    template<typename ResultType>
    static void construct(ResultType& dst, Error&& src)
    {
       dst.messages().errors.push_back(std::move(src.message));
    }
};

//! ResultTraits specialization for Warning struct.
template<>
struct ResultTraits<Warning> {
    template<typename ResultType>
    static void construct(ResultType& dst, Warning&& src)
    {
       dst.messages().warnings.push_back(std::move(src.message));
    }
};

//! ResultTraits specialization for Info struct.
template<typename T>
struct ResultTraits<Info<T>> {
    template<typename ResultType>
    static void construct(ResultType& dst, Info<T>&& src)
    {
       dst.info() = std::move(src.m_value);
    }
};
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
