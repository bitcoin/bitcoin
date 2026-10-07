// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <sync.h>
#include <test/util/common.h>
#include <util/not_null.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <set>
#include <type_traits>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(util_not_null_tests)

static_assert(std::is_same_v<util::NotNullUniquePtr<int>, util::NotNull<std::unique_ptr<int>>>);
static_assert(std::is_same_v<util::NotNullSharedPtr<int>, util::NotNull<std::shared_ptr<int>>>);

template <class P>
concept NotNullConstructible = requires { typename std::bool_constant<(util::NotNull{P{}}, true)>; };

BOOST_AUTO_TEST_CASE(check_ctor)
{
    struct SomePtr {
        constexpr bool operator==(std::nullptr_t) const { return false; }
    };
    struct NullPtr {
        constexpr bool operator==(std::nullptr_t) const { return true; }
    };

    static_assert(NotNullConstructible<SomePtr>);
    static_assert(!NotNullConstructible<NullPtr>);
    static_assert(!NotNullConstructible<std::nullptr_t>);
    static_assert(!NotNullConstructible<int*>);
}

template <class T>
concept NotNullType = requires { typename util::NotNull<T>; };

BOOST_AUTO_TEST_CASE(check_ctor_ref)
{
    static_assert(!NotNullType<std::shared_ptr<int>&>);
    static_assert(!NotNullType<int*&>);
}

BOOST_AUTO_TEST_CASE(check_ctor_conv)
{
    using Ptr = std::shared_ptr<int>;
    using NNPtr = util::NotNullSharedPtr<int>;

    // For context: std::shared_ptr's raw pointer constructor is explicit
    static_assert(std::constructible_from<Ptr, int*>);
    static_assert(!std::convertible_to<int*, Ptr>);

    // util::NotNull's constructor is explicit
    static_assert(std::constructible_from<NNPtr, Ptr>);
    static_assert(!std::convertible_to<Ptr, NNPtr>);

    // For context: std::reference_wrapper's constructor is implicit
    using Ref = std::reference_wrapper<int>;
    static_assert(std::constructible_from<Ref, int&>);
    static_assert(std::convertible_to<int&, Ref>);

    using UniqPtr = std::unique_ptr<int>;
    using NNUniqPtr = util::NotNullUniquePtr<int>;

    // NNPtr can be copied, but NNUniqPtr can not:
    static_assert(std::is_nothrow_constructible_v<NNPtr, NNPtr&>);
    static_assert(std::is_nothrow_constructible_v<NNPtr, const NNPtr&>);
    static_assert(std::is_nothrow_constructible_v<NNUniqPtr, NNUniqPtr&&>);
    static_assert(!std::is_constructible_v<NNUniqPtr, NNUniqPtr&>);
    static_assert(!std::is_constructible_v<NNUniqPtr, const NNUniqPtr&>);
    static_assert(!std::is_constructible_v<NNPtr, const NNUniqPtr&>);

    // NNPtr from NNUniqPtr may throw:
    static_assert(!std::is_nothrow_constructible_v<NNPtr, NNUniqPtr&&>);
    static_assert(std::is_constructible_v<NNPtr, NNUniqPtr&&>);

    // Extraction is implicit:
    static_assert(std::convertible_to<NNUniqPtr&&, UniqPtr>);
    static_assert(std::convertible_to<const NNPtr&, Ptr>);
    static_assert(std::convertible_to<NNPtr&&, Ptr>);
}

template <class Target, class Source>
concept NotNullCopyConvertible = requires {
    typename std::bool_constant<([] {
        util::NotNull<Source> copy_source{Source{}};
        util::NotNull<Target>{copy_source};
        return true;
    }())>;
};

template <class Target, class Source>
concept NotNullMoveConvertible = requires { typename std::bool_constant<(util::NotNull<Target>{util::NotNull<Source>{Source{}}}, true)>; };

BOOST_AUTO_TEST_CASE(check_cross_type_conversion)
{
    struct SomePtr;
    struct NullPtr {
        constexpr NullPtr(const SomePtr&) {}
        constexpr bool operator==(std::nullptr_t) const { return true; }
    };
    struct SomePtr {
        constexpr explicit operator bool() const { return true; }
        constexpr bool operator==(std::nullptr_t) const { return false; }
    };

    // Assume checks failed conversions during constant evaluation.
#if !defined(_MSC_VER)
    // MSVC does not reject this failed constant evaluation in a requires-expression.
    // See https://github.com/bitcoin/bitcoin/issues/36153.
    static_assert(!NotNullCopyConvertible<NullPtr, SomePtr>);
    static_assert(!NotNullMoveConvertible<NullPtr, SomePtr>);
#endif
}

