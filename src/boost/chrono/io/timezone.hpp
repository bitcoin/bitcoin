//  (C) Copyright Howard Hinnant
//  (C) Copyright 2010-2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
// This code was adapted by Vicente from Howard Hinnant's experimental work
// on chrono i/o to Boost

#ifndef BOOST_CHRONO_IO_TIMEZONE_HPP
#define BOOST_CHRONO_IO_TIMEZONE_HPP
#include <boost/detail/scoped_enum_emulation.hpp>

namespace boost
{
  namespace chrono
  {
    /**
     * Scoped enumeration emulation stating whether the time_point for system_clock I/O is UTC or local.
     */
    BOOST_SCOPED_ENUM_DECLARE_BEGIN(timezone)
          {
            utc, local
          }
    BOOST_SCOPED_ENUM_DECLARE_END(timezone)

  } // chrono
} // boost

#endif  // header
