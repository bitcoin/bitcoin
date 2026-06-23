// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/pointer_tag_pair.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(pointer_tag_pair_tests)

namespace {
struct alignas(4) Aligned {
    int x;
};
} // namespace

BOOST_AUTO_TEST_CASE(default_is_empty)
{
    PointerTagPair<Aligned, 2> p;
    BOOST_CHECK(p.empty());
    BOOST_CHECK_EQUAL(p.pointer(), nullptr);
    BOOST_CHECK_EQUAL(p.tag(), 0u);
}

BOOST_AUTO_TEST_CASE(pointer_and_tag_are_independent)
{
    Aligned a{}, b{};
    PointerTagPair<Aligned, 2> p;

    // Setting the pointer leaves the tag untouched and vice versa.
    p.set_pointer(&a);
    BOOST_CHECK_EQUAL(p.pointer(), &a);
    BOOST_CHECK_EQUAL(p.tag(), 0u);
    BOOST_CHECK(!p.empty());

    p.set_tag(3);
    BOOST_CHECK_EQUAL(p.pointer(), &a); // pointer preserved across tag change
    BOOST_CHECK_EQUAL(p.tag(), 3u);

    p.set_pointer(&b);
    BOOST_CHECK_EQUAL(p.pointer(), &b); // tag preserved across pointer change
    BOOST_CHECK_EQUAL(p.tag(), 3u);

    // Tag is replaced (not OR-ed) and only the low Bits are kept.
    p.set_tag(1);
    BOOST_CHECK_EQUAL(p.tag(), 1u);
    BOOST_CHECK_EQUAL(p.pointer(), &b);
}

BOOST_AUTO_TEST_CASE(tag_is_masked_to_bits)
{
    Aligned a{};
    PointerTagPair<Aligned, 2> p;
    p.set_pointer(&a);
    p.set_tag(0xFF); // only the low 2 bits may be stored
    BOOST_CHECK_EQUAL(p.tag(), 0x3u);
    BOOST_CHECK_EQUAL(p.pointer(), &a); // high bits did not leak into the pointer
}

BOOST_AUTO_TEST_CASE(clear_resets_both)
{
    Aligned a{};
    PointerTagPair<Aligned, 2> p;
    p.set_pointer(&a);
    p.set_tag(2);
    p.clear();
    BOOST_CHECK(p.empty());
    BOOST_CHECK_EQUAL(p.pointer(), nullptr);
    BOOST_CHECK_EQUAL(p.tag(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
