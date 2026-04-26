// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <test/util/setup_common.h>
#include <util/expected.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>
#include <utility>


using namespace util;

BOOST_AUTO_TEST_SUITE(util_expected_tests)

BOOST_AUTO_TEST_CASE(expected_value)
{
    struct Obj {
        int x;
    };
    Expected<Obj, int> e{};
    BOOST_CHECK_EQUAL(e.value().x, 0);

    e = Obj{42};

    BOOST_CHECK(e.has_value());
    BOOST_CHECK(static_cast<bool>(e));
    BOOST_CHECK_EQUAL(e.value().x, 42);
    BOOST_CHECK_EQUAL((*e).x, 42);
    BOOST_CHECK_EQUAL(e->x, 42);

    // modify value
    e.value().x += 1;
    (*e).x += 1;
    e->x += 1;

    const auto& read{e};
    BOOST_CHECK_EQUAL(read.value().x, 45);
    BOOST_CHECK_EQUAL((*read).x, 45);
    BOOST_CHECK_EQUAL(read->x, 45);
}

BOOST_AUTO_TEST_CASE(expected_value_rvalue)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(5)};
    const auto moved{std::move(no_copy).value()};
    BOOST_CHECK_EQUAL(*moved, 5);
}

BOOST_AUTO_TEST_CASE(expected_deref_rvalue)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(5)};
    const auto moved{*std::move(no_copy)};
    BOOST_CHECK_EQUAL(*moved, 5);
}

BOOST_AUTO_TEST_CASE(expected_value_or)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(1)};
    const int one{*std::move(no_copy).value_or(std::make_unique<int>(2))};
    BOOST_CHECK_EQUAL(one, 1);

    const Expected<std::string, int> const_val{Unexpected{-1}};
    BOOST_CHECK_EQUAL(const_val.value_or("fallback"), "fallback");
}

BOOST_AUTO_TEST_CASE(expected_value_throws)
{
    const Expected<int, std::string> e{Unexpected{"fail"}};
    BOOST_CHECK_THROW(e.value(), BadExpectedAccess);

    const Expected<void, std::string> void_e{Unexpected{"fail"}};
    BOOST_CHECK_THROW(void_e.value(), BadExpectedAccess);
}

BOOST_AUTO_TEST_CASE(expected_error)
{
    Expected<void, std::string> e{};
    BOOST_CHECK(e.has_value());
    [&]() -> void { return e.value(); }(); // check value returns void and does not throw
    [&]() -> void { return *e; }();

    e = Unexpected{"fail"};
    BOOST_CHECK(!e.has_value());
    BOOST_CHECK(!static_cast<bool>(e));
    BOOST_CHECK_EQUAL(e.error(), "fail");

    // modify error
    e.error() += "1";

    const auto& read{e};
    BOOST_CHECK_EQUAL(read.error(), "fail1");
}

BOOST_AUTO_TEST_CASE(expected_error_rvalue)
{
    {
        Expected<int, std::unique_ptr<int>> nocopy_err{Unexpected{std::make_unique<int>(7)}};
        const auto moved{std::move(nocopy_err).error()};
        BOOST_CHECK_EQUAL(*moved, 7);
    }
    {
        Expected<void, std::unique_ptr<int>> void_nocopy_err{Unexpected{std::make_unique<int>(9)}};
        const auto moved{std::move(void_nocopy_err).error()};
        BOOST_CHECK_EQUAL(*moved, 9);
    }
}

BOOST_AUTO_TEST_CASE(unexpected_error_accessors)
{
    Unexpected u{std::make_unique<int>(-1)};
    BOOST_CHECK_EQUAL(*u.error(), -1);

    *u.error() -= 1;
    const auto& read{u};
    BOOST_CHECK_EQUAL(*read.error(), -2);

    const auto moved{std::move(u).error()};
    BOOST_CHECK_EQUAL(*moved, -2);
}

BOOST_AUTO_TEST_CASE(expected_swap)
{
    Expected<const char*, std::unique_ptr<int>> a{Unexpected{std::make_unique<int>(-1)}};
    Expected<const char*, std::unique_ptr<int>> b{"good"};
    a.swap(b);
    BOOST_CHECK_EQUAL(a.value(), "good");
    BOOST_CHECK_EQUAL(*b.error(), -1);
}

BOOST_AUTO_TEST_SUITE_END()
