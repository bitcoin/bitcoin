/*!
@file
Forward declares `boost::hana::pair`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PAIR_HPP
#define BOOST_HANA_FWD_PAIR_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-datatypes
    //! Generic container for two elements.
    //!
    //! `hana::pair` is conceptually the same as `std::pair`. However,
    //! `hana::pair` automatically compresses the storage of empty types,
    //! and as a result it does not have the `.first` and `.second` members.
    //! Instead, one must use the `hana::first` and `hana::second` free
    //! functions to access the elements of a pair.
    //!
    //! @note
    //! When you use a container, remember not to make assumptions about its
    //! representation, unless the documentation gives you those guarantees.
    //! More details [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Comparable`\n
    //! Two pairs `(x, y)` and `(x', y')` are equal if and only if both
    //! `x == x'` and `y == y'`.
    //! @include example/pair/comparable.cpp
    //!
    //! 2. `Orderable`\n
    //! Pairs are ordered as-if they were 2-element tuples, using a
    //! lexicographical ordering.
    //! @include example/pair/orderable.cpp
    //!
    //! 3. `Foldable`\n
    //! Folding a pair is equivalent to folding a 2-element tuple. In other
    //! words:
    //! @code
    //!     fold_left(make_pair(x, y), s, f) == f(f(s, x), y)
    //!     fold_right(make_pair(x, y), s, f) == f(x, f(y, s))
    //! @endcode
    //! Example:
    //! @include example/pair/foldable.cpp
    //!
    //! 4. `Product`\n
    //! The model of `Product` is the simplest one possible; the first element
    //! of a pair `(x, y)` is `x`, and its second element is `y`.
    //! @include example/pair/product.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename First, typename Second>
    struct pair {
        //! Default constructs the `pair`. Only exists when both elements
        //! of the pair are default constructible.
        constexpr pair();

        //! Initialize each element of the pair with the corresponding element.
        //! Only exists when both elements of the pair are copy-constructible.
        constexpr pair(First const& first, Second const& second);

        //! Initialize both elements of the pair by perfect-forwarding the
        //! corresponding argument. Only exists when both arguments are
        //! implicitly-convertible to the corresponding element of the pair.
        template <typename T, typename U>
        constexpr pair(T&& t, U&& u);

        //! Copy-initialize a pair from another pair. Only exists when both
        //! elements of the source pair are implicitly convertible to the
        //! corresponding element of the constructed pair.
        template <typename T, typename U>
        constexpr pair(pair<T, U> const& other);

        //! Move-initialize a pair from another pair. Only exists when both
        //! elements of the source pair are implicitly convertible to the
        //! corresponding element of the constructed pair.
        template <typename T, typename U>
        constexpr pair(pair<T, U>&& other);

        //! Assign a pair to another pair. Only exists when both elements
        //! of the destination pair are assignable from the corresponding
        //! element in the source pair.
        template <typename T, typename U>
        constexpr pair& operator=(pair<T, U> const& other);

        //! Move-assign a pair to another pair. Only exists when both elements
        //! of the destination pair are move-assignable from the corresponding
        //! element in the source pair.
        template <typename T, typename U>
        constexpr pair& operator=(pair<T, U>&& other);

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

        //! Equivalent to `hana::less`
        template <typename X, typename Y>
        friend constexpr auto operator<(X&& x, Y&& y);

        //! Equivalent to `hana::greater`
        template <typename X, typename Y>
        friend constexpr auto operator>(X&& x, Y&& y);

        //! Equivalent to `hana::less_equal`
        template <typename X, typename Y>
        friend constexpr auto operator<=(X&& x, Y&& y);

        //! Equivalent to `hana::greater_equal`
        template <typename X, typename Y>
        friend constexpr auto operator>=(X&& x, Y&& y);
    };
#else
    template <typename First, typename Second>
    struct pair;
#endif

    //! Tag representing `hana::pair`.
    //! @relates hana::pair
    struct pair_tag { };

#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! Creates a `hana::pair` with the given elements.
    //! @relates hana::pair
    //!
    //!
    //! Example
    //! -------
    //! @include example/pair/make.cpp
    template <>
    constexpr auto make<pair_tag> = [](auto&& first, auto&& second)
        -> hana::pair<std::decay_t<decltype(first)>, std::decay_t<decltype(second)>>
    {
        return {forwarded(first), forwarded(second)};
    };
#endif

    //! Alias to `make<pair_tag>`; provided for convenience.
    //! @relates hana::pair
    //!
    //! Example
    //! -------
    //! @include example/pair/make.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_pair = make<pair_tag>;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PAIR_HPP
