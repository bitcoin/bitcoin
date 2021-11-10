// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_BASE_UNITS_INFORMATION_BIT_HPP_INCLUDED
#define BOOST_UNITS_BASE_UNITS_INFORMATION_BIT_HPP_INCLUDED

#include <string>

#include <boost/units/config.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/physical_dimensions/information.hpp>

namespace boost {
namespace units {
namespace information {

struct bit_base_unit : public base_unit<bit_base_unit, information_dimension, -700>
{
    static std::string name()   { return("bit"); }
    static std::string symbol() { return("b"); }
};

} // namespace information
} // namespace units
} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::information::bit_base_unit)

#endif

#endif // BOOST_UNITS_BASE_UNITS_INFORMATION_BIT_HPP_INCLUDED
