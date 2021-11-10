#ifndef  BOOST_SERIALIZATION_VOID_CAST_FWD_HPP
#define BOOST_SERIALIZATION_VOID_CAST_FWD_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// void_cast_fwd.hpp:   interface for run-time casting of void pointers.

// (C) Copyright 2005 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// gennadiy.rozental@tfn.com

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // NULL
#include <boost/serialization/force_include.hpp>

namespace boost {
namespace serialization {
namespace void_cast_detail{
class void_caster;
} // namespace void_cast_detail
template<class Derived, class Base>
BOOST_DLLEXPORT
inline const void_cast_detail::void_caster & void_cast_register(
    const Derived * dnull = NULL,
    const Base * bnull = NULL
) BOOST_USED;
} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_VOID_CAST_HPP
