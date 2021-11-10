// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_US_INCH_BASE_UNIT_HPP
#define BOOST_UNITS_US_INCH_BASE_UNIT_HPP

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_units/us/yard.hpp>

namespace boost {

namespace units {

namespace us {

typedef scaled_base_unit<yard_base_unit, scale<36, static_rational<-1> > > inch_base_unit;

} // namespace us

template<>
struct base_unit_info<us::inch_base_unit> {
    static BOOST_CONSTEXPR const char* name()   { return("inch"); }
    static BOOST_CONSTEXPR const char* symbol() { return("in"); }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_US_INCH_BASE_UNIT_HPP
