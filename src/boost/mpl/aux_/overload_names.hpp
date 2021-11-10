
#ifndef BOOST_MPL_AUX_OVERLOAD_NAMES_HPP_INCLUDED
#define BOOST_MPL_AUX_OVERLOAD_NAMES_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/aux_/ptr_to_ref.hpp>
#include <boost/mpl/aux_/config/operators.hpp>

#if defined(BOOST_MPL_CFG_USE_OPERATORS_OVERLOADING)

#   include <boost/mpl/aux_/static_cast.hpp>

#   define BOOST_MPL_AUX_OVERLOAD_VALUE_BY_KEY  operator/
#   define BOOST_MPL_AUX_OVERLOAD_ITEM_BY_ORDER operator|
#   define BOOST_MPL_AUX_OVERLOAD_ORDER_BY_KEY  operator||
#   define BOOST_MPL_AUX_OVERLOAD_IS_MASKED     operator%

#   define BOOST_MPL_AUX_OVERLOAD_CALL_VALUE_BY_KEY(T, x)   BOOST_MPL_AUX_PTR_TO_REF(T) / x
#   define BOOST_MPL_AUX_OVERLOAD_CALL_ITEM_BY_ORDER(T, x)  BOOST_MPL_AUX_PTR_TO_REF(T) | x
#   define BOOST_MPL_AUX_OVERLOAD_CALL_ORDER_BY_KEY(T, x)   BOOST_MPL_AUX_PTR_TO_REF(T) || x
#   define BOOST_MPL_AUX_OVERLOAD_CALL_IS_MASKED(T, x)      BOOST_MPL_AUX_PTR_TO_REF(T) % x

#else

#   define BOOST_MPL_AUX_OVERLOAD_VALUE_BY_KEY  value_by_key_
#   define BOOST_MPL_AUX_OVERLOAD_ITEM_BY_ORDER item_by_order_
#   define BOOST_MPL_AUX_OVERLOAD_ORDER_BY_KEY  order_by_key_
#   define BOOST_MPL_AUX_OVERLOAD_IS_MASKED     is_masked_

#   define BOOST_MPL_AUX_OVERLOAD_CALL_VALUE_BY_KEY(T, x)   T::BOOST_MPL_AUX_OVERLOAD_VALUE_BY_KEY( BOOST_MPL_AUX_PTR_TO_REF(T), x )
#   define BOOST_MPL_AUX_OVERLOAD_CALL_ITEM_BY_ORDER(T, x)  T::BOOST_MPL_AUX_OVERLOAD_ITEM_BY_ORDER( BOOST_MPL_AUX_PTR_TO_REF(T), x )
#   define BOOST_MPL_AUX_OVERLOAD_CALL_ORDER_BY_KEY(T, x)   T::BOOST_MPL_AUX_OVERLOAD_ORDER_BY_KEY( BOOST_MPL_AUX_PTR_TO_REF(T), x )
#   define BOOST_MPL_AUX_OVERLOAD_CALL_IS_MASKED(T, x)      T::BOOST_MPL_AUX_OVERLOAD_IS_MASKED( BOOST_MPL_AUX_PTR_TO_REF(T), x )

#endif

#endif // BOOST_MPL_AUX_OVERLOAD_NAMES_HPP_INCLUDED
