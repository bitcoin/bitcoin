
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_HPP)
#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/detail/equal_type.hpp>
#include <boost/vmd/detail/is_array_common.hpp>
#include <boost/vmd/detail/is_list.hpp>
#include <boost/vmd/detail/type_registration.hpp>

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_ARRAY(data,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX(data), \
        BOOST_VMD_TYPE_ARRAY, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_ARRAY_D(d,data,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX_D(d,data), \
        BOOST_VMD_TYPE_ARRAY, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_LIST(data,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_LIST_WLOOP(data), \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_LIST), \
            BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_ARRAY \
            ) \
        (data,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_LIST_D(d,data,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_LIST_WLOOP_D(d,data), \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_LIST), \
            BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_ARRAY_D \
            ) \
        (d,data,type) \
        ) \
/**/

/*

  Input is any VMD data and a VMD type for that data
  
  If the type is a tuple, checks to see if it is a more specific
  type and, if it is, returns that type,
  otherwise returns the type passed as a parameter

*/

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE(data,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE(BOOST_VMD_TYPE_TUPLE,type), \
            BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_LIST, \
            BOOST_VMD_IDENTITY(type) \
            ) \
        (data,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_D(d,data,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,BOOST_VMD_TYPE_TUPLE,type), \
            BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_CHECK_LIST_D, \
            BOOST_VMD_IDENTITY(type) \
            ) \
        (d,data,type) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_HPP */
