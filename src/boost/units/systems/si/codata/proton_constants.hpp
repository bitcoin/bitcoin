// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_PROTON_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_PROTON_CONSTANTS_HPP

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

/// proton mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_p,quantity<mass>,1.672621637e-27*kilograms,8.3e-35*kilograms);
/// proton-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_p_over_m_e,quantity<dimensionless>,1836.15267247*dimensionless(),8.0e-7*dimensionless());
/// proton-muon mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_p_over_m_mu,quantity<dimensionless>,8.88024339*dimensionless(),2.3e-7*dimensionless());
/// proton-tau mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_p_over_m_tau,quantity<dimensionless>,0.528012*dimensionless(),8.6e-5*dimensionless());
/// proton-neutron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_p_over_m_n,quantity<dimensionless>,0.99862347824*dimensionless(),4.6e-10*dimensionless());
/// proton charge to mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(e_over_m_p,quantity<electric_charge_over_mass>,9.57883392e7*coulombs/kilogram,2.4e0*coulombs/kilogram);
/// proton molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_p,quantity<mass_over_amount>,1.00727646677e-3*kilograms/mole,1.0e-13*kilograms/mole);
/// proton Compton wavelength
BOOST_UNITS_PHYSICAL_CONSTANT(lambda_C_p,quantity<length>,1.3214098446e-15*meters,1.9e-24*meters);
/// proton rms charge radius
BOOST_UNITS_PHYSICAL_CONSTANT(R_p,quantity<length>,0.8768e-15*meters,6.9e-18*meters);
/// proton magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p,quantity<energy_over_magnetic_flux_density>,1.410606662e-26*joules/tesla,3.7e-34*joules/tesla);
/// proton-Bohr magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_over_mu_B,quantity<dimensionless>,1.521032209e-3*dimensionless(),1.2e-11*dimensionless());
/// proton-nuclear magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_over_mu_N,quantity<dimensionless>,2.792847356*dimensionless(),2.3e-8*dimensionless());
/// proton g-factor
BOOST_UNITS_PHYSICAL_CONSTANT(g_p,quantity<dimensionless>,5.585694713*dimensionless(),4.6e-8*dimensionless());
/// proton-neutron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_over_mu_n,quantity<dimensionless>,-1.45989806*dimensionless(),3.4e-7*dimensionless());
/// shielded proton magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_prime,quantity<energy_over_magnetic_flux_density>,1.410570419e-26*joules/tesla,3.8e-34*joules/tesla);
/// shielded proton-Bohr magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_prime_over_mu_B,quantity<dimensionless>,1.520993128e-3*dimensionless(),1.7e-11*dimensionless());
/// shielded proton-nuclear magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_p_prime_over_mu_N,quantity<dimensionless>,2.792775598*dimensionless(),3.0e-8*dimensionless());
/// proton magnetic shielding correction
BOOST_UNITS_PHYSICAL_CONSTANT(sigma_p_prime,quantity<dimensionless>,25.694e-6*dimensionless(),1.4e-8*dimensionless());
/// proton gyromagnetic ratio
BOOST_UNITS_PHYSICAL_CONSTANT(gamma_p,quantity<frequency_over_magnetic_flux_density>,2.675222099e8/second/tesla,7.0e0/second/tesla);
/// shielded proton gyromagnetic ratio
BOOST_UNITS_PHYSICAL_CONSTANT(gamma_p_prime,quantity<frequency_over_magnetic_flux_density>,2.675153362e8/second/tesla,7.3e0/second/tesla);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_PROTON_CONSTANTS_HPP
