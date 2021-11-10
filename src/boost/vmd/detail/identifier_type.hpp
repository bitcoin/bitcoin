
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IDENTIFIER_TYPE_HPP)
#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_tuple.hpp>
#include <boost/vmd/detail/idprefix.hpp>
#include <boost/vmd/detail/number_registration.hpp>
#include <boost/vmd/detail/type_registration.hpp>

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCATENATE(id) \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_SUBTYPE_REGISTRATION_PREFIX, \
        id \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_SIZE(cres) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL \
                ( \
                BOOST_PP_TUPLE_SIZE(cres), \
                2 \
                ), \
            BOOST_PP_TUPLE_ELEM, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_IDENTIFIER) \
            ) \
        (0,cres) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_SIZE_D(d,cres) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D \
                ( \
                d, \
                BOOST_PP_TUPLE_SIZE(cres), \
                2 \
                ), \
            BOOST_PP_TUPLE_ELEM, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_IDENTIFIER) \
            ) \
        (0,cres) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCAT(cres) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_TUPLE(cres), \
            BOOST_VMD_DETAIL_IDENTIFIER_TYPE_SIZE, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_IDENTIFIER) \
            ) \
        (cres) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCAT_D(d,cres) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_TUPLE(cres), \
            BOOST_VMD_DETAIL_IDENTIFIER_TYPE_SIZE_D, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_IDENTIFIER) \
            ) \
        (d,cres) \
        ) \
/**/

/*

  Determines the type of an identifier.
  
  The type may be that of an identifier or else
  it may be a subtype.
  
  Assumes the 'id' is a valid identifier id
  
  Expands to the appropriate type

*/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE(id) \
    BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCAT \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCATENATE(id) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_TYPE_D(d,id) \
    BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCAT_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_IDENTIFIER_TYPE_CONCATENATE(id) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IDENTIFIER_TYPE_HPP */
