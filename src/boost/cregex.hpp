/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org/libs/regex for most recent version.
  *   FILE         cregex.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares POSIX API functions
  *                + boost::RegEx high level wrapper.
  */

#ifndef BOOST_RE_CREGEX_HPP
#define BOOST_RE_CREGEX_HPP

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif

#ifdef BOOST_REGEX_CXX03
#include <boost/regex/v4/cregex.hpp>
#else
#include <boost/regex/v5/cregex.hpp>
#endif

#endif /* include guard */










