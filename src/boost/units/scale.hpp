// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SCALE_HPP_INCLUDED
#define BOOST_UNITS_SCALE_HPP_INCLUDED

///
/// \file
/// \brief 10^3 Engineering & 2^10 binary scaling factors for autoprefixing.
/// \details
///

#include <string>

#include <boost/units/config.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/one.hpp>
#include <boost/units/detail/static_rational_power.hpp>

namespace boost {

namespace units {

template<class S, class Scale>
struct scaled_base_unit;

/// class representing a scaling factor such as 10^3
/// The exponent must be a static rational.
template<long Base, class Exponent>
struct scale
{
    BOOST_STATIC_CONSTEXPR long base = Base;
    typedef Exponent exponent;
    typedef double value_type;
    static BOOST_CONSTEXPR value_type value() { return(detail::static_rational_power<Exponent>(static_cast<double>(base))); }
    // These need to be defined in specializations for
    // printing to work.
    // static std::string name();
    // static std::string symbol();
};

template<long Base, class Exponent>
BOOST_CONSTEXPR_OR_CONST long scale<Base, Exponent>::base;

/// INTERNAL ONLY
template<long Base>
struct scale<Base, static_rational<0> >
{
    BOOST_STATIC_CONSTEXPR long base = Base;
    typedef static_rational<0> exponent;
    typedef one value_type;
    static BOOST_CONSTEXPR one value() { return(one()); }
    static std::string name() { return(""); }
    static std::string symbol() { return(""); }
};

template<long Base>
BOOST_CONSTEXPR_OR_CONST long scale<Base, static_rational<0> >::base;

template<long Base,class Exponent>
std::string symbol_string(const scale<Base,Exponent>&)
{
    return scale<Base,Exponent>::symbol();
}

template<long Base,class Exponent>
std::string name_string(const scale<Base,Exponent>&)
{
    return scale<Base,Exponent>::name();
}

#ifndef BOOST_UNITS_DOXYGEN

#define BOOST_UNITS_SCALE_SPECIALIZATION(base_,exponent_,val_,name_,symbol_) \
template<>                                                                   \
struct scale<base_, exponent_ >                                              \
{                                                                            \
    BOOST_STATIC_CONSTEXPR long base = base_;                                \
    typedef exponent_ exponent;                                              \
    typedef double value_type;                                               \
    static BOOST_CONSTEXPR value_type value()   { return(val_); }            \
    static std::string name()   { return(#name_); }                          \
    static std::string symbol() { return(#symbol_); }                        \
}

#define BOOST_UNITS_SCALE_DEF(exponent_,value_,name_,symbol_)                 \
BOOST_UNITS_SCALE_SPECIALIZATION(10,static_rational<exponent_>,value_, name_, symbol_)

BOOST_UNITS_SCALE_DEF(-24, 1e-24, yocto, y);
BOOST_UNITS_SCALE_DEF(-21, 1e-21, zepto, z);
BOOST_UNITS_SCALE_DEF(-18, 1e-18, atto, a);
BOOST_UNITS_SCALE_DEF(-15, 1e-15, femto, f);
BOOST_UNITS_SCALE_DEF(-12, 1e-12, pico, p);
BOOST_UNITS_SCALE_DEF(-9, 1e-9, nano, n);
BOOST_UNITS_SCALE_DEF(-6, 1e-6, micro, u);
BOOST_UNITS_SCALE_DEF(-3, 1e-3, milli, m);
BOOST_UNITS_SCALE_DEF(-2, 1e-2, centi, c);
BOOST_UNITS_SCALE_DEF(-1, 1e-1, deci, d);

BOOST_UNITS_SCALE_DEF(1, 1e1, deka, da);
BOOST_UNITS_SCALE_DEF(2, 1e2, hecto, h);
BOOST_UNITS_SCALE_DEF(3, 1e3, kilo, k);
BOOST_UNITS_SCALE_DEF(6, 1e6, mega, M);
BOOST_UNITS_SCALE_DEF(9, 1e9, giga, G);
BOOST_UNITS_SCALE_DEF(12, 1e12, tera, T);
BOOST_UNITS_SCALE_DEF(15, 1e15, peta, P);
BOOST_UNITS_SCALE_DEF(18, 1e18, exa, E);
BOOST_UNITS_SCALE_DEF(21, 1e21, zetta, Z);
BOOST_UNITS_SCALE_DEF(24, 1e24, yotta, Y);

BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<10>, 1024.0, kibi, Ki);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<20>, 1048576.0, mebi, Mi);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<30>, 1073741824.0, gibi, Gi);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<40>, 1099511627776.0, tebi, Ti);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<50>, 1125899906842624.0, pebi, Pi);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<60>, 1152921504606846976.0, exbi, Ei);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<70>, 1180591620717411303424.0, zebi, Zi);
BOOST_UNITS_SCALE_SPECIALIZATION(2, static_rational<80>, 1208925819614629174706176.0, yobi, Yi);

#undef BOOST_UNITS_SCALE_DEF
#undef BOOST_UNITS_SCALE_SPECIALIZATION

#endif

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::scale, (long)(class))

#endif

#endif
