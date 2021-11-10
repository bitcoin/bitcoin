// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_ELECTRON_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_ELECTRON_CONSTANTS_HPP

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

/// electron mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_e,quantity<mass>,9.10938215e-31*kilograms,4.5e-38*kilograms);
/// electron-muon mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_mu,quantity<dimensionless>,4.83633171e-3*dimensionless(),1.2e-10*dimensionless());
/// electron-tau mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_tau,quantity<dimensionless>,2.87564e-4*dimensionless(),4.7e-8*dimensionless());
/// electron-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_p,quantity<dimensionless>,5.4461702177e-4*dimensionless(),2.4e-13*dimensionless());
/// electron-neutron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_n,quantity<dimensionless>,5.4386734459e-4*dimensionless(),3.3e-13*dimensionless());
/// electron-deuteron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_d,quantity<dimensionless>,2.7244371093e-4*dimensionless(),1.2e-13*dimensionless());
/// electron-alpha particle mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_e_over_m_alpha,quantity<dimensionless>,1.37093355570e-4*dimensionless(),5.8e-14*dimensionless());
/// electron charge to mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(e_over_m_e,quantity<electric_charge_over_mass>,1.758820150e11*coulombs/kilogram,4.4e3*coulombs/kilogram);
/// electron molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_e,quantity<mass_over_amount>,5.4857990943e-7*kilograms/mole,2.3e-16*kilograms/mole);
/// Compton wavelength
BOOST_UNITS_PHYSICAL_CONSTANT(lambda_C,quantity<length>,2.4263102175e-12*meters,3.3e-21*meters);
/// classical electron radius
BOOST_UNITS_PHYSICAL_CONSTANT(r_e,quantity<length>,2.8179402894e-15*meters,5.8e-24*meters);
/// Thompson cross section
BOOST_UNITS_PHYSICAL_CONSTANT(sigma_e,quantity<area>,0.6652458558e-28*square_meters,2.7e-37*square_meters);
/// electron magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e,quantity<energy_over_magnetic_flux_density>,-928.476377e-26*joules/tesla,2.3e-31*joules/tesla);
/// electron-Bohr magenton moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_B,quantity<dimensionless>,-1.00115965218111*dimensionless(),7.4e-13*dimensionless());
/// electron-nuclear magneton moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_N,quantity<dimensionless>,-183.28197092*dimensionless(),8.0e-7*dimensionless());
/// electron magnetic moment anomaly
BOOST_UNITS_PHYSICAL_CONSTANT(a_e,quantity<dimensionless>,1.15965218111e-3*dimensionless(),7.4e-13*dimensionless());
/// electron g-factor
BOOST_UNITS_PHYSICAL_CONSTANT(g_e,quantity<dimensionless>,-2.0023193043622*dimensionless(),1.5e-12*dimensionless());
/// electron-muon magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_mu,quantity<dimensionless>,206.7669877*dimensionless(),5.2e-6*dimensionless());
/// electron-proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_p,quantity<dimensionless>,-658.2106848*dimensionless(),5.4e-6*dimensionless());
/// electron-shielded proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_p_prime,quantity<dimensionless>,-658.2275971*dimensionless(),7.2e-6*dimensionless());
/// electron-neutron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_n,quantity<dimensionless>,960.92050*dimensionless(),2.3e-4*dimensionless());
/// electron-deuteron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_d,quantity<dimensionless>,-2143.923498*dimensionless(),1.8e-5*dimensionless());
/// electron-shielded helion magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_e_over_mu_h_prime,quantity<dimensionless>,864.058257*dimensionless(),1.0e-5*dimensionless());
/// electron gyromagnetic ratio
BOOST_UNITS_PHYSICAL_CONSTANT(gamma_e,quantity<frequency_over_magnetic_flux_density>,1.760859770e11/second/tesla,4.4e3/second/tesla);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_ELECTRON_CONSTANTS_HPP
