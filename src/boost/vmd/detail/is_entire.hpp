
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_ENTIRE_HPP)
#define BOOST_VMD_DETAIL_IS_ENTIRE_HPP

#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/not_empty.hpp>

#define BOOST_VMD_DETAIL_IS_ENTIRE(tuple) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_NOT_EMPTY(BOOST_PP_TUPLE_ELEM(0,tuple)), \
        BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(1,tuple)) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IS_ENTIRE_HPP */
