// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_HELION_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_HELION_CONSTANTS_HPP

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

/// helion mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_h,quantity<mass>,5.00641192e-27*kilograms,2.5e-34*kilograms);
/// helion-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_h_over_m_e,quantity<dimensionless>,5495.8852765*dimensionless(),5.2e-6*dimensionless());
/// helion-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_h_over_m_p,quantity<dimensionless>,2.9931526713*dimensionless(),2.6e-9*dimensionless());
/// helion molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_h,quantity<mass_over_amount>,3.0149322473e-3*kilograms/mole,2.6e-12*kilograms/mole);
/// helion shielded magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_h_prime,quantity<energy_over_magnetic_flux_density>,-1.074552982e-26*joules/tesla,3.0e-34*joules/tesla);
/// shielded helion-Bohr magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_h_prime_over_mu_B,quantity<dimensionless>,-1.158671471e-3*dimensionless(),1.4e-11*dimensionless());
/// shielded helion-nuclear magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_h_prime_over_mu_N,quantity<dimensionless>,-2.127497718*dimensionless(),2.5e-8*dimensionless());
/// shielded helion-proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_h_prime_over_mu_p,quantity<dimensionless>,-0.761766558*dimensionless(),1.1e-8*dimensionless());
/// shielded helion-shielded proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_h_prime_over_mu_p_prime,quantity<dimensionless>,-0.7617861313*dimensionless(),3.3e-8*dimensionless());
/// shielded helion gyromagnetic ratio
BOOST_UNITS_PHYSICAL_CONSTANT(gamma_h_prime,quantity<frequency_over_magnetic_flux_density>,2.037894730e8/second/tesla,5.6e-0/second/tesla);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_HELION_CONSTANTS_HPP
