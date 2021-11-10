
//  Copyright 2000 John Maddock (john@johnmaddock.co.uk)
//  Copyright 2002 Aleksey Gurtovoy (agurtovoy@meta-comm.com)
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_FUNCTION_HPP_INCLUDED
#define BOOST_TT_IS_FUNCTION_HPP_INCLUDED

#include <boost/type_traits/detail/config.hpp>
#include <boost/config/workaround.hpp>

#ifdef BOOST_TT_HAS_ASCCURATE_IS_FUNCTION

#include <boost/type_traits/detail/is_function_cxx_11.hpp>

#else

#include <boost/type_traits/detail/is_function_cxx_03.hpp>

#endif

#endif // BOOST_TT_IS_FUNCTION_HPP_INCLUDED
