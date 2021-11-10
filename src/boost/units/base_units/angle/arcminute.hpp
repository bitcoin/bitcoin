// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNIT_BASE_UNITS_ANGLE_ARCMINUTE_HPP_INCLUDED
#define BOOST_UNIT_BASE_UNITS_ANGLE_ARCMINUTE_HPP_INCLUDED

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_units/angle/degree.hpp>

namespace boost {
namespace units {
namespace angle {

typedef scaled_base_unit<degree_base_unit, scale<60, static_rational<-1> > >  arcminute_base_unit;

}

template<>
struct base_unit_info<angle::arcminute_base_unit> {
    static BOOST_CONSTEXPR const char* name()   { return("arcminute"); }
    static BOOST_CONSTEXPR const char* symbol() { return("'"); }
};

}
}

#endif // BOOST_UNIT_BASE_UNITS_ANGLE_ARCMINUTE_HPP_INCLUDED
