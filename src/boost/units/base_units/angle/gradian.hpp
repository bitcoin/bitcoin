// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_ANGLE_GRADIAN_BASE_UNIT_HPP
#define BOOST_UNITS_ANGLE_GRADIAN_BASE_UNIT_HPP

#include <boost/units/conversion.hpp>
#include <boost/units/base_units/angle/radian.hpp>

BOOST_UNITS_DEFINE_BASE_UNIT_WITH_CONVERSIONS(angle,gradian,"gradian","grad",6.28318530718/400.,boost::units::angle::radian_base_unit,-102);

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::angle::gradian_base_unit)

#endif

#endif // BOOST_UNITS_ANGLE_GRADIAN_BASE_UNIT_HPP
