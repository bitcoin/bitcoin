// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_OPERATORS_HPP 
#define BOOST_UNITS_OPERATORS_HPP


///
/// \file
/// \brief Compile time operators and typeof helper classes.
/// \details
///   These operators declare the compile-time operators needed to support dimensional
///   analysis algebra.  They require the use of Boost.Typeof, emulation or native.
///   Typeof helper classes define result type for heterogeneous operators on value types.
///   These must be defined through specialization for powers and roots.
///

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/config.hpp>

namespace boost {
namespace units {

#if BOOST_UNITS_HAS_TYPEOF

#ifndef BOOST_UNITS_DOXYGEN

// to avoid need for default constructor and eliminate divide by zero errors.
namespace typeof_ {

/// INTERNAL ONLY
template<class T> T make();

} // namespace typeof_

#endif

#if (BOOST_UNITS_HAS_BOOST_TYPEOF)

template<typename X> struct unary_plus_typeof_helper            
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (+typeof_::make<X>()))
    typedef typename nested::type type;
};

template<typename X> struct unary_minus_typeof_helper           
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (-typeof_::make<X>()))
    typedef typename nested::type type;
};

template<typename X,typename Y> struct add_typeof_helper        
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (typeof_::make<X>()+typeof_::make<Y>()))
    typedef typename nested::type type;
};

template<typename X,typename Y> struct subtract_typeof_helper   
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (typeof_::make<X>()-typeof_::make<Y>()))
    typedef typename nested::type type;
};

template<typename X,typename Y> struct multiply_typeof_helper   
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (typeof_::make<X>()*typeof_::make<Y>()))
    typedef typename nested::type type;
};

template<typename X,typename Y> struct divide_typeof_helper     
{
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (typeof_::make<X>()/typeof_::make<Y>()))
    typedef typename nested::type type;
};

#elif (BOOST_UNITS_HAS_MWERKS_TYPEOF)

template<typename X> struct unary_plus_typeof_helper            { typedef __typeof__((+typeof_::make<X>())) type; };
template<typename X> struct unary_minus_typeof_helper           { typedef __typeof__((-typeof_::make<X>())) type; };

template<typename X,typename Y> struct add_typeof_helper        { typedef __typeof__((typeof_::make<X>()+typeof_::make<Y>())) type; };
template<typename X,typename Y> struct subtract_typeof_helper   { typedef __typeof__((typeof_::make<X>()-typeof_::make<Y>())) type; };
template<typename X,typename Y> struct multiply_typeof_helper   { typedef __typeof__((typeof_::make<X>()*typeof_::make<Y>())) type; };
template<typename X,typename Y> struct divide_typeof_helper     { typedef __typeof__((typeof_::make<X>()/typeof_::make<Y>())) type; };

#elif (BOOST_UNITS_HAS_GNU_TYPEOF) || defined(BOOST_UNITS_DOXYGEN)

template<typename X> struct unary_plus_typeof_helper            { typedef typeof((+typeof_::make<X>())) type; };
template<typename X> struct unary_minus_typeof_helper           { typedef typeof((-typeof_::make<X>())) type; };

template<typename X,typename Y> struct add_typeof_helper        { typedef typeof((typeof_::make<X>()+typeof_::make<Y>())) type; };
template<typename X,typename Y> struct subtract_typeof_helper   { typedef typeof((typeof_::make<X>()-typeof_::make<Y>())) type; };
template<typename X,typename Y> struct multiply_typeof_helper   { typedef typeof((typeof_::make<X>()*typeof_::make<Y>())) type; };
template<typename X,typename Y> struct divide_typeof_helper     { typedef typeof((typeof_::make<X>()/typeof_::make<Y>())) type; };

#endif

#else // BOOST_UNITS_HAS_TYPEOF

template<typename X> struct unary_plus_typeof_helper            { typedef X type; };
template<typename X> struct unary_minus_typeof_helper           { typedef X type; };

template<typename X,typename Y> struct add_typeof_helper        { BOOST_STATIC_ASSERT((is_same<X,Y>::value == true)); typedef X type; };
template<typename X,typename Y> struct subtract_typeof_helper   { BOOST_STATIC_ASSERT((is_same<X,Y>::value == true)); typedef X type; };
template<typename X,typename Y> struct multiply_typeof_helper   { BOOST_STATIC_ASSERT((is_same<X,Y>::value == true)); typedef X type; };
template<typename X,typename Y> struct divide_typeof_helper     { BOOST_STATIC_ASSERT((is_same<X,Y>::value == true)); typedef X type; };

#endif // BOOST_UNITS_HAS_TYPEOF

template<typename X,typename Y> struct power_typeof_helper;
template<typename X,typename Y> struct root_typeof_helper;

#ifdef BOOST_UNITS_DOXYGEN

/// A helper used by @c pow to raise
/// a runtime object to a compile time
/// known exponent.  This template is intended to
/// be specialized.  All specializations must
/// conform to the interface shown here.
/// @c Exponent will be either the exponent
/// passed to @c pow or @c static_rational<N>
/// for and integer argument, N.
template<typename BaseType, typename Exponent>
struct power_typeof_helper
{
    /// specifies the result type
    typedef detail::unspecified type;
    /// Carries out the runtime calculation.
    static BOOST_CONSTEXPR type value(const BaseType& base);
};

/// A helper used by @c root to take a root
/// of a runtime object using a compile time
/// known index.  This template is intended to
/// be specialized.  All specializations must
/// conform to the interface shown here.
/// @c Index will be either the type
/// passed to @c pow or @c static_rational<N>
/// for and integer argument, N.
template<typename Radicand, typename Index>
struct root_typeof_helper
{
    /// specifies the result type
    typedef detail::unspecified type;
    /// Carries out the runtime calculation.
    static BOOST_CONSTEXPR type value(const Radicand& base);
};

#endif

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_OPERATORS_HPP
