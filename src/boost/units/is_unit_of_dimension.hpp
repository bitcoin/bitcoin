// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IS_UNIT_OF_DIMENSION_HPP
#define BOOST_UNITS_IS_UNIT_OF_DIMENSION_HPP

///
/// \file
/// \brief Check that a type is a unit of the specified dimension.
///

#include <boost/mpl/bool.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

/// Check that a type is a unit of the specified dimension. 
template<class T,class Dim>
struct is_unit_of_dimension :
    public mpl::false_
{ };

template<class Dim,class System>
struct is_unit_of_dimension< unit<Dim,System>,Dim > :
    public mpl::true_
{ };

template<class Dim,class System>
struct is_unit_of_dimension< absolute<unit<Dim,System> >,Dim > :
    public mpl::true_
{ };

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_IS_UNIT_OF_DIMENSION_HPP
