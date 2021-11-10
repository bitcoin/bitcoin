// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_GET_SYSTEM_HPP
#define BOOST_UNITS_GET_SYSTEM_HPP

/// \file
/// \brief Get the system of a unit, absolute unit or quantity.

#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

template<class T>
struct get_system {};

/// Get the system of a unit.
template<class Dim,class System>
struct get_system< unit<Dim,System> >
{
    typedef System type;
};

/// Get the system of an absolute unit.
template<class Unit>
struct get_system< absolute<Unit> >
{
    typedef typename get_system<Unit>::type type;
};

/// Get the system of a quantity.
template<class Unit,class Y>
struct get_system< quantity<Unit,Y> >
{
    typedef typename get_system<Unit>::type     type;
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_GET_SYSTEM_HPP
