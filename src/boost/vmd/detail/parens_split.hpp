
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_PARENS_SPLIT_HPP)
#define BOOST_VMD_DETAIL_PARENS_SPLIT_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/vmd/detail/empty_result.hpp>
#include <boost/vmd/detail/parens_common.hpp>
  
#define BOOST_VMD_DETAIL_PARENS_SPLIT(vseq) \
    BOOST_PP_IIF \
      ( \
      BOOST_PP_IS_BEGIN_PARENS(vseq), \
      BOOST_VMD_DETAIL_SPLIT_PARENS, \
      BOOST_VMD_DETAIL_EMPTY_RESULT \
      ) \
    (vseq) \
/**/
#endif /* BOOST_VMD_DETAIL_PARENS_SPLIT_HPP */
