
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_PARENS_HPP)
#define BOOST_VMD_DETAIL_PARENS_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/parens_split.hpp>
#include <boost/vmd/detail/parens_common.hpp>
  
#define BOOST_VMD_DETAIL_PARENS_PROCESS(vseq) \
    BOOST_PP_IIF \
      ( \
      BOOST_PP_IS_BEGIN_PARENS(vseq), \
      BOOST_VMD_DETAIL_BEGIN_PARENS, \
      BOOST_VMD_EMPTY \
      ) \
    (vseq) \
/**/

#define BOOST_VMD_DETAIL_PARENS(...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_PARENS_SPLIT, \
        BOOST_VMD_DETAIL_PARENS_PROCESS \
        ) \
    (BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_PARENS_D(d,...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS_D(d,BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_PARENS_SPLIT, \
        BOOST_VMD_DETAIL_PARENS_PROCESS \
        ) \
    (BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#endif /* BOOST_VMD_DETAIL_PARENS_HPP */
