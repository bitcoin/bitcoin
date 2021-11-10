// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SYSTEMS_INFORMATION_PREFIXES_HPP_INCLUDED
#define BOOST_UNITS_SYSTEMS_INFORMATION_PREFIXES_HPP_INCLUDED

#include <boost/units/make_scaled_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>

#include <boost/units/systems/information/byte.hpp>

#define BOOST_UNITS_INFOSYS_PREFIX(exponent, name) \
    typedef make_scaled_unit<dimensionless, scale<2, static_rational<exponent> > >::type name ## _pf_type; \
    BOOST_UNITS_STATIC_CONSTANT(name, name ## _pf_type)

namespace boost {
namespace units {
namespace information {

// Note, these are defined (somewhat arbitrarily) against the 'byte' system.
// They work smoothly with bit_information, nat_information, etc, so it is
// transparent to the user.
BOOST_UNITS_INFOSYS_PREFIX(10, kibi);
BOOST_UNITS_INFOSYS_PREFIX(20, mebi);
BOOST_UNITS_INFOSYS_PREFIX(30, gibi);
BOOST_UNITS_INFOSYS_PREFIX(40, tebi);
BOOST_UNITS_INFOSYS_PREFIX(50, pebi);
BOOST_UNITS_INFOSYS_PREFIX(60, exbi);
BOOST_UNITS_INFOSYS_PREFIX(70, zebi);
BOOST_UNITS_INFOSYS_PREFIX(80, yobi);

} // namespace information
} // namespace units
} // namespace boost

#undef BOOST_UNITS_INFOSYS_PREFIX

#endif // BOOST_UNITS_SYSTEMS_INFORMATION_PREFIXES_HPP_INCLUDED
