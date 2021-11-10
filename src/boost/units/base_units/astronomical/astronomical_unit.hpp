// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNIT_SYSTEMS_ASTRONOMICAL_ASTRONOMICAL_UNIT_HPP_INCLUDED
#define BOOST_UNIT_SYSTEMS_ASTRONOMICAL_ASTRONOMICAL_UNIT_HPP_INCLUDED

#include <boost/units/conversion.hpp>
#include <boost/units/base_units/si/meter.hpp>

BOOST_UNITS_DEFINE_BASE_UNIT_WITH_CONVERSIONS(astronomical, astronomical_unit, "astronomical unit", "a.u.", 149597870691.0, boost::units::si::meter_base_unit, -207);

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::astronomical::astronomical_unit_base_unit)

#endif

#endif // BOOST_UNIT_SYSTEMS_ASTRONOMICAL_ASTRONOMICAL_UNIT_HPP_INCLUDED
