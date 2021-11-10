/*!
@file
Forward declares `boost::hana::detail::nested_by`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_NESTED_BY_FWD_HPP
#define BOOST_HANA_DETAIL_NESTED_BY_FWD_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename Algorithm>
    struct nested_by_t {
        template <typename Predicate, typename Object>
        constexpr decltype(auto)
        operator()(Predicate&& predicate, Object&& object) const;

        template <typename Predicate>
        constexpr decltype(auto) operator()(Predicate&& predicate) const;
    };

    //! @ingroup group-details
    //! Provides a `.by` static constexpr function object.
    //!
    //! When creating a binary function object of type `Algorithm` whose
    //! signature is `Object x Predicate -> Return`, `nested_by<Algorithm>`
    //! can be used as a base class to `Algorithm`. Doing so will provide a
    //! static constexpr member called `by`, which has the two following
    //! signatures:
    //! @code
    //!     Predicate x Object -> Return
    //!     Predicate -> (Object -> Return)
    //! @endcode
    //!
    //! In other words, `nested_by` is a `curry`ed and `flip`ped version of
    //! `Algorithm`. Note that the function object `Algorithm` must be
    //! default-constructible, since the algorithm will be called as
    //! `Algorithm{}(arguments...)`.
    //!
    //! @note
    //! This function object is especially useful because it takes care of
    //! avoiding ODR violations caused by the nested static constexpr member.
    template <typename Algorithm>
    struct nested_by { static constexpr nested_by_t<Algorithm> by{}; };

    template <typename Algorithm>
    constexpr nested_by_t<Algorithm> nested_by<Algorithm>::by;
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_NESTED_BY_FWD_HPP
