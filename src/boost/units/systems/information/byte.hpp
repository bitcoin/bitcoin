// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SYSTEMS_INFORMATION_BYTE_HPP_INCLUDED
#define BOOST_UNITS_SYSTEMS_INFORMATION_BYTE_HPP_INCLUDED

#include <boost/units/make_system.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/static_constant.hpp>

#include <boost/units/base_units/information/byte.hpp>

namespace boost {
namespace units { 
namespace information {

typedef make_system<byte_base_unit>::type system;

typedef unit<dimensionless_type, system> dimensionless;

namespace hu {
namespace byte {
typedef unit<information_dimension, system> info;
} // namespace bit
} // namespace hu

BOOST_UNITS_STATIC_CONSTANT(byte, hu::byte::info);
BOOST_UNITS_STATIC_CONSTANT(bytes, hu::byte::info);

// I'm going to define boost::units::information::info (the "default")
// to be hu::byte::info -- other variants such as hu::bit::info, hu::nat::info, etc
// must be explicitly referred to
typedef hu::byte::info info;

} // namespace information
} // namespace units
} // namespace boost

#endif // BOOST_UNITS_SYSTEMS_INFORMATION_BYTE_HPP_INCLUDED
