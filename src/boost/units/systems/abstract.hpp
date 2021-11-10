// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_ABSTRACT_HPP
#define BOOST_UNITS_ABSTRACT_HPP

#include <string>

#include <boost/units/conversion.hpp>
#include <boost/units/unit.hpp>

#include <boost/units/make_system.hpp>
#include <boost/units/base_unit.hpp>

#include <boost/units/physical_dimensions/amount.hpp>
#include <boost/units/physical_dimensions/current.hpp>
#include <boost/units/physical_dimensions/length.hpp>
#include <boost/units/physical_dimensions/luminous_intensity.hpp>
#include <boost/units/physical_dimensions/mass.hpp>
#include <boost/units/physical_dimensions/plane_angle.hpp>
#include <boost/units/physical_dimensions/solid_angle.hpp>
#include <boost/units/physical_dimensions/temperature.hpp>
#include <boost/units/physical_dimensions/time.hpp>

namespace boost {

namespace units {

namespace abstract {

struct length_unit_tag : base_unit<length_unit_tag, length_dimension, -30> { };
struct mass_unit_tag : base_unit<mass_unit_tag, mass_dimension, -29> { };
struct time_unit_tag : base_unit<time_unit_tag, time_dimension, -28> { };
struct current_unit_tag : base_unit<current_unit_tag, current_dimension, -27> { };
struct temperature_unit_tag : base_unit<temperature_unit_tag, temperature_dimension, -26> { };
struct amount_unit_tag : base_unit<amount_unit_tag, amount_dimension, -25> { };
struct luminous_intensity_unit_tag : base_unit<luminous_intensity_unit_tag, luminous_intensity_dimension, -24> { };
struct plane_angle_unit_tag : base_unit<plane_angle_unit_tag, plane_angle_dimension, -23> { };
struct solid_angle_unit_tag : base_unit<solid_angle_unit_tag, solid_angle_dimension, -22> { };

typedef make_system<
    length_unit_tag,
    mass_unit_tag,
    time_unit_tag,
    current_unit_tag,
    temperature_unit_tag,
    amount_unit_tag,
    luminous_intensity_unit_tag,
    plane_angle_unit_tag,
    solid_angle_unit_tag
>::type system;

typedef unit<length_dimension,system>                length;                 ///< abstract unit of length
typedef unit<mass_dimension,system>                  mass;                   ///< abstract unit of mass
typedef unit<time_dimension,system>                  time;                   ///< abstract unit of time
typedef unit<current_dimension,system>               current;                ///< abstract unit of current
typedef unit<temperature_dimension,system>           temperature;            ///< abstract unit of temperature
typedef unit<amount_dimension,system>                amount;                 ///< abstract unit of amount
typedef unit<luminous_intensity_dimension,system>    luminous_intensity;     ///< abstract unit of luminous intensity
typedef unit<plane_angle_dimension,system>           plane_angle;            ///< abstract unit of plane angle
typedef unit<solid_angle_dimension,system>           solid_angle;            ///< abstract unit of solid angle

} // namespace abstract

template<> 
struct base_unit_info<abstract::length_unit_tag> 
{ 
    static std::string name()       { return "[Length]"; }
    static std::string symbol()     { return "[L]"; }
};

template<> 
struct base_unit_info<abstract::mass_unit_tag> 
{ 
    static std::string name()       { return "[Mass]"; }
    static std::string symbol()     { return "[M]"; }
};

template<> 
struct base_unit_info<abstract::time_unit_tag> 
{ 
    static std::string name()       { return "[Time]"; }
    static std::string symbol()     { return "[T]"; }
};

template<> 
struct base_unit_info<abstract::current_unit_tag> 
{ 
    static std::string name()       { return "[Electric Current]"; }
    static std::string symbol()     { return "[I]"; }
};

template<> 
struct base_unit_info<abstract::temperature_unit_tag> 
{ 
    static std::string name()       { return "[Temperature]"; }
    static std::string symbol()     { return "[Theta]"; }
};

template<> 
struct base_unit_info<abstract::amount_unit_tag> 
{ 
    static std::string name()       { return "[Amount]"; }
    static std::string symbol()     { return "[N]"; }
};

template<> 
struct base_unit_info<abstract::luminous_intensity_unit_tag> 
{ 
    static std::string name()       { return "[Luminous Intensity]"; }
    static std::string symbol()     { return "[J]"; }
};

template<> 
struct base_unit_info<abstract::plane_angle_unit_tag> 
{ 
    static std::string name()       { return "[Plane Angle]"; }
    static std::string symbol()     { return "[QP]"; }
};

template<> 
struct base_unit_info<abstract::solid_angle_unit_tag> 
{ 
    static std::string name()       { return "[Solid Angle]"; }
    static std::string symbol()     { return "[QS]"; }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_ABSTRACT_HPP
