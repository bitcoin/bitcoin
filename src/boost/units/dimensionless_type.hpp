// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DIMENSIONLESS_TYPE_HPP
#define BOOST_UNITS_DIMENSIONLESS_TYPE_HPP

///
/// \file
/// \brief Dimension lists in which all exponents resolve to zero reduce to @c dimensionless_type.
///

#include <boost/mpl/long.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/arithmetic.hpp>

#include <boost/units/config.hpp>

namespace boost {

namespace units {

namespace detail {

struct dimension_list_tag;

}

/// Dimension lists in which all exponents resolve to zero reduce to @c dimensionless_type.
struct dimensionless_type
{
    typedef dimensionless_type          type;
    typedef detail::dimension_list_tag  tag;
    typedef mpl::long_<0>               size;
};

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::dimensionless_type)

#endif

#endif // BOOST_UNITS_DIMENSIONLESS_TYPE_HPP
