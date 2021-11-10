// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_ATOMIC_AND_NUCLEAR_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_ATOMIC_AND_NUCLEAR_CONSTANTS_HPP

#include <boost/units/systems/si/codata/alpha_constants.hpp>
#include <boost/units/systems/si/codata/deuteron_constants.hpp>
#include <boost/units/systems/si/codata/electron_constants.hpp>
#include <boost/units/systems/si/codata/helion_constants.hpp>
#include <boost/units/systems/si/codata/muon_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/proton_constants.hpp>
#include <boost/units/systems/si/codata/tau_constants.hpp>
#include <boost/units/systems/si/codata/triton_constants.hpp>

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

/// CODATA recommended values of the fundamental physical constants: NIST SP 961

// ATOMIC AND NUCLEAR
/// fine structure constant
BOOST_UNITS_PHYSICAL_CONSTANT(alpha,quantity<dimensionless>,7.2973525376e-3*dimensionless(),5.0e-12*dimensionless());
/// Rydberg constant
BOOST_UNITS_PHYSICAL_CONSTANT(R_infinity,quantity<wavenumber>,10973731.568527/meter,7.3e-5/meter);
/// Bohr radius
BOOST_UNITS_PHYSICAL_CONSTANT(a_0,quantity<length>,0.52917720859e-10*meters,3.6e-20*meters);
/// Hartree energy
BOOST_UNITS_PHYSICAL_CONSTANT(E_h,quantity<energy>,4.35974394e-18*joules,2.2e-25*joules);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_ATOMIC_AND_NUCLEAR_CONSTANTS_HPP
