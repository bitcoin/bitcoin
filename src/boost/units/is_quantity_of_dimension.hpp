// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IS_QUANTITY_OF_DIMENSION_HPP
#define BOOST_UNITS_IS_QUANTITY_OF_DIMENSION_HPP

///
/// \file
/// \brief Check that a type is a quantity of the specified dimension.
///

#include <boost/mpl/bool.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/is_unit_of_dimension.hpp>

namespace boost {

namespace units {

/// Check that a type is a quantity of the specified dimension.
template<class T,class Dim>
struct is_quantity_of_dimension :
    public mpl::false_
{ };

template<class Unit,class Y,class Dim>
struct is_quantity_of_dimension< quantity< Unit,Y>,Dim > :
    public is_unit_of_dimension<Unit, Dim>
{ };

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_IS_QUANTITY_OF_DIMENSION_HPP
