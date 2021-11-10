/*!
@file
Forward declares `boost::hana::range`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_RANGE_HPP
#define BOOST_HANA_FWD_RANGE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/integral_constant.hpp>


BOOST_HANA_NAMESPACE_BEGIN
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! @ingroup group-datatypes
    //! Compile-time half-open interval of `hana::integral_constant`s.
    //!
    //! A `range` represents a half-open interval of the form `[from, to)`
    //! containing `hana::integral_constant`s of a given type. The `[from, to)`
    //! notation represents the values starting at `from` (inclusively) up
    //! to but excluding `from`. In other words, it is a bit like the list
    //! `from, from+1, ..., to-1`.
    //!
    //! In particular, note that the bounds of the range can be any
    //! `hana::integral_constant`s (negative numbers are allowed) and the
    //! range does not have to start at zero. The only requirement is that
    //! `from <= to`.
    //!
    //! @note
    //! The representation of `hana::range` is implementation defined. In
    //! particular, one should not take for granted the number and types
    //! of template parameters. The proper way to create a `hana::range`
    //! is to use `hana::range_c` or `hana::make_range`. More details
    //! [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Comparable`\n
    //! Two ranges are equal if and only if they are both empty or they both
    //! span the same interval.
    //! @include example/range/comparable.cpp
    //!
    //! 2. `Foldable`\n
    //! Folding a `range` is equivalent to folding a list of the
    //! `integral_constant`s in the interval it spans.
    //! @include example/range/foldable.cpp
    //!
    //! 3. `Iterable`\n
    //! Iterating over a `range` is equivalent to iterating over a list of
    //! the values it spans. In other words, iterating over the range
    //! `[from, to)` is equivalent to iterating over a list containing
    //! `from, from+1, from+2, ..., to-1`. Also note that `operator[]` can
    //! be used in place of the `at` function.
    //! @include example/range/iterable.cpp
    //!
    //! 4. `Searchable`\n
    //! Searching a `range` is equivalent to searching a list of the values
    //! in the range `[from, to)`, but it is much more compile-time efficient.
    //! @include example/range/searchable.cpp
    template <typename T, T from, T to>
    struct range {
        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

        //! Equivalent to `hana::at`
        template <typename N>
        constexpr decltype(auto) operator[](N&& n);
    };
#else
    template <typename T, T from, T to>
    struct range;
#endif

    //! Tag representing a `hana::range`.
    //! @relates hana::range
    struct range_tag { };

#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! Create a `hana::range` representing a half-open interval of
    //! `integral_constant`s.
    //! @relates hana::range
    //!
    //! Given two `IntegralConstant`s `from` and `to`, `make<range_tag>`
    //! returns a `hana::range` representing the half-open interval of
    //! `integral_constant`s `[from, to)`. `from` and `to` must form a
    //! valid interval, which means that `from <= to` must be true. Otherwise,
    //! a compilation error is triggered. Also note that if `from` and `to`
    //! are `IntegralConstant`s with different underlying integral types,
    //! the created range contains `integral_constant`s whose underlying
    //! type is their common type.
    //!
    //!
    //! Example
    //! -------
    //! @include example/range/make.cpp
    template <>
    constexpr auto make<range_tag> = [](auto const& from, auto const& to) {
        return range<implementation_defined>{implementation_defined};
    };
#endif

    //! Alias to `make<range_tag>`; provided for convenience.
    //! @relates hana::range
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_range = make<range_tag>;

    //! Shorthand to create a `hana::range` with the given bounds.
    //! @relates hana::range
    //!
    //! This shorthand is provided for convenience only and it is equivalent
    //! to `make_range`. Specifically, `range_c<T, from, to>` is such that
    //! @code
    //!     range_c<T, from, to> == make_range(integral_c<T, from>, integral_c<T, to>)
    //! @endcode
    //!
    //!
    //! @tparam T
    //! The underlying integral type of the `integral_constant`s in the
    //! created range.
    //!
    //! @tparam from
    //! The inclusive lower bound of the created range.
    //!
    //! @tparam to
    //! The exclusive upper bound of the created range.
    //!
    //!
    //! Example
    //! -------
    //! @include example/range/range_c.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, T from, T to>
    constexpr auto range_c = make_range(integral_c<T, from>, integral_c<T, to>);
#else
    template <typename T, T from, T to>
    BOOST_HANA_INLINE_VARIABLE constexpr range<T, from, to> range_c{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_RANGE_HPP
