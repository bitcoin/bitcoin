#ifndef GREGORIAN_HPP__
#define GREGORIAN_HPP__

/* Copyright (c) 2002-2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 * $Date$
 */

/*! @file gregorian.hpp
  Single file header that provides overall include for all elements of 
  the gregorian date-time system.  This includes the various types 
  defined, but also other functions for formatting and parsing.
*/


#include "boost/date_time/compiler_config.hpp"
#include "boost/date_time/gregorian/gregorian_types.hpp"
#include "boost/date_time/gregorian/conversion.hpp"
#if defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
#include "boost/date_time/gregorian/formatters_limited.hpp"
#else
#include "boost/date_time/gregorian/formatters.hpp"
#endif

#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO)
#include "boost/date_time/gregorian/greg_facet.hpp"
#else
#include "boost/date_time/gregorian/gregorian_io.hpp"
#endif // USE_DATE_TIME_PRE_1_33_FACET_IO

#include "boost/date_time/gregorian/parsers.hpp"



#endif
