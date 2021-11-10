#ifndef BOOST_SMART_PTR_MAKE_SHARED_HPP_INCLUDED
#define BOOST_SMART_PTR_MAKE_SHARED_HPP_INCLUDED

//  make_shared.hpp
//
//  Copyright (c) 2007, 2008, 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/smart_ptr/make_shared_object.hpp>

#if !defined( BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION ) && !defined( BOOST_NO_SFINAE )
# include <boost/smart_ptr/make_shared_array.hpp>
# include <boost/smart_ptr/allocate_shared_array.hpp>
#endif

#endif // #ifndef BOOST_SMART_PTR_MAKE_SHARED_HPP_INCLUDED
