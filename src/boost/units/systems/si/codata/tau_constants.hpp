// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_TAU_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_TAU_CONSTANTS_HPP

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

/// tau mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_tau,quantity<mass>,3.16777e-27*kilograms,5.2e-31*kilograms);
/// tau-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_tau_over_m_e,quantity<dimensionless>,3477.48*dimensionless(),5.7e-1*dimensionless());
/// tau-muon mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_tau_over_m_mu,quantity<dimensionless>,16.8183*dimensionless(),2.7e-3*dimensionless());
/// tau-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_tau_over_m_p,quantity<dimensionless>,1.89390*dimensionless(),3.1e-4*dimensionless());
/// tau-neutron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_tau_over_m_n,quantity<dimensionless>,1.89129*dimensionless(),3.1e-4*dimensionless());
/// tau molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_tau,quantity<mass_over_amount>,1.90768e-3*kilograms/mole,3.1e-7*kilograms/mole);
/// tau Compton wavelength
BOOST_UNITS_PHYSICAL_CONSTANT(lambda_C_tau,quantity<length>,0.69772e-15*meters,1.1e-19*meters);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_TAU_CONSTANTS_HPP
