#ifndef BOOST_SMART_PTR_DETAIL_SP_TYPEINFO_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_TYPEINFO_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  smart_ptr/detail/sp_typeinfo_.hpp
//
//  Copyright 2007, 2019 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined( BOOST_NO_TYPEID ) || defined( BOOST_NO_STD_TYPEINFO )

#include <boost/core/typeinfo.hpp>

namespace boost
{

namespace detail
{

typedef boost::core::typeinfo sp_typeinfo_;

} // namespace detail

} // namespace boost

#define BOOST_SP_TYPEID_(T) BOOST_CORE_TYPEID(T)

#else // defined( BOOST_NO_TYPEID ) || defined( BOOST_NO_STD_TYPEINFO )

#include <typeinfo>

namespace boost
{

namespace detail
{

typedef std::type_info sp_typeinfo_;

} // namespace detail

} // namespace boost

#define BOOST_SP_TYPEID_(T) typeid(T)

#endif // defined( BOOST_NO_TYPEID ) || defined( BOOST_NO_STD_TYPEINFO )

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_TYPEINFO_HPP_INCLUDED
