// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SI_AMOUNT_HPP
#define BOOST_UNITS_SI_AMOUNT_HPP

#include <boost/units/systems/si/base.hpp>

namespace boost {

namespace units { 

namespace si {

typedef unit<amount_dimension,si::system>    amount;

BOOST_UNITS_STATIC_CONSTANT(mole,amount);
BOOST_UNITS_STATIC_CONSTANT(moles,amount);

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_SI_AMOUNT_HPP
