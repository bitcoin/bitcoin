
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_ASSERT_HPP)
#define BOOST_VMD_DETAIL_ASSERT_HPP

#include <boost/preprocessor/debug/assert.hpp>
#include <boost/preprocessor/variadic/elem.hpp>

#if BOOST_VMD_MSVC

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/vmd/empty.hpp>

#define BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_OUTPUT(errstr) \
    BOOST_PP_ASSERT(0) \
    typedef char errstr[-1]; \
/**/

#define BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_DEFAULT(...) \
    BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_OUTPUT(BOOST_VMD_ASSERT_ERROR) \
/**/

#define BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_ERRSTR(...) \
    BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_OUTPUT(BOOST_PP_VARIADIC_ELEM(1,__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_ASSERT_TRUE(...) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL \
            ( \
            BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), \
            1 \
            ), \
        BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_DEFAULT, \
        BOOST_VMD_DETAIL_ASSERT_VC_GEN_ERROR_ERRSTR \
        ) \
    (__VA_ARGS__) \
/**/

#define BOOST_VMD_DETAIL_ASSERT(...) \
    BOOST_PP_IF \
      ( \
      BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__), \
      BOOST_VMD_EMPTY, \
      BOOST_VMD_DETAIL_ASSERT_TRUE \
      ) \
    (__VA_ARGS__) \
/**/

#else

#define BOOST_VMD_DETAIL_ASSERT_DO(cond) \
    BOOST_PP_ASSERT(cond) \
/**/

#define BOOST_VMD_DETAIL_ASSERT(...) \
    BOOST_VMD_DETAIL_ASSERT_DO(BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#endif /* BOOST_VMD_MSVC */
#endif /* BOOST_VMD_DETAIL_ASSERT_HPP */
