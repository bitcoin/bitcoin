// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_TYPEDEFS_HPP
#define BOOST_UNITS_CODATA_TYPEDEFS_HPP

#include <boost/units/operators.hpp>
#include <boost/units/systems/si/amount.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/capacitance.hpp>
#include <boost/units/systems/si/electric_charge.hpp>
#include <boost/units/systems/si/current.hpp>
#include <boost/units/systems/si/electric_potential.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/force.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/magnetic_flux_density.hpp>
#include <boost/units/systems/si/mass.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/power.hpp>
#include <boost/units/systems/si/solid_angle.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/volume.hpp>

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

typedef divide_typeof_helper<frequency,electric_potential>::type frequency_over_electric_potential;
typedef divide_typeof_helper<electric_charge,mass>::type electric_charge_over_mass;
typedef divide_typeof_helper<mass,amount>::type mass_over_amount;
typedef divide_typeof_helper<energy,magnetic_flux_density>::type energy_over_magnetic_flux_density;
typedef divide_typeof_helper<frequency,magnetic_flux_density>::type frequency_over_magnetic_flux_density;
typedef divide_typeof_helper<current,energy>::type current_over_energy;
typedef divide_typeof_helper<dimensionless,amount>::type inverse_amount;
typedef divide_typeof_helper<energy,temperature>::type energy_over_temperature;
typedef divide_typeof_helper<energy_over_temperature,amount>::type energy_over_temperature_amount;
typedef divide_typeof_helper<
            divide_typeof_helper<power,area>::type,
            power_typeof_helper<temperature,static_rational<4> >::type
        >::type power_over_area_temperature_4;
typedef multiply_typeof_helper<power,area>::type power_area;
typedef divide_typeof_helper<power_area,solid_angle>::type power_area_over_solid_angle;
typedef multiply_typeof_helper<length,temperature>::type length_temperature;
typedef divide_typeof_helper<frequency,temperature>::type frequency_over_temperature;
typedef divide_typeof_helper<divide_typeof_helper<force,current>::type,current>::type force_over_current_squared;
typedef divide_typeof_helper<capacitance,length>::type capacitance_over_length;
typedef divide_typeof_helper<
            divide_typeof_helper<divide_typeof_helper<volume,mass>::type,time>::type,
            time
        >::type volume_over_mass_time_squared;
typedef multiply_typeof_helper<energy,time>::type energy_time;
typedef divide_typeof_helper<electric_charge,amount>::type electric_charge_over_amount;

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif
