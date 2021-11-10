/*!
@file
Forward declares `boost::hana::set`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SET_HPP
#define BOOST_HANA_FWD_SET_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/to.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/erase_key.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-datatypes
    //! Basic unordered container requiring unique, `Comparable` and
    //! `Hashable` keys.
    //!
    //! A set is an unordered container that can hold heterogeneous keys.
    //! A set requires (and ensures) that no duplicates are present when
    //! inserting new keys.
    //!
    //! @note
    //! The actual representation of a `hana::set` is implementation-defined.
    //! In particular, one should not take for granted the order of the
    //! template parameters and the presence of any additional constructors
    //! or assignment operators than what is documented. The canonical way of
    //! creating a `hana::set` is through `hana::make_set`. More details
    //! [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Comparable`\n
    //! Two sets are equal iff they contain the same elements, regardless of
    //! the order.
    //! @include example/set/comparable.cpp
    //!
    //! 2. Foldable\n
    //! Folding a set is equivalent to folding the sequence of its values.
    //! However, note that the values are not required to be in any specific
    //! order, so using the folds provided here with an operation that is not
    //! both commutative and associative will yield non-deterministic behavior.
    //! @include example/set/foldable.cpp
    //!
    //! 3. Searchable\n
    //! The elements in a set act as both its keys and its values. Since the
    //! elements of a set are unique, searching for an element will return
    //! either the only element which is equal to the searched value, or
    //! `nothing`. Also note that `operator[]` can be used instead of the
    //! `at_key` function.
    //! @include example/set/searchable.cpp
    //!
    //!
    //! Conversion from any `Foldable`
    //! ------------------------------
    //! Any `Foldable` structure can be converted into a `hana::set` with
    //! `to<set_tag>`. The elements of the structure must all be compile-time
    //! `Comparable`. If the structure contains duplicate elements, only
    //! the first occurence will appear in the resulting set. More
    //! specifically, conversion from a `Foldable` is equivalent to
    //! @code
    //!     to<set_tag>(xs) == fold_left(xs, make_set(), insert)
    //! @endcode
    //!
    //! __Example__
    //! @include example/set/to.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename implementation_defined>
    struct set {
        //! Default-construct a set. This constructor only exists when all the
        //! elements of the set are default-constructible.
        constexpr set() = default;

        //! Copy-construct a set from another set. This constructor only
        //! exists when all the elements of the set are copy-constructible.
        constexpr set(set const& other) = default;

        //! Move-construct a set from another set. This constructor only
        //! exists when all the elements of the set are move-constructible.
        constexpr set(set&& other) = default;

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

        //! Equivalent to `hana::at_key`
        template <typename Key>
        constexpr decltype(auto) operator[](Key&& key);
    };
#else
    template <typename ...Xs>
    struct set;
#endif

    //! Tag representing the `hana::set` container.
    //! @relates hana::set
    struct set_tag { };

    //! Function object for creating a `hana::set`.
    //! @relates hana::set
    //!
    //! Given zero or more values `xs...`, `make<set_tag>` returns a `set`
    //! containing those values. The values must all be compile-time
    //! `Comparable`, and no duplicate values may be provided. To create
    //! a `set` from a sequence with possible duplicates, use `to<set_tag>`
    //! instead.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/make.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <>
    constexpr auto make<set_tag> = [](auto&& ...xs) {
        return set<implementation_defined>{forwarded(xs)...};
    };
#endif

    //! Equivalent to `make<set_tag>`; provided for convenience.
    //! @relates hana::set
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/make.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_set = make<set_tag>;

    //! Insert an element in a `hana::set`.
    //! @relates hana::set
    //!
    //! If the set already contains an element that compares equal, then
    //! nothing is done and the set is returned as is.
    //!
    //!
    //! @param set
    //! The set in which to insert a value.
    //!
    //! @param element
    //! The value to insert. It must be compile-time `Comparable`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/insert.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto insert = [](auto&& set, auto&& element) {
        return tag-dispatched;
    };
#endif

    //! Remove an element from a `hana::set`.
    //! @relates hana::set
    //!
    //! Returns a new set containing all the elements of the original,
    //! except the one comparing `equal` to the given element. If the set
    //! does not contain such an element, a new set equal to the original
    //! set is returned.
    //!
    //!
    //! @param set
    //! The set in which to remove a value.
    //!
    //! @param element
    //! The value to remove. It must be compile-time `Comparable`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/erase_key.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto erase_key = [](auto&& set, auto&& element) {
        return tag-dispatched;
    };
#endif

    //! Returns the union of two sets.
    //! @relates hana::set
    //!
    //! Given two sets `xs` and `ys`, `union_(xs, ys)` is a new set containing
    //! all the elements of `xs` and all the elements of `ys`, without
    //! duplicates. For any object `x`, the following holds: `x` is in
    //! `hana::union_(xs, ys)` if and only if `x` is in `xs` or `x` is in `ys`.
    //!
    //!
    //! @param xs, ys
    //! Two sets to compute the union of.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/union.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto union_ = [](auto&& xs, auto&& ys) {
        return tag-dispatched;
    };
#endif

    //! Returns the intersection of two sets.
    //! @relates hana::set
    //!
    //! Given two sets `xs` and `ys`, `intersection(xs, ys)` is a new set
    //! containing exactly those elements that are present both in `xs` and
    //! in `ys`.
    //! In other words, the following holds for any object `x`:
    //! @code
    //!     x ^in^ intersection(xs, ys) if and only if x ^in^ xs && x ^in^ ys
    //! @endcode
    //!
    //!
    //! @param xs, ys
    //! Two sets to intersect.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/intersection.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto intersection = [](auto&& xs, auto&& ys) {
        return tag-dispatched;
    };
#endif
    //! Equivalent to `to<set_tag>`; provided for convenience.
    //! @relates hana::set
    constexpr auto to_set = to<set_tag>;

    //! Returns the set-theoretic difference of two sets.
    //! @relates hana::set
    //!
    //! Given two sets `xs` and `ys`, `difference(xs, ys)` is a new set
    //! containing all the elements of `xs` that are _not_ contained in `ys`.
    //! For any object `x`, the following holds:
    //! @code
    //!     x ^in^ difference(xs, ys) if and only if x ^in^ xs && !(x ^in^ ys)
    //! @endcode
    //!
    //!
    //! This operation is not commutative, i.e. `difference(xs, ys)` is not
    //! necessarily the same as `difference(ys, xs)`. Indeed, consider the
    //! case where `xs` is empty and `ys` isn't. Then, `difference(xs, ys)`
    //! is empty but `difference(ys, xs)` is equal to `ys`. For the symmetric
    //! version of this operation, see `symmetric_difference`.
    //!
    //!
    //! @param xs
    //! A set param to remove values from.
    //!
    //! @param ys
    //! The set whose values are removed from `xs`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/difference.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto difference = [](auto&& xs, auto&& ys) {
        return tag-dispatched;
};
#endif

    //! Returns the symmetric set-theoretic difference of two sets.
    //! @relates hana::set
    //!
    //! Given two sets `xs` and `ys`, `symmetric_difference(xs, ys)` is a new
    //! set containing all the elements of `xs` that are not contained in `ys`,
    //! and all the elements of `ys` that are not contained in `xs`. The
    //! symmetric difference of two sets satisfies the following:
    //! @code
    //!     symmetric_difference(xs, ys) == union_(difference(xs, ys), difference(ys, xs))
    //! @endcode
    //!
    //!
    //! @param xs, ys
    //! Two sets to compute the symmetric difference of.
    //!
    //!
    //! Example
    //! -------
    //! @include example/set/symmetric_difference.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
constexpr auto symmetric_difference = [](auto&& xs, auto&& ys) {
        return tag-dispatched;
    };
#endif


BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SET_HPP
