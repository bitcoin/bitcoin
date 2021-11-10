// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SI_LENGTH_HPP
#define BOOST_UNITS_SI_LENGTH_HPP

#include <boost/units/systems/si/base.hpp>

namespace boost {

namespace units { 

namespace si {

typedef unit<length_dimension,si::system>    length;
    
BOOST_UNITS_STATIC_CONSTANT(meter,length);  
BOOST_UNITS_STATIC_CONSTANT(meters,length); 
BOOST_UNITS_STATIC_CONSTANT(metre,length);  
BOOST_UNITS_STATIC_CONSTANT(metres,length); 

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_SI_LENGTH_HPP
