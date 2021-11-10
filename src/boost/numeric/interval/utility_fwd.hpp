/* Boost interval/utility.hpp template interface file
 *
 * Copyright 2013 Cromwell D. Enage
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_UTILITY_FWD_HPP
#define BOOST_NUMERIC_INTERVAL_UTILITY_FWD_HPP

#include <boost/config.hpp>
#include <boost/numeric/interval/detail/interval_prototype.hpp>
#include <utility>

/*
 * Interface of simple functions
 */

namespace boost { namespace numeric {

    /*
     * Utility Functions
     */

    template<class T, class Policies>
    const T& lower(const interval<T,Policies>& x);

    template<class T, class Policies>
    const T& upper(const interval<T,Policies>& x);

    template<class T, class Policies>
    T checked_lower(const interval<T,Policies>& x);

    template<class T, class Policies>
    T width(const interval<T,Policies>& x);

    template<class T, class Policies>
    T median(const interval<T,Policies>& x);

    template<class T, class Policies>
    interval<T,Policies> widen(const interval<T,Policies>& x, const T& v);

    /*
     * Set-like operations
     */

    template <class T, class Policies>
    bool empty(const interval<T,Policies>& x);

    template <class T, class Policies>
    bool zero_in(const interval<T,Policies>& x);

    template <class T, class Policies>
    bool in_zero(const interval<T,Policies>& x);  // DEPRECATED

    template <class T, class Policies>
    bool in(const T& x, const interval<T,Policies>& y);

    template <class T, class Policies>
    bool
        subset(
            const interval<T,Policies>& x
          , const interval<T,Policies>& y
        );

    template <class T, class Policies1, class Policies2>
    bool
        proper_subset(
            const interval<T,Policies1>& x
          , const interval<T,Policies2>& y
        );

    template <class T, class Policies1, class Policies2>
    bool
        overlap(
            const interval<T,Policies1>& x
          , const interval<T,Policies2>& y
        );

    template <class T, class Policies>
    bool singleton(const interval<T, Policies>& x);

    template <class T, class Policies1, class Policies2>
    bool
        equal(
            const interval<T,Policies1>& x
          , const interval<T,Policies2>& y
        );

    template <class T, class Policies>
    interval<T, Policies>
        intersect(
            const interval<T,Policies>& x
          , const interval<T,Policies>& y
        );

    template <class T, class Policies>
    interval<T, Policies>
        hull(const interval<T,Policies>& x, const interval<T,Policies>& y);

    template <class T, class Policies>
    interval<T, Policies>
        hull(const interval<T,Policies>& x, const T& y);

    template <class T, class Policies>
    interval<T, Policies>
        hull(const T& x, const interval<T,Policies>& y);

    template <class T>
    interval<T> hull(const T& x, const T& y);

    template <class T, class Policies>
    std::pair<interval<T,Policies>,interval<T,Policies> >
        bisect(const interval<T,Policies>& x);

    /*
     * Elementary functions
     */

    template <class T, class Policies>
    T norm(const interval<T,Policies>& x);

    template <class T, class Policies>
    interval<T,Policies> abs(const interval<T,Policies>& x);

    template <class T, class Policies>
    interval<T,Policies>
        max BOOST_PREVENT_MACRO_SUBSTITUTION (
            const interval<T,Policies>& x
          , const interval<T,Policies>& y
        );

    template <class T, class Policies>
    interval<T,Policies>
        max BOOST_PREVENT_MACRO_SUBSTITUTION (
            const interval<T,Policies>& x
          , const T& y
        );

    template <class T, class Policies>
    interval<T,Policies>
        max BOOST_PREVENT_MACRO_SUBSTITUTION (
            const T& x
          , const interval<T,Policies>& y
        );

    template <class T, class Policies>
    interval<T,Policies>
        min BOOST_PREVENT_MACRO_SUBSTITUTION (
            const interval<T,Policies>& x
          , const interval<T,Policies>& y
        );

    template <class T, class Policies>
    interval<T,Policies>
        min BOOST_PREVENT_MACRO_SUBSTITUTION (
            const interval<T,Policies>& x
          , const T& y
        );

    template <class T, class Policies>
    interval<T,Policies>
        min BOOST_PREVENT_MACRO_SUBSTITUTION (
            const T& x
          , const interval<T,Policies>& y
        );
}} // namespace boost::numeric

#endif  // include guard

