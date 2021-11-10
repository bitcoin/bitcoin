// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_DEUTERON_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_DEUTERON_CONSTANTS_HPP

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

/// deuteron mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_d,quantity<mass>,3.34358320e-27*kilograms,1.7e-34*kilograms);
/// deuteron-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_d_over_m_e,quantity<dimensionless>,3670.4829654*dimensionless(),1.6e-6*dimensionless());
/// deuteron-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_d_over_m_p,quantity<dimensionless>,1.99900750108*dimensionless(),2.2e-10*dimensionless());
/// deuteron molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_d,quantity<mass_over_amount>,2.013553212724e-3*kilograms/mole,7.8e-14*kilograms/mole);
/// deuteron rms charge radius
BOOST_UNITS_PHYSICAL_CONSTANT(R_d,quantity<length>,2.1402e-15*meters,2.8e-18*meters);
/// deuteron magnetic moment
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d,quantity<energy_over_magnetic_flux_density>,0.433073465e-26*joules/tesla,1.1e-34*joules/tesla);
/// deuteron-Bohr magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d_over_mu_B,quantity<dimensionless>,0.4669754556e-3*dimensionless(),3.9e-12*dimensionless());
/// deuteron-nuclear magneton ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d_over_mu_N,quantity<dimensionless>,0.8574382308*dimensionless(),7.2e-9*dimensionless());
/// deuteron g-factor
BOOST_UNITS_PHYSICAL_CONSTANT(g_d,quantity<dimensionless>,0.8574382308*dimensionless(),7.2e-9*dimensionless());
/// deuteron-electron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d_over_mu_e,quantity<dimensionless>,-4.664345537e-4*dimensionless(),3.9e-12*dimensionless());
/// deuteron-proton magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d_over_mu_p,quantity<dimensionless>,0.3070122070*dimensionless(),2.4e-9*dimensionless());
/// deuteron-neutron magnetic moment ratio
BOOST_UNITS_PHYSICAL_CONSTANT(mu_d_over_mu_n,quantity<dimensionless>,-0.44820652*dimensionless(),1.1e-7*dimensionless());

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_DEUTERON_CONSTANTS_HPP
