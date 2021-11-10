// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SI_BASE_HPP
#define BOOST_UNITS_SI_BASE_HPP

#include <string>

#include <boost/units/static_constant.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/make_system.hpp>

#include <boost/units/base_units/si/meter.hpp>
#include <boost/units/base_units/si/kilogram.hpp>
#include <boost/units/base_units/si/second.hpp>
#include <boost/units/base_units/si/ampere.hpp>
#include <boost/units/base_units/si/kelvin.hpp>
#include <boost/units/base_units/si/mole.hpp>
#include <boost/units/base_units/si/candela.hpp>
#include <boost/units/base_units/angle/radian.hpp>
#include <boost/units/base_units/angle/steradian.hpp>

namespace boost {

namespace units { 

namespace si {

/// placeholder class defining si unit system
typedef make_system<meter_base_unit, 
                    kilogram_base_unit, 
                    second_base_unit, 
                    ampere_base_unit,
                    kelvin_base_unit,
                    mole_base_unit,
                    candela_base_unit,
                    angle::radian_base_unit,
                    angle::steradian_base_unit>::type system;

/// dimensionless si unit
typedef unit<dimensionless_type,system>         dimensionless;

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_SI_BASE_HPP
