// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>

#include <test/util/framework.h>
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

BOOST_AUTO_TEST_CASE(same_span)
{
    const std::vector<int> a{1, 2, 3}, b{1, 2, 3};
    const std::span<const int> span_a{a}, span_b{b};
    // Spans over the same memory are the same, but spans over equal contents elsewhere are not.
    BOOST_CHECK(SameSpan(span_a, std::span<const int>{a}));
    BOOST_CHECK(!SameSpan(span_a, span_b));
    // Both the start and the size matter.
    BOOST_CHECK(!SameSpan(span_a, span_a.first(2)));
    BOOST_CHECK(!SameSpan(span_a.first(2), span_a.last(2)));
    BOOST_CHECK(SameSpan(span_a.subspan(1), span_a.last(2)));
    // Empty spans are the same only if they are in the same place.
    BOOST_CHECK(SameSpan(span_a.first(0), span_a.first(0)));
    BOOST_CHECK(!SameSpan(span_a.first(0), span_a.last(0)));
    BOOST_CHECK(SameSpan(std::span<const int>{}, std::span<const int>{}));
}

BOOST_AUTO_TEST_SUITE_END()
