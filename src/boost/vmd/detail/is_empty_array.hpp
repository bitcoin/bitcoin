
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_HPP)
#define BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_HPP

#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/logical/not.hpp>

#define BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_SIZE(array) \
    BOOST_PP_NOT \
        ( \
        BOOST_PP_ARRAY_SIZE(array) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_HPP */
