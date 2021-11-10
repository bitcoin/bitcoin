
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_SIZE_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_SIZE_HPP

#include <boost/preprocessor/array/size.hpp>
#include <boost/vmd/detail/sequence_to_array.hpp>

#define BOOST_VMD_DETAIL_SEQUENCE_SIZE(vseq) \
    BOOST_PP_ARRAY_SIZE(BOOST_VMD_DETAIL_SEQUENCE_TO_ARRAY(vseq)) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_SIZE_D(d,vseq) \
    BOOST_PP_ARRAY_SIZE(BOOST_VMD_DETAIL_SEQUENCE_TO_ARRAY_D(d,vseq)) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_SIZE_HPP */
