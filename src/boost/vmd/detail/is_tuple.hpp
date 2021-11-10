
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_TUPLE_HPP)
#define BOOST_VMD_DETAIL_IS_TUPLE_HPP

#include <boost/vmd/detail/is_entire.hpp>
#include <boost/vmd/detail/parens_split.hpp>
  
#define BOOST_VMD_DETAIL_IS_TUPLE(vseq) \
    BOOST_VMD_DETAIL_IS_ENTIRE \
        ( \
        BOOST_VMD_DETAIL_PARENS_SPLIT(vseq) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IS_TUPLE_HPP */
