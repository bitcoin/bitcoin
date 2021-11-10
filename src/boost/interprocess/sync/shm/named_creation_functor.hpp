//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SYNC_NAMED_CREATION_FUNCTOR_HPP
#define BOOST_INTERPROCESS_SYNC_NAMED_CREATION_FUNCTOR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/container/detail/placement_new.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

struct named_creation_functor_no_arg{};

template <class T, class Arg = named_creation_functor_no_arg>
class named_creation_functor
{
   typedef named_creation_functor_no_arg no_arg_t;
   public:
   named_creation_functor(create_enum_t type, Arg arg = Arg())
      :  m_creation_type(type), m_arg(arg){}

   template<class ArgType>
   void construct(void *address, typename enable_if_c<is_same<ArgType, no_arg_t>::value>::type * = 0) const
   {  ::new(address, boost_container_new_t())T; }

   template<class ArgType>
   void construct(void *address, typename enable_if_c<!is_same<ArgType, no_arg_t>::value>::type * = 0) const
   {  ::new(address, boost_container_new_t())T(m_arg); }

   bool operator()(void *address, std::size_t, bool created) const
   {
      switch(m_creation_type){
         case DoOpen:
            return true;
         break;
         case DoCreate:
         case DoOpenOrCreate:
            if(created){
               construct<Arg>(address);
            }
            return true;
         break;

         default:
            return false;
         break;
      }
   }

   static std::size_t get_min_size()
   {  return sizeof(T);  }

   private:
   create_enum_t m_creation_type;
   Arg m_arg;
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#endif   //BOOST_INTERPROCESS_SYNC_NAMED_CREATION_FUNCTOR_HPP
