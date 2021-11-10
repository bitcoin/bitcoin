
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VARIADIC_MACRO_DATA_HPP)
#define BOOST_VARIADIC_MACRO_DATA_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/array.hpp>
#include <boost/vmd/assert.hpp>
#include <boost/vmd/assert_is_array.hpp>
#include <boost/vmd/assert_is_empty.hpp>
#include <boost/vmd/assert_is_identifier.hpp>
#include <boost/vmd/assert_is_list.hpp>
#include <boost/vmd/assert_is_number.hpp>
#include <boost/vmd/assert_is_seq.hpp>
#include <boost/vmd/assert_is_tuple.hpp>
#include <boost/vmd/assert_is_type.hpp>
#include <boost/vmd/elem.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/enum.hpp>
#include <boost/vmd/equal.hpp>
#include <boost/vmd/get_type.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_array.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_empty_array.hpp>
#include <boost/vmd/is_empty_list.hpp>
#include <boost/vmd/is_general_identifier.hpp>
#include <boost/vmd/is_identifier.hpp>
#include <boost/vmd/is_list.hpp>
#include <boost/vmd/is_multi.hpp>
#include <boost/vmd/is_number.hpp>
#include <boost/vmd/is_parens_empty.hpp>
#include <boost/vmd/is_seq.hpp>
#include <boost/vmd/is_tuple.hpp>
#include <boost/vmd/is_type.hpp>
#include <boost/vmd/is_unary.hpp>
#include <boost/vmd/list.hpp>
#include <boost/vmd/not_equal.hpp>
#include <boost/vmd/seq.hpp>
#include <boost/vmd/size.hpp>
#include <boost/vmd/to_array.hpp>
#include <boost/vmd/to_list.hpp>
#include <boost/vmd/to_seq.hpp>
#include <boost/vmd/to_tuple.hpp>
#include <boost/vmd/tuple.hpp>

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VARIADIC_MACRO_DATA_HPP */
