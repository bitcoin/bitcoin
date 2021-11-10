// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP

#include <boost/units/pow.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/static_constant.hpp>

#include <boost/units/systems/detail/constants.hpp>
#include <boost/units/systems/si/amount.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/electric_charge.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/mass.hpp>
#include <boost/units/systems/si/power.hpp>
#include <boost/units/systems/si/solid_angle.hpp>
#include <boost/units/systems/si/temperature.hpp>

#include <boost/units/systems/si/codata/typedefs.hpp>

/// \file
/// CODATA recommended values of fundamental physico-chemical constants
/// CODATA 2014 values as of 2016/04/26

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

// PHYSICO-CHEMICAL
/// Avogadro constant
BOOST_UNITS_PHYSICAL_CONSTANT(N_A,quantity<inverse_amount>,6.022140857e23/mole,7.4e15/mole);
/// atomic mass constant
BOOST_UNITS_PHYSICAL_CONSTANT(m_u,quantity<mass>,1.660539040e-27*kilograms,2.0e-35*kilograms);
/// Faraday constant
BOOST_UNITS_PHYSICAL_CONSTANT(F,quantity<electric_charge_over_amount>,96485.33289*coulombs/mole,5.9e-4*coulombs/mole);
/// molar gas constant
BOOST_UNITS_PHYSICAL_CONSTANT(R,quantity<energy_over_temperature_amount>,8.3144598*joules/kelvin/mole,4.8e-06*joules/kelvin/mole);
/// Boltzmann constant
BOOST_UNITS_PHYSICAL_CONSTANT(k_B,quantity<energy_over_temperature>,1.38064852e-23*joules/kelvin,7.9e-30*joules/kelvin);
/// Stefan-Boltzmann constant
BOOST_UNITS_PHYSICAL_CONSTANT(sigma_SB,quantity<power_over_area_temperature_4>,5.670367e-8*watts/square_meter/pow<4>(kelvin),1.3e-13*watts/square_meter/pow<4>(kelvin));
/// first radiation constant
BOOST_UNITS_PHYSICAL_CONSTANT(c_1,quantity<power_area>,3.741771790e-16*watt*square_meters,4.6e-24*watt*square_meters);
/// first radiation constant for spectral radiance
BOOST_UNITS_PHYSICAL_CONSTANT(c_1L,quantity<power_area_over_solid_angle>,1.191042953e-16*watt*square_meters/steradian,1.5e-24*watt*square_meters/steradian);
/// second radiation constant
BOOST_UNITS_PHYSICAL_CONSTANT(c_2,quantity<length_temperature>,1.43877736e-2*meter*kelvin,8.3e-9*meter*kelvin);
/// Wien displacement law constant : lambda_max T
BOOST_UNITS_PHYSICAL_CONSTANT(b,quantity<length_temperature>,2.8977729e-3*meter*kelvin,1.7e-9*meter*kelvin);
/// Wien displacement law constant : nu_max/T
BOOST_UNITS_PHYSICAL_CONSTANT(b_prime,quantity<frequency_over_temperature>,5.8789238e10*hertz/kelvin,3.4e4*hertz/kelvin);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP
