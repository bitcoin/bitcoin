// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_ALPHA_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_ALPHA_CONSTANTS_HPP

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

/// alpha particle mass
BOOST_UNITS_PHYSICAL_CONSTANT(m_alpha,quantity<mass>,6.64465620e-27*kilograms,3.3e-34*kilograms);
/// alpha-electron mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_alpha_over_m_e,quantity<dimensionless>,7294.2995365*dimensionless(),3.1e-6*dimensionless());
/// alpha-proton mass ratio
BOOST_UNITS_PHYSICAL_CONSTANT(m_alpha_over_m_p,quantity<dimensionless>,3.97259968951*dimensionless(),4.1e-10*dimensionless());
/// alpha molar mass
BOOST_UNITS_PHYSICAL_CONSTANT(M_alpha,quantity<mass_over_amount>,4.001506179127e-3*kilograms/mole,6.2e-14*kilograms/mole);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_ALPHA_CONSTANTS_HPP
