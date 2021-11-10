// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CGS_IO_HPP
#define BOOST_UNITS_CGS_IO_HPP

#include <boost/units/io.hpp>
#include <boost/units/reduce_unit.hpp>
#include <boost/units/systems/cgs.hpp>

namespace boost {

namespace units { 

inline std::string name_string(const reduce_unit<cgs::acceleration>::type&) { return "galileo"; }
inline std::string symbol_string(const reduce_unit<cgs::acceleration>::type&) { return "Gal"; }

inline std::string name_string(const reduce_unit<cgs::current>::type&)   { return "biot"; }
inline std::string symbol_string(const reduce_unit<cgs::current>::type&) { return "Bi"; }

inline std::string name_string(const reduce_unit<cgs::dynamic_viscosity>::type&) { return "poise"; }
inline std::string symbol_string(const reduce_unit<cgs::dynamic_viscosity>::type&) { return "P"; }

inline std::string name_string(const reduce_unit<cgs::energy>::type&) { return "erg"; }
inline std::string symbol_string(const reduce_unit<cgs::energy>::type&) { return "erg"; }

inline std::string name_string(const reduce_unit<cgs::force>::type&) { return "dyne"; }
inline std::string symbol_string(const reduce_unit<cgs::force>::type&) { return "dyn"; }

inline std::string name_string(const reduce_unit<cgs::kinematic_viscosity>::type&) { return "stoke"; }
inline std::string symbol_string(const reduce_unit<cgs::kinematic_viscosity>::type&) { return "St"; }

inline std::string name_string(const reduce_unit<cgs::pressure>::type&) { return "barye"; }
inline std::string symbol_string(const reduce_unit<cgs::pressure>::type&) { return "Ba"; }

inline std::string name_string(const reduce_unit<cgs::wavenumber>::type&) { return "kayser"; }
inline std::string symbol_string(const reduce_unit<cgs::wavenumber>::type&) { return "K"; }

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CGS_IO_HPP
