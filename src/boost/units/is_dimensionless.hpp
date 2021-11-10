// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IS_DIMENSIONLESS_HPP
#define BOOST_UNITS_IS_DIMENSIONLESS_HPP

///
/// \file
/// \brief Check if a unit or quantity is dimensionless.
///

#include <boost/mpl/bool.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

template<class T>
struct is_dimensionless :
    public mpl::false_
{ };

/// Check if a unit is dimensionless.
template<class System>
struct is_dimensionless< unit<dimensionless_type,System> > :
    public mpl::true_
{ };

/// Check if a quantity is dimensionless.
template<class Unit,class Y>
struct is_dimensionless< quantity<Unit,Y> > :
    public is_dimensionless<Unit>
{ };

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_IS_DIMENSIONLESS_HPP
