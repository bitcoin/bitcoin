// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DIMENSIONLESS_QUANTITY_HPP
#define BOOST_UNITS_DIMENSIONLESS_QUANTITY_HPP

///
/// \file
/// \brief Utility class to simplify construction of dimensionless quantities.
///

#include <boost/units/dimensionless_unit.hpp>
#include <boost/units/quantity.hpp>

namespace boost {

namespace units {

/// Utility class to simplify construction of dimensionless quantities.
template<class System,class Y>
struct dimensionless_quantity
{
    typedef quantity<typename dimensionless_unit<System>::type,Y>   type;
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_DIMENSIONLESS_QUANTITY_HPP
