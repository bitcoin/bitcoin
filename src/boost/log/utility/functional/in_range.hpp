/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   in_range.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains a predicate for checking if the provided value is within a half-open range.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_IN_RANGE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_IN_RANGE_HPP_INCLUDED_

#include <utility>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/functional/logical.hpp> // make_common_integral_type
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

//! The in_range functor
struct in_range_fun
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& value, std::pair< U, U > const& rng) const
    {
        return op(value, rng, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& value, std::pair< U, U > const& rng, false_type)
    {
        return (value >= rng.first && value < rng.second);
    }
    template< typename T, typename U >
    static bool op(T const& value, std::pair< U, U > const& rng, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return (static_cast< common_integral_type >(value) >= static_cast< common_integral_type >(rng.first))
            && (static_cast< common_integral_type >(value) < static_cast< common_integral_type >(rng.second));
    }
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_IN_RANGE_HPP_INCLUDED_
