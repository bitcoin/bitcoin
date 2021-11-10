// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_UNITS_FWD_HPP
#define BOOST_UNITS_UNITS_FWD_HPP

///
/// \file
/// \brief Forward declarations of library components.
/// \details Forward declarations of units library - dimensions, systems, quantity and string components.
///

#ifndef BOOST_UNITS_DOXYGEN

#include <string>

namespace boost {

namespace units {

template<typename T,typename V> struct dim;
template<typename T> struct is_dim;

struct dimensionless_type;
template<class Item,class Next> struct list;
template<typename Seq> struct make_dimension_list;

template<class T> struct is_dimensionless;
template<class S1,class S2> struct is_implicitly_convertible;
template<class T> struct get_dimension;
template<class T> struct get_system;

template<class Y> class absolute;

template<class Dim,class System, class Enable=void> class unit;

template<long Base, class Exponent> struct scale;

template<class BaseUnitTag> struct base_unit_info;
template<class System> struct dimensionless_unit;
template<class T> struct is_unit;
template<class T,class Dim> struct is_unit_of_dimension;
template<class T,class System> struct is_unit_of_system;

template<class Unit,class Y = double> class quantity;

template<class System,class Y> struct dimensionless_quantity;
template<class T> struct is_quantity;
template<class T,class Dim> struct is_quantity_of_dimension;
template<class T,class System> struct is_quantity_of_system;

template<class From,class To> struct conversion_helper;

template<class T> std::string to_string(const T&);
template<class T> std::string name_string(const T&);
template<class T> std::string symbol_string(const T&);
template<class T> std::string raw_string(const T&);
template<class T> std::string typename_string(const T&);

} // namespace units

} // namespace boost

#endif

#endif // BOOST_UNITS_UNITS_FWD_HPP
