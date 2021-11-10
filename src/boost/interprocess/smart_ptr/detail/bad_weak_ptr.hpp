//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/detail/bad_weak_ptr.hpp
//
// (C) Copyright Peter Dimov and Multi Media Ltd. 2001, 2002, 2003
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTERPROCESS_BAD_WEAK_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_BAD_WEAK_PTR_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

namespace boost{
namespace interprocess{

class bad_weak_ptr
   :  public std::exception
{
   public:

   virtual char const * what() const BOOST_NOEXCEPT_OR_NOTHROW
   {  return "boost::interprocess::bad_weak_ptr"; }
};

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_BAD_WEAK_PTR_HPP_INCLUDED
