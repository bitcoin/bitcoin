
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_ENUM_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_ENUM_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/sequence_to_tuple.hpp>

#define BOOST_VMD_DETAIL_SEQUENCE_ENUM_PROCESS(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(tuple), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_ENUM \
        ) \
    (tuple) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ENUM(...) \
    BOOST_VMD_DETAIL_SEQUENCE_ENUM_PROCESS \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_TO_TUPLE(__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ENUM_D(d,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ENUM_PROCESS \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_TO_TUPLE_D(d,__VA_ARGS__) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_ENUM_HPP */
