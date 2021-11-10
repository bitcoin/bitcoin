// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_ELECTROMAGNETIC_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_ELECTROMAGNETIC_CONSTANTS_HPP

///
/// \file
/// \brief CODATA recommended values of fundamental electromagnetic constants.
/// \details CODATA recommended values of the fundamental physical constants: NIST SP 961
///   CODATA 2006 values as of 2007/03/30
///

#include <boost/units/quantity.hpp>
#include <boost/units/static_constant.hpp>

#include <boost/units/systems/detail/constants.hpp>
#include <boost/units/systems/si/conductance.hpp>
#include <boost/units/systems/si/current.hpp>
#include <boost/units/systems/si/electric_charge.hpp>
#include <boost/units/systems/si/electric_potential.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/magnetic_flux.hpp>
#include <boost/units/systems/si/magnetic_flux_density.hpp>
#include <boost/units/systems/si/resistance.hpp>

#include <boost/units/systems/si/codata/typedefs.hpp>

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

// ELECTROMAGNETIC
/// elementary charge
BOOST_UNITS_PHYSICAL_CONSTANT(e,quantity<electric_charge>,1.602176487e-19*coulombs,4.0e-27*coulombs);
/// elementary charge to Planck constant ratio
BOOST_UNITS_PHYSICAL_CONSTANT(e_over_h,quantity<current_over_energy>,2.417989454e14*amperes/joule,6.0e6*amperes/joule);
/// magnetic flux quantum
BOOST_UNITS_PHYSICAL_CONSTANT(Phi_0,quantity<magnetic_flux>,2.067833667e-15*webers,5.2e-23*webers);
/// conductance quantum
BOOST_UNITS_PHYSICAL_CONSTANT(G_0,quantity<conductance>,7.7480917004e-5*siemens,5.3e-14*siemens);
/// Josephson constant
BOOST_UNITS_PHYSICAL_CONSTANT(K_J,quantity<frequency_over_electric_potential>,483597.891e9*hertz/volt,1.2e7*hertz/volt);
/// von Klitzing constant
BOOST_UNITS_PHYSICAL_CONSTANT(R_K,quantity<resistance>,25812.807557*ohms,1.77e-5*ohms);
/// Bohr magneton
BOOST_UNITS_PHYSICAL_CONSTANT(mu_B,quantity<energy_over_magnetic_flux_density>,927.400915e-26*joules/tesla,2.3e-31*joules/tesla);
/// nuclear magneton
BOOST_UNITS_PHYSICAL_CONSTANT(mu_N,quantity<energy_over_magnetic_flux_density>,5.05078324e-27*joules/tesla,1.3e-34*joules/tesla);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_ELECTROMAGNETIC_CONSTANTS_HPP
