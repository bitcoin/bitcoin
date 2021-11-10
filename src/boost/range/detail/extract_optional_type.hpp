// Boost.Range library
//
//  Copyright Arno Schoedl & Neil Groves 2009.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_EXTRACT_OPTIONAL_TYPE_HPP_INCLUDED
#define BOOST_RANGE_DETAIL_EXTRACT_OPTIONAL_TYPE_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/has_xxx.hpp>

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX)

// Defines extract_some_typedef<T> which exposes T::some_typedef as
// extract_some_typedef<T>::type if T::some_typedef exists. Otherwise
// extract_some_typedef<T> is empty.
#define BOOST_RANGE_EXTRACT_OPTIONAL_TYPE( a_typedef )                          \
    BOOST_MPL_HAS_XXX_TRAIT_DEF(a_typedef)                                      \
    template< typename  C, bool B = BOOST_PP_CAT(has_, a_typedef)<C>::value >   \
    struct BOOST_PP_CAT(extract_, a_typedef)                                    \
    {};                                                                         \
    template< typename C >                                                      \
    struct BOOST_PP_CAT(extract_, a_typedef)< C, true >                         \
    {                                                                           \
        typedef BOOST_DEDUCED_TYPENAME C::a_typedef type;                       \
    };

#else

#define BOOST_RANGE_EXTRACT_OPTIONAL_TYPE( a_typedef )                          \
    template< typename C >                                                      \
    struct BOOST_PP_CAT(extract_, a_typedef)                                    \
    {                                                                           \
        typedef BOOST_DEDUCED_TYPENAME C::a_typedef type;                       \
    };

#endif

#endif // include guard
