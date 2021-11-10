// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CGS_DYNAMIC_VISCOSITY_HPP
#define BOOST_UNITS_CGS_DYNAMIC_VISCOSITY_HPP

#include <boost/units/systems/cgs/base.hpp>
#include <boost/units/physical_dimensions/dynamic_viscosity.hpp>

namespace boost {

namespace units { 

namespace cgs {

typedef unit<dynamic_viscosity_dimension,cgs::system>     dynamic_viscosity;
    
BOOST_UNITS_STATIC_CONSTANT(poise,dynamic_viscosity);    

} // namespace cgs

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CGS_DYNAMIC_VISCOSITY_HPP
