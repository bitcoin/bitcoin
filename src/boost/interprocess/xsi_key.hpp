//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_XSI_KEY_HPP
#define BOOST_INTERPROCESS_XSI_KEY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/detail/workaround.hpp>

#if !defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)
#error "This header can't be used in operating systems without XSI (System V) shared memory support"
#endif

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <sys/types.h>
#include <sys/ipc.h>
#include <cstddef>
#include <boost/cstdint.hpp>

//!\file
//!Describes a class representing a xsi key type.

namespace boost {
namespace interprocess {

//!A class that wraps XSI (System V) key_t type.
//!This type calculates key_t from path and id using ftok,
//!sets key to a specified value,
//!or sets key to IPC_PRIVATE using the default constructor.
class xsi_key
{
   public:

   //!Default constructor.
   //!Represents a private xsi_key.
   xsi_key()
      : m_key(IPC_PRIVATE)
   {}

   //!Creates a new XSI key using a specified value. Constructor is explicit to avoid ambiguity with shmid.
   explicit xsi_key(key_t key)
      : m_key(key)
   {}

   //!Creates a new XSI  shared memory with a key obtained from a call to ftok (with path
   //!"path" and id "id"), of size "size" and permissions "perm".
   //!If the shared memory previously exists, throws an error.
   xsi_key(const char *path, boost::uint8_t id)
   {
      key_t key;
      if(path){
         key  = ::ftok(path, id);
         if(((key_t)-1) == key){
            error_info err = system_error_code();
            throw interprocess_exception(err);
         }
      }
      else{
         key = IPC_PRIVATE;
      }
      m_key = key;
   }

   //!Returns the internal key_t value
   key_t get_key() const
   {  return m_key;  }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   key_t m_key;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_XSI_KEY_HPP
