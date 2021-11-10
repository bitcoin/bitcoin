//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP
#define BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#ifdef BOOST_INTERPROCESS_WINDOWS
   #include <boost/interprocess/detail/windows_intermodule_singleton.hpp>
#else
   #include <boost/interprocess/detail/portable_intermodule_singleton.hpp>
#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

//Now this class is a singleton, initializing the singleton in
//the first get() function call if LazyInit is true. If false
//then the singleton will be initialized when loading the module.
template<typename C, bool LazyInit = true, bool Phoenix = false>
class intermodule_singleton
   #ifdef BOOST_INTERPROCESS_WINDOWS
   : public windows_intermodule_singleton<C, LazyInit, Phoenix>
   #else
   : public portable_intermodule_singleton<C, LazyInit, Phoenix>
   #endif
{};

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
