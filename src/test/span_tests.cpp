// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>

#include <boost/test/unit_test.hpp>
#include <array>
#include <set>
#include <vector>

namespace spannable {
struct Ignore
{
    template<typename T> Ignore(T&&) {}
};
template<typename T>
bool Spannable(T&& value, decltype(std::span{value})* enable = nullptr)
{
    return true;
}
bool Spannable(Ignore)
{
    return false;
}

struct SpannableYes
{
    int* data();
    int* begin();
    int* end();
    size_t size();
};
struct SpannableNo
{
    void data();
    size_t size();
};
} // namespace spannable

using namespace spannable;

BOOST_AUTO_TEST_SUITE(span_tests)

// Make sure template std::span template deduction guides accurately enable calls to
// std::span constructor overloads that work, and disable calls to constructor overloads that
// don't work. This makes it possible to use the std::span constructor in a SFINAE
// contexts like in the Spannable function above to detect whether types are or
// aren't compatible with std::span at compile time.
BOOST_AUTO_TEST_CASE(span_constructor_sfinae)
{
    BOOST_CHECK(Spannable(std::vector<int>{}));
    BOOST_CHECK(!Spannable(std::set<int>{}));
    BOOST_CHECK(!Spannable(std::vector<bool>{}));
    BOOST_CHECK(Spannable(std::array<int, 3>{}));
    BOOST_CHECK(Spannable(std::span<int>{}));
    BOOST_CHECK(Spannable("char array"));
    BOOST_CHECK(Spannable(SpannableYes{}));
    BOOST_CHECK(!Spannable(SpannableNo{}));
}

BOOST_AUTO_TEST_SUITE_END()
