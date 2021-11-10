/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   logical.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains logical predicates for value comparison, analogous to \c std::less, \c std::greater
 * and others. The main difference from the standard equivalents is that the predicates defined in this
 * header are not templates and therefore do not require a fixed argument type. Furthermore, both arguments
 * may have different types, in which case the comparison is performed without type conversion.
 *
 * \note In case if arguments are integral, the conversion is performed according to the standard C++ rules
 *       in order to avoid warnings from the compiler.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_LOGICAL_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_LOGICAL_HPP_INCLUDED_

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The trait creates a common integral type suitable for comparison. This is mostly to silence compiler warnings like 'signed/unsigned mismatch'.
template< typename T, typename U, unsigned int TSizeV = sizeof(T), unsigned int USizeV = sizeof(U), bool TSmallerThanU = (sizeof(T) < sizeof(U)) >
struct make_common_integral_type
{
    typedef T type;
};

//! Specialization for case when \c T is smaller than \c U
template< typename T, typename U, unsigned int TSizeV, unsigned int USizeV >
struct make_common_integral_type< T, U, TSizeV, USizeV, true >
{
    typedef U type;
};

//! Specialization for the case when both types have the same size
template< typename T, typename U, unsigned int SizeV >
struct make_common_integral_type< T, U, SizeV, SizeV, false > :
    public boost::conditional<
        is_unsigned< T >::value,
        T,
        U
    >
{
};

} // namespace aux

//! Equality predicate
struct equal_to
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left == right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) == static_cast< common_integral_type >(right);
    }
};

//! Inequality predicate
struct not_equal_to
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left != right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) != static_cast< common_integral_type >(right);
    }
};

//! Less predicate
struct less
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left < right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) < static_cast< common_integral_type >(right);
    }
};

//! Greater predicate
struct greater
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left > right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) > static_cast< common_integral_type >(right);
    }
};

//! Less or equal predicate
struct less_equal
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left <= right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) <= static_cast< common_integral_type >(right);
    }
};

//! Greater or equal predicate
struct greater_equal
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, integral_constant< bool, is_integral< T >::value && is_integral< U >::value >());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, false_type)
    {
        return (left >= right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, true_type)
    {
        typedef typename aux::make_common_integral_type< T, U >::type common_integral_type;
        return static_cast< common_integral_type >(left) >= static_cast< common_integral_type >(right);
    }
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_LOGICAL_HPP_INCLUDED_
