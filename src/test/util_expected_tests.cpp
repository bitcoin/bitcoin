// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <test/util/setup_common.h>
#include <util/expected.h>

#include <boost/test/unit_test.hpp>

using namespace util;

BOOST_AUTO_TEST_SUITE(util_expected_tests)

BOOST_AUTO_TEST_CASE(expected_value)
{
    struct Obj {
        int x;
    };
    Expected<Obj, int> e{};
    BOOST_CHECK(e.value().x == 0);

    e = Obj{42};

    BOOST_CHECK(e.has_value());
    BOOST_CHECK(static_cast<bool>(e));
    BOOST_CHECK(e.value().x == 42);
    BOOST_CHECK((*e).x == 42);
    BOOST_CHECK(e->x == 42);

    // modify value
    e.value().x += 1;
    (*e).x += 1;
    e->x += 1;

    const auto& read{e};
    BOOST_CHECK(read.value().x == 45);
    BOOST_CHECK((*read).x == 45);
    BOOST_CHECK(read->x == 45);
}

BOOST_AUTO_TEST_CASE(expected_value_or)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(1)};
    const int one{*std::move(no_copy).value_or(std::make_unique<int>(2))};
    BOOST_CHECK_EQUAL(one, 1);

    const Expected<std::string, int> const_val{Unexpected{-1}};
    BOOST_CHECK_EQUAL(const_val.value_or("fallback"), "fallback");
}

BOOST_AUTO_TEST_CASE(expected_error)
{
    Expected<void, std::string> e{};
    BOOST_CHECK(e.has_value());

    e = Unexpected{"fail"};
    BOOST_CHECK(!e.has_value());
    BOOST_CHECK(!static_cast<bool>(e));
    BOOST_CHECK(e.error() == "fail");

    // modify error
    e.error() += "1";

    const auto& read{e};
    BOOST_CHECK(read.error() == "fail1");
}

BOOST_AUTO_TEST_SUITE_END()
