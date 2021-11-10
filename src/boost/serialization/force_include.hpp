#ifndef BOOST_SERIALIZATION_FORCE_INCLUDE_HPP
#define BOOST_SERIALIZATION_FORCE_INCLUDE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// force_include.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>

// the following help macro is to guarentee that certain coded
// is not removed by over-eager linker optimiser.  In certain cases
// we create static objects must be created but are actually never
// referenced - creation has a side-effect such as global registration
// which is important to us. We make an effort to refer these objects
// so that a smart linker won't remove them as being unreferenced.
// In microsoft compilers, inlining the code that does the referring
// means the code gets lost and the static object is not included
// in the library and hence never registered.  This manifests itself
// in an ungraceful crash at runtime when (and only when) built in
// release mode.

#if defined(BOOST_HAS_DECLSPEC) && !defined(__COMO__)
#   define BOOST_DLLEXPORT __declspec(dllexport)
#elif ! defined(_WIN32) && ! defined(_WIN64)
#   if defined(__MWERKS__)
#       define BOOST_DLLEXPORT __declspec(dllexport)
#   elif defined(__GNUC__) && (__GNUC__ >= 3)
#       define BOOST_USED __attribute__ ((__used__))
#   elif defined(__IBMCPP__) && (__IBMCPP__ >= 1110)
#       define BOOST_USED __attribute__ ((__used__))
#   elif defined(__INTEL_COMPILER) && (BOOST_INTEL_CXX_VERSION >= 800)
#       define BOOST_USED __attribute__ ((__used__))
#   endif
#endif

#ifndef BOOST_USED
#    define BOOST_USED
#endif

#ifndef BOOST_DLLEXPORT
#    define BOOST_DLLEXPORT
#endif

#endif // BOOST_SERIALIZATION_FORCE_INCLUDE_HPP
