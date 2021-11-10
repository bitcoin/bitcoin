// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_BASE_UNITS_INFORMATION_SHANNON_HPP_INCLUDED
#define BOOST_UNITS_BASE_UNITS_INFORMATION_SHANNON_HPP_INCLUDED

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/base_units/information/bit.hpp>

namespace boost {
namespace units {
namespace information {

typedef scaled_base_unit<boost::units::information::bit_base_unit, scale<1, static_rational<1> > > shannon_base_unit;

} // namespace information

template<>
struct base_unit_info<information::shannon_base_unit> {
    static BOOST_CONSTEXPR const char* name()   { return("shannon"); }
    static BOOST_CONSTEXPR const char* symbol() { return("Sh"); }
};

} // namespace units
} // namespace boost

#endif // BOOST_UNITS_BASE_UNITS_INFORMATION_SHANNON_HPP_INCLUDED
