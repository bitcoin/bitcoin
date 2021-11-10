// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_BASE_UNITS_INFORMATION_HARTLEY_HPP_INCLUDED
#define BOOST_UNITS_BASE_UNITS_INFORMATION_HARTLEY_HPP_INCLUDED

#include <boost/units/conversion.hpp>
#include <boost/units/base_units/information/bit.hpp>

BOOST_UNITS_DEFINE_BASE_UNIT_WITH_CONVERSIONS(information, hartley,
                                              "hartley", "Hart",
                                              3.321928094887363,
                                              boost::units::information::bit_base_unit, 
                                              -703);

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::information::hartley_base_unit)

#endif

#endif // BOOST_UNITS_BASE_UNITS_INFORMATION_HARTLEY_HPP_INCLUDED
