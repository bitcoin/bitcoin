// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_PREVENT_REDEFINITION_HPP
#define BOOST_UNITS_DETAIL_PREVENT_REDEFINITION_HPP

#include <boost/mpl/long.hpp>

namespace boost {

namespace units {

namespace detail {

struct no { BOOST_CONSTEXPR no() : dummy() {} char dummy; };
struct yes { no dummy[2]; };

template<bool> struct ordinal_has_already_been_defined;

template<>
struct ordinal_has_already_been_defined<true>   { };

template<>
struct ordinal_has_already_been_defined<false>  { typedef void type; };

}

/// This must be in namespace boost::units so that ADL
/// will work.  we need a mangled name because it must
/// be found by ADL
/// INTERNAL ONLY
template<class T>
BOOST_CONSTEXPR
detail::no 
boost_units_is_registered(const T&) 
{ return(detail::no()); }

/// INTERNAL ONLY
template<class T>
BOOST_CONSTEXPR
detail::no 
boost_units_unit_is_registered(const T&) 
{ return(detail::no()); }

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_PREVENT_ORDINAL_REDEFINITION_IMPL_HPP
