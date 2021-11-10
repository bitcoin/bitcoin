// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IS_UNIT_HPP
#define BOOST_UNITS_IS_UNIT_HPP

///
/// \file
/// \brief Check that a type is a unit.
///

#include <boost/mpl/bool.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

/// Check that a type is a unit.
template<class T>
struct is_unit : 
    public mpl::false_
{ };

template<class Dim,class System>
struct is_unit< unit<Dim,System> > :
    public mpl::true_
{ };

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_IS_UNIT_HPP
