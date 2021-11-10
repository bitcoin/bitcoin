// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_PLANE_ANGLE_BASE_DIMENSION_HPP
#define BOOST_UNITS_PLANE_ANGLE_BASE_DIMENSION_HPP

#include <boost/units/config.hpp>
#include <boost/units/base_dimension.hpp>

namespace boost {

namespace units { 

/// base dimension of plane angle
struct plane_angle_base_dimension : 
    boost::units::base_dimension<plane_angle_base_dimension,-2> 
{ };               

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(boost::units::plane_angle_base_dimension)

#endif

namespace boost {

namespace units {

/// base dimension of plane angle (QP)
typedef plane_angle_base_dimension::dimension_type    plane_angle_dimension;

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_PLANE_ANGLE_BASE_DIMENSION_HPP
