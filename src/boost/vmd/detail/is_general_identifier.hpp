
//  (C) Copyright Edward Diener 2020
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_HPP)
#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_HPP

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/identifier_concat.hpp>

#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ_RESULT(ignored) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_VMD_IDENTITY(1)(ignored) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ_ID(vseq) \
    BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ_RESULT(BOOST_VMD_DETAIL_IDENTIFIER_CONCATENATE(vseq)) \
/**/

#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ(vseq) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
          ( \
          BOOST_PP_BITOR \
            ( \
            BOOST_VMD_IS_EMPTY(vseq), \
            BOOST_PP_IS_BEGIN_PARENS(vseq) \
            ), \
          BOOST_VMD_IDENTITY(0), \
          BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ_ID \
          ) \
        (vseq) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_SINGLE(...) \
    BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_VSEQ(BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER(...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),1), \
            BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_SINGLE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (__VA_ARGS__) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER_HPP */
