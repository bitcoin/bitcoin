// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SYSTEMS_INFORMATION_HARTLEY_HPP_INCLUDED
#define BOOST_UNITS_SYSTEMS_INFORMATION_HARTLEY_HPP_INCLUDED

#include <boost/units/systems/information/byte.hpp>
#include <boost/units/base_units/information/hartley.hpp>

namespace boost {
namespace units { 
namespace information {

namespace hu {
namespace hartley {
typedef unit<information_dimension, make_system<hartley_base_unit>::type> info;
} // namespace bit
} // namespace hu

BOOST_UNITS_STATIC_CONSTANT(hartley, hu::hartley::info);
BOOST_UNITS_STATIC_CONSTANT(hartleys, hu::hartley::info);

} // namespace information
} // namespace units
} // namespace boost

#endif // BOOST_UNITS_SYSTEMS_INFORMATION_HARTLEY_HPP_INCLUDED
