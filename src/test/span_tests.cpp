// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>

#include <boost/test/unit_test.hpp>
#include <array>
#include <set>
#include <vector>

namespace {
struct Ignore
{
    template<typename T> Ignore(T&&) {}
};
template<typename T>
bool Spannable(T&& value, decltype(Span{value})* enable = nullptr)
{
    return true;
}
bool Spannable(Ignore)
{
    return false;
}

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunneeded-member-function"
#    pragma clang diagnostic ignored "-Wunused-member-function"
#endif
struct SpannableYes
{
    int* data();
    size_t size();
};
struct SpannableNo
{
    void* data();
    size_t size();
};
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
} // namespace

BOOST_AUTO_TEST_SUITE(span_tests)

// Make sure template Span template deduction guides accurately enable calls to
// Span constructor overloads that work, and disable calls to constructor overloads that
// don't work. This makes it is possible to use the Span constructor in a SFINAE
// contexts like in the Spannable function above to detect whether types are or
// aren't compatible with Spans at compile time.
//
// Previously there was a bug where writing a SFINAE check for vector<bool> was
// not possible, because in libstdc++ vector<bool> has a data() member
// returning void*, and the Span template guide ignored the data() return value,
// so the template substitution would succeed, but the constructor would fail,
// resulting in a fatal compile error, rather than a SFINAE error that could be
// handled.
BOOST_AUTO_TEST_CASE(span_constructor_sfinae)
{
    BOOST_CHECK(Spannable(std::vector<int>{}));
    BOOST_CHECK(!Spannable(std::set<int>{}));
    BOOST_CHECK(!Spannable(std::vector<bool>{}));
    BOOST_CHECK(Spannable(std::array<int, 3>{}));
    BOOST_CHECK(Spannable(Span<int>{}));
    BOOST_CHECK(Spannable("char array"));
    BOOST_CHECK(Spannable(SpannableYes{}));
    BOOST_CHECK(!Spannable(SpannableNo{}));
}

BOOST_AUTO_TEST_SUITE_END()
