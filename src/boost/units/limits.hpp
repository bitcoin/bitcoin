// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_LIMITS_HPP
#define BOOST_UNITS_LIMITS_HPP

///
/// \file
/// \brief specialize std::numeric_limits for units.
///

#include <limits>

#include <boost/config.hpp>
#include <boost/units/units_fwd.hpp>

namespace std {

template<class Unit, class T>
class numeric_limits< ::boost::units::quantity<Unit, T> >
{
    public:
        typedef ::boost::units::quantity<Unit, T> quantity_type;
        BOOST_STATIC_CONSTEXPR bool is_specialized = std::numeric_limits<T>::is_specialized;
        static BOOST_CONSTEXPR quantity_type (min)() { return(quantity_type::from_value((std::numeric_limits<T>::min)())); }
        static BOOST_CONSTEXPR quantity_type (max)() { return(quantity_type::from_value((std::numeric_limits<T>::max)())); }
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
        static BOOST_CONSTEXPR quantity_type (lowest)() { return(quantity_type::from_value((std::numeric_limits<T>::lowest)())); }
#endif
        BOOST_STATIC_CONSTEXPR int digits = std::numeric_limits<T>::digits;
        BOOST_STATIC_CONSTEXPR int digits10 = std::numeric_limits<T>::digits10;
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
        BOOST_STATIC_CONSTEXPR int max_digits10 = std::numeric_limits<T>::max_digits10;
#endif
        BOOST_STATIC_CONSTEXPR bool is_signed = std::numeric_limits<T>::is_signed;
        BOOST_STATIC_CONSTEXPR bool is_integer = std::numeric_limits<T>::is_integer;
        BOOST_STATIC_CONSTEXPR bool is_exact = std::numeric_limits<T>::is_exact;
        BOOST_STATIC_CONSTEXPR int radix = std::numeric_limits<T>::radix;
        static BOOST_CONSTEXPR quantity_type epsilon()  { return(quantity_type::from_value(std::numeric_limits<T>::epsilon())); }
        static BOOST_CONSTEXPR quantity_type round_error()  { return(quantity_type::from_value(std::numeric_limits<T>::round_error())); }
        BOOST_STATIC_CONSTEXPR int min_exponent = std::numeric_limits<T>::min_exponent;
        BOOST_STATIC_CONSTEXPR int min_exponent10 = std::numeric_limits<T>::min_exponent10;
        BOOST_STATIC_CONSTEXPR int max_exponent = std::numeric_limits<T>::max_exponent;
        BOOST_STATIC_CONSTEXPR int max_exponent10 = std::numeric_limits<T>::max_exponent10;
        BOOST_STATIC_CONSTEXPR bool has_infinity = std::numeric_limits<T>::has_infinity;
        BOOST_STATIC_CONSTEXPR bool has_quiet_NaN = std::numeric_limits<T>::has_quiet_NaN;
        BOOST_STATIC_CONSTEXPR bool has_signaling_NaN = std::numeric_limits<T>::has_signaling_NaN;
        BOOST_STATIC_CONSTEXPR bool has_denorm_loss = std::numeric_limits<T>::has_denorm_loss;
        static BOOST_CONSTEXPR quantity_type infinity()  { return(quantity_type::from_value(std::numeric_limits<T>::infinity())); }
        static BOOST_CONSTEXPR quantity_type quiet_NaN()  { return(quantity_type::from_value(std::numeric_limits<T>::quiet_NaN())); }
        static BOOST_CONSTEXPR quantity_type signaling_NaN()  { return(quantity_type::from_value(std::numeric_limits<T>::signaling_NaN())); }
        static BOOST_CONSTEXPR quantity_type denorm_min()  { return(quantity_type::from_value(std::numeric_limits<T>::denorm_min())); }
        BOOST_STATIC_CONSTEXPR bool is_iec559 = std::numeric_limits<T>::is_iec559;
        BOOST_STATIC_CONSTEXPR bool is_bounded = std::numeric_limits<T>::is_bounded;
        BOOST_STATIC_CONSTEXPR bool is_modulo = std::numeric_limits<T>::is_modulo;
        BOOST_STATIC_CONSTEXPR bool traps = std::numeric_limits<T>::traps;
        BOOST_STATIC_CONSTEXPR bool tinyness_before = std::numeric_limits<T>::tinyness_before;
#if defined(_STLP_STATIC_CONST_INIT_BUG)
        BOOST_STATIC_CONSTEXPR int has_denorm = std::numeric_limits<T>::has_denorm;
        BOOST_STATIC_CONSTEXPR int round_style = std::numeric_limits<T>::round_style;
#else
        BOOST_STATIC_CONSTEXPR float_denorm_style has_denorm = std::numeric_limits<T>::has_denorm;
        BOOST_STATIC_CONSTEXPR float_round_style round_style = std::numeric_limits<T>::round_style;
#endif
};

}

#endif // BOOST_UNITS_LIMITS_HPP
