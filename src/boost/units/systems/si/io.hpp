// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SI_IO_HPP
#define BOOST_UNITS_SI_IO_HPP

#include <boost/units/io.hpp>
#include <boost/units/reduce_unit.hpp>

#include <boost/units/systems/si.hpp>

namespace boost {

namespace units { 

// gray and sievert are indistinguishable
inline std::string name_string(const reduce_unit<si::absorbed_dose>::type&) { return "gray"; }
inline std::string symbol_string(const reduce_unit<si::absorbed_dose>::type&) { return "Gy"; }

// activity and frequency are indistinguishable - would need a "decays" base unit
//inline std::string name_string(const si::activity&) { return "becquerel"; }
//inline std::string symbol_string(const si::activity&) { return "Bq"; }

inline std::string name_string(const reduce_unit<si::capacitance>::type&)   { return "farad"; }
inline std::string symbol_string(const reduce_unit<si::capacitance>::type&) { return "F"; }

inline std::string name_string(const reduce_unit<si::catalytic_activity>::type&) { return "katal"; }
inline std::string symbol_string(const reduce_unit<si::catalytic_activity>::type&) { return "kat"; }

inline std::string name_string(const reduce_unit<si::conductance>::type&) { return "siemen"; }
inline std::string symbol_string(const reduce_unit<si::conductance>::type&) { return "S"; }

// gray and sievert are indistinguishable
//inline std::string name_string(const si::dose_equivalent&) { return "sievert"; }
//inline std::string symbol_string(const si::dose_equivalent&) { return "Sv"; }

inline std::string name_string(const reduce_unit<si::electric_charge>::type&) { return "coulomb"; }
inline std::string symbol_string(const reduce_unit<si::electric_charge>::type&) { return "C"; }

inline std::string name_string(const reduce_unit<si::electric_potential>::type&) { return "volt"; }
inline std::string symbol_string(const reduce_unit<si::electric_potential>::type&) { return "V"; }

inline std::string name_string(const reduce_unit<si::energy>::type&) { return "joule"; }
inline std::string symbol_string(const reduce_unit<si::energy>::type&) { return "J"; }

inline std::string name_string(const reduce_unit<si::force>::type&) { return "newton"; }
inline std::string symbol_string(const reduce_unit<si::force>::type&) { return "N"; }

inline std::string name_string(const reduce_unit<si::frequency>::type&) { return "hertz"; }
inline std::string symbol_string(const reduce_unit<si::frequency>::type&) { return "Hz"; }

inline std::string name_string(const reduce_unit<si::illuminance>::type&) { return "lux"; }
inline std::string symbol_string(const reduce_unit<si::illuminance>::type&) { return "lx"; }

inline std::string name_string(const reduce_unit<si::inductance>::type&) { return "henry"; }
inline std::string symbol_string(const reduce_unit<si::inductance>::type&) { return "H"; }

inline std::string name_string(const reduce_unit<si::luminous_flux>::type&) { return "lumen"; }
inline std::string symbol_string(const reduce_unit<si::luminous_flux>::type&) { return "lm"; }

inline std::string name_string(const reduce_unit<si::magnetic_flux>::type&) { return "weber"; }
inline std::string symbol_string(const reduce_unit<si::magnetic_flux>::type&) { return "Wb"; }

inline std::string name_string(const reduce_unit<si::magnetic_flux_density>::type&) { return "tesla"; }
inline std::string symbol_string(const reduce_unit<si::magnetic_flux_density>::type&) { return "T"; }

inline std::string name_string(const reduce_unit<si::power>::type&) { return "watt"; }
inline std::string symbol_string(const reduce_unit<si::power>::type&) { return "W"; }

inline std::string name_string(const reduce_unit<si::pressure>::type&) { return "pascal"; }
inline std::string symbol_string(const reduce_unit<si::pressure>::type&) { return "Pa"; }

inline std::string name_string(const reduce_unit<si::resistance>::type&) { return "ohm"; }
inline std::string symbol_string(const reduce_unit<si::resistance>::type&) { return "Ohm"; }


} // namespace units

} // namespace boost

#endif // BOOST_UNITS_SI_IO_HPP
