// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_NEUTRON_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_NEUTRON_CONSTANTS_HPP

#include <boost/units/quantity.hpp>
#include <boost/units/static_constant.hpp>

#include <boost/units/systems/detail/constants.hpp>
#include <boost/units/systems/si/amount.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/electric_charge.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/mass.hpp>
#include <boost/units/systems/si/magnetic_flux_density.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/wavenumber.hpp>

#include <boost/units/systems/si/codata/typedefs.hpp>

/// \file
/// CODATA recommended values of fundamental atomic and nuclear constants
/// CODATA 2006 values as of 2007/03/30

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

/// CODATA recommended values of the fundamental physical constants: NIST SP 961

/// neutron mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_n,quantity<mass>,1.674927211e-27*kilograms,8.4e-35*kilograms);
/// neutron-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_n_over_m_e,quantity<dimensionless>,1838.6836605*dimensionless(),1.1e-6*dimensionless());
/// neutron-muon mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_n_over_m_mu,quantity<dimensionless>,8.89248409*dimensionless(),2.3e-7*dimensionless());
/// neutron-tau mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_n_over_m_tau,quantity<dimensionless>,0.528740*dimensionless(),8.6e-5*dimensionless());
/// neutron-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_n_over_m_p,quantity<dimensionless>,1.00137841918*dimensionless(),4.6e-10*dimensionless());
/// neutron molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_n,quantity<mass_over_amount>,1.00866491597e-3*kilograms/mole,4.3e-13*kilograms/mole);
/// neutron Compton wavelength
BOOST_UNITS_PHYSICAL_CONSTANT(lambda_C_n,quantity<length>,1.3195908951e-15*meters,2.0e-24*meters);
/// neutron magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_n,quantity<energy_over_magnetic_flux_density>,-0.96623641e-26*joules/tesla,2.3e-33*joules/tesla);
/// neutron g-factor
BOOST_UNITS_PHYSICAL_CONSTANT(g_n,quantity<dimensionless>,-3.82608545*dimensionless(),9.0e-7*dimensionless());
/// neutron-electron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_n_over_mu_e,quantity<dimensionless>,1.04066882e-3*dimensionless(),2.5e-10*dimensionless());
/// neutron-proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_n_over_mu_p,quantity<dimensionless>,-0.68497934*dimensionless(),1.6e-7*dimensionless());
/// neutron-shielded proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_n_over_mu_p_prime,quantity<dimensionless>,-0.68499694*dimensionless(),1.6e-7*dimensionless());
/// neutron gyromagnetic ratio
BOOST_UNITS_PHYSICAL_CONSTANT(gamma_n,quantity<frequency_over_magnetic_flux_density>,1.83247185e8/second/tesla,4.3e1/second/tesla);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_NEUTRON_CONSTANTS_HPP
