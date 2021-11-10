// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CONSTANTS_HPP
#define BOOST_UNITS_CONSTANTS_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <iomanip>

#include <boost/io/ios_state.hpp>

#include <boost/units/static_constant.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/operators.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/detail/one.hpp>

namespace boost {

namespace units {

template<class Base>
struct constant 
{ 
    typedef typename Base::value_type value_type; 
    BOOST_CONSTEXPR operator value_type() const    { return Base().value(); } 
    BOOST_CONSTEXPR value_type value() const       { return Base().value(); } 
    BOOST_CONSTEXPR value_type uncertainty() const { return Base().uncertainty(); } 
    BOOST_CONSTEXPR value_type lower_bound() const { return Base().lower_bound(); } 
    BOOST_CONSTEXPR value_type upper_bound() const { return Base().upper_bound(); } 
}; 

template<class Base>
struct physical_constant 
{ 
    typedef typename Base::value_type value_type; 
    BOOST_CONSTEXPR operator value_type() const    { return Base().value(); } 
    BOOST_CONSTEXPR value_type value() const       { return Base().value(); } 
    BOOST_CONSTEXPR value_type uncertainty() const { return Base().uncertainty(); } 
    BOOST_CONSTEXPR value_type lower_bound() const { return Base().lower_bound(); } 
    BOOST_CONSTEXPR value_type upper_bound() const { return Base().upper_bound(); } 
}; 

#define BOOST_UNITS_DEFINE_HELPER(name, symbol, template_name)  \
                                                                \
template<class T, class Arg1, class Arg2>                       \
struct name ## _typeof_helper<constant<T>, template_name<Arg1, Arg2> >\
{                                                               \
    typedef typename name ## _typeof_helper<typename T::value_type, template_name<Arg1, Arg2> >::type type;\
};                                                              \
                                                                \
template<class T, class Arg1, class Arg2>                       \
struct name ## _typeof_helper<template_name<Arg1, Arg2>, constant<T> >\
{                                                               \
    typedef typename name ## _typeof_helper<template_name<Arg1, Arg2>, typename T::value_type>::type type;\
};                                                              \
                                                                \
template<class T, class Arg1, class Arg2>                       \
BOOST_CONSTEXPR                                                 \
typename name ## _typeof_helper<typename T::value_type, template_name<Arg1, Arg2> >::type \
operator symbol(const constant<T>& t, const template_name<Arg1, Arg2>& u)\
{                                                               \
    return(t.value() symbol u);                                 \
}                                                               \
                                                                \
template<class T, class Arg1, class Arg2>                       \
BOOST_CONSTEXPR                                                 \
typename name ## _typeof_helper<template_name<Arg1, Arg2>, typename T::value_type>::type \
operator symbol(const template_name<Arg1, Arg2>& u, const constant<T>& t)\
{                                                               \
    return(u symbol t.value());                                 \
}

BOOST_UNITS_DEFINE_HELPER(add, +, unit)
BOOST_UNITS_DEFINE_HELPER(add, +, quantity)
BOOST_UNITS_DEFINE_HELPER(subtract, -, unit)
BOOST_UNITS_DEFINE_HELPER(subtract, -, quantity)
BOOST_UNITS_DEFINE_HELPER(multiply, *, unit)
BOOST_UNITS_DEFINE_HELPER(multiply, *, quantity)
BOOST_UNITS_DEFINE_HELPER(divide, /, unit)
BOOST_UNITS_DEFINE_HELPER(divide, /, quantity)

#undef BOOST_UNITS_DEFINE_HELPER

#define BOOST_UNITS_DEFINE_HELPER(name, symbol)                     \
                                                                    \
template<class T1, class T2>                                        \
struct name ## _typeof_helper<constant<T1>, constant<T2> >          \
{                                                                   \
    typedef typename name ## _typeof_helper<typename T1::value_type, typename T2::value_type>::type type;\
};                                                                  \
                                                                    \
template<class T1, class T2>                                        \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<typename T1::value_type, typename T2::value_type>::type \
operator symbol(const constant<T1>& t, const constant<T2>& u)       \
{                                                                   \
    return(t.value() symbol u.value());                             \
}                                                                   \
                                                                    \
template<class T1, class T2>                                        \
struct name ## _typeof_helper<constant<T1>, T2>                     \
{                                                                   \
    typedef typename name ## _typeof_helper<typename T1::value_type, T2>::type type;\
};                                                                  \
                                                                    \
template<class T1, class T2>                                        \
struct name ## _typeof_helper<T1, constant<T2> >                    \
{                                                                   \
    typedef typename name ## _typeof_helper<T1, typename T2::value_type>::type type;\
};                                                                  \
                                                                    \
template<class T1, class T2>                                        \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<typename T1::value_type, T2>::type  \
operator symbol(const constant<T1>& t, const T2& u)                 \
{                                                                   \
    return(t.value() symbol u);                                     \
}                                                                   \
                                                                    \
template<class T1, class T2>                                        \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<T1, typename T2::value_type>::type  \
operator symbol(const T1& t, const constant<T2>& u)                 \
{                                                                   \
    return(t symbol u.value());                                     \
}

BOOST_UNITS_DEFINE_HELPER(add, +)
BOOST_UNITS_DEFINE_HELPER(subtract, -)
BOOST_UNITS_DEFINE_HELPER(multiply, *)
BOOST_UNITS_DEFINE_HELPER(divide, /)

#undef BOOST_UNITS_DEFINE_HELPER

#define BOOST_UNITS_DEFINE_HELPER(name, symbol)                     \
                                                                    \
template<class T1>                                                  \
struct name ## _typeof_helper<constant<T1>, one>                    \
{                                                                   \
    typedef typename name ## _typeof_helper<typename T1::value_type, one>::type type;\
};                                                                  \
                                                                    \
template<class T2>                                                  \
struct name ## _typeof_helper<one, constant<T2> >                   \
{                                                                   \
    typedef typename name ## _typeof_helper<one, typename T2::value_type>::type type;\
};                                                                  \
                                                                    \
template<class T1>                                                  \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<typename T1::value_type, one>::type \
operator symbol(const constant<T1>& t, const one& u)                \
{                                                                   \
    return(t.value() symbol u);                                     \
}                                                                   \
                                                                    \
template<class T2>                                                  \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<one, typename T2::value_type>::type \
operator symbol(const one& t, const constant<T2>& u)                \
{                                                                   \
    return(t symbol u.value());                                     \
}

BOOST_UNITS_DEFINE_HELPER(multiply, *)
BOOST_UNITS_DEFINE_HELPER(divide, /)

#undef BOOST_UNITS_DEFINE_HELPER

template<class T1, long N, long D>
struct power_typeof_helper<constant<T1>, static_rational<N,D> >
{
    typedef power_typeof_helper<typename T1::value_type, static_rational<N,D> > base;
    typedef typename base::type type;
    static BOOST_CONSTEXPR type value(const constant<T1>& arg)
    {
        return base::value(arg.value());
    }
};

#define BOOST_UNITS_DEFINE_HELPER(name, symbol)                     \
                                                                    \
template<class T1, class E>                                         \
struct name ## _typeof_helper<constant<T1> >                        \
{                                                                   \
    typedef typename name ## _typeof_helper<typename T1::value_type, E>::type type;\
};                                                                  \
                                                                    \
template<class T1>                                                  \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<typename T1::value_type, one>::type \
operator symbol(const constant<T1>& t, const one& u)                \
{                                                                   \
    return(t.value() symbol u);                                     \
}                                                                   \
                                                                    \
template<class T2>                                                  \
BOOST_CONSTEXPR                                                     \
typename name ## _typeof_helper<one, typename T2::value_type>::type \
operator symbol(const one& t, const constant<T2>& u)                \
{                                                                   \
    return(t symbol u.value());                                     \
}

#define BOOST_UNITS_PHYSICAL_CONSTANT(name, type, value_, uncertainty_)             \
struct name ## _t {                                                                 \
    typedef type value_type;                                                        \
    BOOST_CONSTEXPR operator value_type() const    { return value_; }               \
    BOOST_CONSTEXPR value_type value() const       { return value_; }               \
    BOOST_CONSTEXPR value_type uncertainty() const { return uncertainty_; }         \
    BOOST_CONSTEXPR value_type lower_bound() const { return value_-uncertainty_; }  \
    BOOST_CONSTEXPR value_type upper_bound() const { return value_+uncertainty_; }  \
};                                                                                  \
BOOST_UNITS_STATIC_CONSTANT(name, boost::units::constant<boost::units::physical_constant<name ## _t> >) = { }

// stream output
template<class Char, class Traits, class Y>
inline
std::basic_ostream<Char,Traits>& operator<<(std::basic_ostream<Char,Traits>& os,const physical_constant<Y>& val)
{
    boost::io::ios_precision_saver precision_saver(os);
    //boost::io::ios_width_saver width_saver(os);
    boost::io::ios_flags_saver flags_saver(os);

    //os << std::setw(21);
    typedef typename Y::value_type value_type;
    
    if (val.uncertainty() > value_type())
    {
        const double relative_uncertainty = std::abs(val.uncertainty()/val.value());
    
        const double  exponent = std::log10(relative_uncertainty);
        const long digits_of_precision = static_cast<long>(std::ceil(std::abs(exponent)))+3;
        
        // should try to replicate NIST CODATA syntax 
        os << std::setprecision(digits_of_precision) 
           //<< std::setw(digits_of_precision+8) 
           //<< std::scientific
           << val.value();
//           << long(10*(relative_uncertainty/std::pow(Y(10),Y(exponent))));

        os << " (rel. unc. = " 
           << std::setprecision(1) 
           //<< std::setw(7) 
           << std::scientific
           << relative_uncertainty << ")";
    }
    else
    {
        os << val.value() << " (exact)";
    }
    
    return os;
}

// stream output
template<class Char, class Traits, class Y>
inline
std::basic_ostream<Char,Traits>& operator<<(std::basic_ostream<Char,Traits>& os,const constant<Y>&)
{
    os << Y();
    return os;
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CONSTANTS_HPP