BOOST_AUTO_TEST_CASE(check_null_smart_pointer)
{
    // This conversion can throw, allowing the test hook to observe Assert.
    static_assert(!std::is_nothrow_constructible_v<util::NotNullSharedPtr<int>, std::unique_ptr<int>>);
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};
    BOOST_CHECK_EXCEPTION(util::NotNullSharedPtr<int>{std::unique_ptr<int>{nullptr}}, NonFatalCheckError,
                          HasReason{"Internal bug detected: m_ptr != nullptr"});
}

template <class P>
concept NullptrComparable = requires(P val) { val == nullptr; };
template <class P>
concept HasOperatorBool = requires(P val) { bool{val}; };

BOOST_AUTO_TEST_CASE(check_nullptr_compare)
{
    // The deleted ctor for nullptr also disallows nullptr compare:
    static_assert(NullptrComparable<std::shared_ptr<int>>);
    static_assert(!NullptrComparable<util::NotNullSharedPtr<int>>);

    static_assert(HasOperatorBool<std::shared_ptr<int>>);
    static_assert(!HasOperatorBool<util::NotNullSharedPtr<int>>);
}

BOOST_AUTO_TEST_CASE(check_nocopy_ptr)
{
    using NoCopyPtr = std::unique_ptr<int>;
    {
        NoCopyPtr no_copy_ptr{std::make_unique<int>()};
        util::NotNull no_copy{std::move(no_copy_ptr)};
        NoCopyPtr extract{std::move(no_copy)};
        (void)extract;
    }
    {
        util::NotNull no_copy{NoCopyPtr{std::make_unique<int>()}};
        NoCopyPtr extract{std::move(no_copy).get()};
        (void)extract;
    }
}

BOOST_AUTO_TEST_CASE(check_derived)
{
    struct MyBase {
        virtual ~MyBase() = default;
    };
    struct MyDerived : public MyBase {
    };

    util::NotNull p{std::make_shared<MyDerived>()};
    util::NotNull q{std::make_shared<MyBase>()};
    q = p;
    const bool same_ptr{q == p};
    BOOST_CHECK(same_ptr);

    util::NotNull nn_derived{std::make_unique<MyDerived>()};
    util::NotNull nn_base{std::make_unique<MyBase>()};
    nn_base = std::move(nn_derived);
    std::unique_ptr<MyBase> base_inner{std::move(nn_base)};
}

BOOST_AUTO_TEST_CASE(check_move)
{
    auto a{std::make_unique<int>()};
    util::NotNull box{std::move(a)};
    auto new_box{std::move(box)};

    auto b{std::make_shared<int>()};
    util::NotNull arc{b};
    auto new_arc{std::move(arc)};
}

BOOST_AUTO_TEST_CASE(check_swap)
{
    util::NotNull box_a{std::make_unique<int>(1)};
    util::NotNull box_b{std::make_unique<int>(2)};
    static_assert(std::is_nothrow_swappable_v<decltype(box_a)>);
    BOOST_CHECK_EQUAL(*box_a - *box_b, -1);
    swap(box_a, box_b);
    BOOST_CHECK_EQUAL(*box_a - *box_b, 1);
}

BOOST_AUTO_TEST_CASE(check_deref)
{
    util::NotNull p{std::make_unique<int>(2)};
    *p = 3;
    BOOST_CHECK_EQUAL(*p, 3);
    *p.get() = 4;
    BOOST_CHECK_EQUAL(*p, 4);
}

BOOST_AUTO_TEST_CASE(check_compare_set)
{
    auto a{std::make_shared<int>(1)};
    auto b{std::make_shared<int>(2)};
    std::set<util::NotNullSharedPtr<int>> uniq{};
    uniq.emplace(a);
    uniq.emplace(a);
    uniq.emplace(b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_CASE(check_hash_set)
{
    auto a{std::make_shared<int>(1)};
    auto b{std::make_shared<int>(2)};
    std::unordered_set<util::NotNullSharedPtr<int>> uniq{};
    uniq.emplace(a);
    uniq.emplace(a);
    uniq.emplace(b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_CASE(check_tsa)
{
    struct Thing {
        Mutex mutex{};
        int a GUARDED_BY(mutex){2};
    };
    const util::NotNull p{std::make_shared<Thing>()};
    // Clang thread safety annotation (TSA) may not be able to derive that
    // operator-> and operator* refer to the same object. So use a named
    // reference before taking the lock.
    Thing& ref{*p};
    LOCK(ref.mutex);
    BOOST_CHECK_EQUAL(ref.a, 2);
}

BOOST_AUTO_TEST_SUITE_END()
