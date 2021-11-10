// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SI_VOLUME_HPP
#define BOOST_UNITS_SI_VOLUME_HPP

#include <boost/units/systems/si/base.hpp>
#include <boost/units/physical_dimensions/volume.hpp>

namespace boost {

namespace units { 

namespace si {

typedef unit<volume_dimension,si::system>    volume;
    
BOOST_UNITS_STATIC_CONSTANT(cubic_meter,volume);    
BOOST_UNITS_STATIC_CONSTANT(cubic_meters,volume);   
BOOST_UNITS_STATIC_CONSTANT(cubic_metre,volume);    
BOOST_UNITS_STATIC_CONSTANT(cubic_metres,volume);   

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_SI_VOLUME_HPP
