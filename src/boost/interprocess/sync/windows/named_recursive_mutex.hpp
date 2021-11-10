 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTERPROCESS_WINDOWS_RECURSIVE_NAMED_MUTEX_HPP
#define BOOST_INTERPROCESS_WINDOWS_RECURSIVE_NAMED_MUTEX_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/windows/named_mutex.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {


class winapi_named_recursive_mutex
   //Windows mutexes based on CreateMutex are already recursive...
   : public winapi_named_mutex
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Non-copyable
   winapi_named_recursive_mutex();
   winapi_named_recursive_mutex(const winapi_named_mutex &);
   winapi_named_recursive_mutex &operator=(const winapi_named_mutex &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   winapi_named_recursive_mutex(create_only_t, const char *name, const permissions &perm = permissions())
      : winapi_named_mutex(create_only_t(), name, perm)
   {}

   winapi_named_recursive_mutex(open_or_create_t, const char *name, const permissions &perm = permissions())
      : winapi_named_mutex(open_or_create_t(), name, perm)
   {}

   winapi_named_recursive_mutex(open_only_t, const char *name)
      : winapi_named_mutex(open_only_t(), name)
   {}

   winapi_named_recursive_mutex(create_only_t, const wchar_t *name, const permissions &perm = permissions())
      : winapi_named_mutex(create_only_t(), name, perm)
   {}

   winapi_named_recursive_mutex(open_or_create_t, const wchar_t *name, const permissions &perm = permissions())
      : winapi_named_mutex(open_or_create_t(), name, perm)
   {}

   winapi_named_recursive_mutex(open_only_t, const wchar_t *name)
      : winapi_named_mutex(open_only_t(), name)
   {}
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_RECURSIVE_NAMED_MUTEX_HPP
