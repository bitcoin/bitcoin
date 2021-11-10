// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_GET_DIMENSION_HPP
#define BOOST_UNITS_GET_DIMENSION_HPP

///
/// \file
/// \brief Get the dimension of a unit, absolute unit and quantity.
/// \details
///

#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

template<class T>
struct get_dimension {};

/// Get the dimension of a unit.
template<class Dim,class System>
struct get_dimension< unit<Dim,System> >
{
    typedef Dim type;
};

/// Get the dimension of an absolute unit.
template<class Unit>
struct get_dimension< absolute<Unit> >
{
    typedef typename get_dimension<Unit>::type  type;
};

/// Get the dimension of a quantity.
template<class Unit,class Y>
struct get_dimension< quantity<Unit,Y> >
{
    typedef typename get_dimension<Unit>::type  type;
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_GET_DIMENSION_HPP
