//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTERPROCESS_EXTERNAL_CONFIG_INCLUDED
#define BOOST_INTERPROCESS_EXTERNAL_CONFIG_INCLUDED
#include <boost/config.hpp>
#endif

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 406)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif
