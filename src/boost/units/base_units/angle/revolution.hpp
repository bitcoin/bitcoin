// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_BASE_UNITS_REVOLUTION_HPP
#define BOOST_UNITS_BASE_UNITS_REVOLUTION_HPP

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_units/angle/degree.hpp>

namespace boost {
namespace units {
namespace angle {

typedef scaled_base_unit<degree_base_unit, scale<360, static_rational<1> > >  revolution_base_unit;

}

template<>
struct base_unit_info<angle::revolution_base_unit> {
    static BOOST_CONSTEXPR const char* name()   { return("revolution"); }
    static BOOST_CONSTEXPR const char* symbol() { return("rev"); }
};

}
}

#endif // BOOST_UNITS_BASE_UNITS_REVOLUTION_HPP
