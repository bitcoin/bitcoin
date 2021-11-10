//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/move/unique_ptr.hpp>

//!\file
//!This header provides utilities to define a unique_ptr that plays nicely with managed segments.

namespace boost{
namespace interprocess{

//For backwards compatibility
using ::boost::movelib::unique_ptr;

//!Returns the type of a unique pointer
//!of type T with boost::interprocess::deleter deleter
//!that can be constructed in the given managed segment type.
template<class T, class ManagedMemory>
struct managed_unique_ptr
{
   typedef boost::movelib::unique_ptr
   < T
   , typename ManagedMemory::template deleter<T>::type
   > type;
};

//!Returns an instance of a unique pointer constructed
//!with boost::interproces::deleter from a pointer
//!of type T that has been allocated in the passed managed segment
template<class T, class ManagedMemory>
inline typename managed_unique_ptr<T, ManagedMemory>::type
   make_managed_unique_ptr(T *constructed_object, ManagedMemory &managed_memory)
{
   return typename managed_unique_ptr<T, ManagedMemory>::type
      (constructed_object, managed_memory.template get_deleter<T>());
}

}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED
