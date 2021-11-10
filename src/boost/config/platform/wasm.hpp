//  (C) Copyright John Maddock 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

//  WASM specific config options:

#define BOOST_PLATFORM "Wasm"

// boilerplate code:
#include <boost/config/detail/posix_features.hpp>
//
// fenv lacks the C++11 macros:
//
#define BOOST_NO_FENV_H
