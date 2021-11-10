// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNIT_SYSTEMS_IMPERIAL_DRACHM_HPP_INCLUDED
#define BOOST_UNIT_SYSTEMS_IMPERIAL_DRACHM_HPP_INCLUDED

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_units/imperial/pound.hpp>

namespace boost {

namespace units {

namespace imperial {

typedef scaled_base_unit<pound_base_unit, scale<16, static_rational<-2> > > drachm_base_unit;

} // namespace imperial

template<>
struct base_unit_info<imperial::drachm_base_unit> {
    static BOOST_CONSTEXPR const char* name()   { return("drachm"); }
    static BOOST_CONSTEXPR const char* symbol() { return("drachm"); }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNIT_SYSTEMS_IMPERIAL_DRACHM_HPP_INCLUDED
