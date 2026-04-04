// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <test/util/common.h>
#include <util/pointers.h>

#include <boost/test/unit_test.hpp>

#include <set>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(util_pointers_tests)

BOOST_AUTO_TEST_CASE(check_nullptr)
{
    // Disable aborts to test the runtime behavior of passing a nullptr to
    // NotNull
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};

    // This unit test only works with a type that may throw on a move
    // construction
    struct ThrowingMoveNullPtr {
        ThrowingMoveNullPtr() = default;
        ThrowingMoveNullPtr(ThrowingMoveNullPtr&& other) {};
        bool operator==(std::nullptr_t) const { return true; }
        int* p{};
    };
    static_assert(!std::is_nothrow_move_constructible_v<ThrowingMoveNullPtr>);
    BOOST_CHECK_EXCEPTION(util::NotNull{ThrowingMoveNullPtr{}}, NonFatalCheckError, HasReason{"Internal bug detected: ptr_ != nullptr"});
}

BOOST_AUTO_TEST_CASE(check_nocopy_ptr)
{
    struct NoCopyPtr {
        NoCopyPtr() = default;
        NoCopyPtr(const NoCopyPtr& other) = delete;
        NoCopyPtr(NoCopyPtr&& other) = default;
        bool operator==(std::nullptr_t) const { return false; }
        int* p{};
    };

    {
        NoCopyPtr no_copy_ptr{};
        util::NotNull no_copy{std::move(no_copy_ptr)};
        NoCopyPtr extract{std::move(no_copy)};
        (void)extract;
    }
    {
        util::NotNull no_copy{NoCopyPtr{}};
        NoCopyPtr extract{std::move(no_copy).get()};
        (void)extract;
    }
    {
        util::NotNull no_copy{NoCopyPtr{}};
        auto extract{std::move(no_copy).get()};
        static_assert(std::is_same_v<decltype(extract), NoCopyPtr>);
    }
}

BOOST_AUTO_TEST_CASE(check_derived)
{
    struct MyBase {
    };
    struct MyDerived : public MyBase {
    };

    MyBase base;
    MyDerived derived;
    util::NotNull<MyDerived*> p{&derived};
    util::NotNull<MyBase*> q(&base);
    q = p;
    BOOST_CHECK_EQUAL(q, p);

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
    int v{2};
    util::NotNull p(&v);
    *p = 3;
    BOOST_CHECK_EQUAL(v, 3);
    *p.get() = 4;
    BOOST_CHECK_EQUAL(v, 4);
    util::NotNull<const int*> c{&v};
    v = 5;
    BOOST_CHECK_EQUAL(*c, 5);
}

BOOST_AUTO_TEST_CASE(check_compare_set)
{
    int a;
    int b;
    std::set<util::NotNull<int*>> uniq{};
    uniq.emplace(&a);
    uniq.emplace(&a);
    uniq.emplace(&b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_CASE(check_hash_set)
{
    int a;
    int b;
    std::unordered_set<util::NotNull<int*>> uniq{};
    uniq.emplace(&a);
    uniq.emplace(&a);
    uniq.emplace(&b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
