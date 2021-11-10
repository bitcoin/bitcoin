
// Copyright (C) 2008-2016 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_FWD_HPP_INCLUDED
#define BOOST_UNORDERED_FWD_HPP_INCLUDED

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/predef.h>

#if defined(BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT)
// Already defined.
#elif defined(BOOST_LIBSTDCXX11)
// https://github.com/gcc-mirror/gcc/blob/gcc-4_6-branch/libstdc++-v3/include/bits/stl_pair.h#L70
#if BOOST_LIBSTDCXX_VERSION > 40600
#define BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT 1
#endif
#elif BOOST_LIB_STD_CXX
// https://github.com/llvm-mirror/libcxx/blob/release_30/include/utility#L206
#if BOOST_LIB_STD_CXX >= BOOST_VERSION_NUMBER(3, 0, 0)
#define BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT 1
#endif
#elif defined(BOOST_LIB_STD_DINKUMWARE)
// Apparently C++11 standard supported in Visual Studio 2012
// https://msdn.microsoft.com/en-us/library/hh567368.aspx#stl
// 2012 = VC+11 = BOOST_MSVC 1700 Hopefully!
// I have no idea when Dinkumware added it, probably a lot
// earlier than this check.
#if BOOST_LIB_STD_DINKUMWARE >= BOOST_VERSION_NUMBER(6, 50, 0) ||              \
  BOOST_COMP_MSVC >= BOOST_VERSION_NUMBER(17, 0, 0)
#define BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT 1
#endif
#endif

// Assume that an unknown library does not support piecewise construction.
#if !defined(BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT)
#define BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT 0
#endif

#if BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT
#include <utility>
#endif

namespace boost {
  namespace unordered {
#if BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT
    using std::piecewise_construct_t;
    using std::piecewise_construct;
#else
    struct piecewise_construct_t
    {
    };
    const piecewise_construct_t piecewise_construct = piecewise_construct_t();
#endif
  }
}

#endif
