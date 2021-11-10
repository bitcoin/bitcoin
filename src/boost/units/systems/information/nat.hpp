// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SYSTEMS_INFORMATION_NAT_HPP_INCLUDED
#define BOOST_UNITS_SYSTEMS_INFORMATION_NAT_HPP_INCLUDED

#include <boost/units/systems/information/byte.hpp>
#include <boost/units/base_units/information/nat.hpp>

namespace boost {
namespace units { 
namespace information {

namespace hu {
namespace nat {
typedef unit<information_dimension, make_system<nat_base_unit>::type> info;
} // namespace bit
} // namespace hu

BOOST_UNITS_STATIC_CONSTANT(nat, hu::nat::info);
BOOST_UNITS_STATIC_CONSTANT(nats, hu::nat::info);

} // namespace information
} // namespace units
} // namespace boost

#endif // BOOST_UNITS_SYSTEMS_INFORMATION_NAT_HPP_INCLUDED
