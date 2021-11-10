// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CMATH_HPP 
#define BOOST_UNITS_CMATH_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <cstdlib>

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/hypot.hpp>
#include <boost/math/special_functions/next.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>

#include <boost/units/dimensionless_quantity.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/detail/cmath_impl.hpp>
#include <boost/units/detail/dimensionless_unit.hpp>

#include <boost/units/systems/si/plane_angle.hpp>

/// \file
/// \brief Overloads of functions in \<cmath\> for quantities.
/// \details Only functions for which a dimensionally-correct result type
///   can be determined are overloaded.
///   All functions work with dimensionless quantities.

// BOOST_PREVENT_MACRO_SUBSTITUTION is needed on certain compilers that define 
// some <cmath> functions as macros; it is used for all functions even though it
// isn't necessary -- I didn't want to think :)
//
// the form using namespace detail; return(f(x)); is used
// to enable ADL for UDTs.

namespace boost {

namespace units { 

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isfinite BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::isfinite;
    return isfinite BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isinf BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::isinf;
    return isinf BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline
BOOST_CONSTEXPR
bool 
isnan BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::isnan;
    return isnan BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isnormal BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::isnormal;
    return isnormal BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isgreater BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                            const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return isgreater BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                                 const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isless BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                         const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return isless BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
islessequal BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                              const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return islessequal BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                                const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
bool 
isunordered BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                              const quantity<Unit,Y>& q2)
{ 
    using namespace detail;
    return isunordered BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
abs BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using std::abs;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(abs BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
ceil BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using std::ceil;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(ceil BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
copysign BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
         const quantity<Unit,Y>& q2)
{
    using boost::math::copysign;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(copysign BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
fabs BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using std::fabs;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(fabs BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
floor BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using std::floor;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(floor BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
fdim BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                       const quantity<Unit,Y>& q2)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(fdim BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

#if 0

template<class Unit1,class Unit2,class Unit3,class Y>
inline 
BOOST_CONSTEXPR
typename add_typeof_helper<
    typename multiply_typeof_helper<quantity<Unit1,Y>,
                                    quantity<Unit2,Y> >::type,
    quantity<Unit3,Y> >::type 
fma BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit1,Y>& q1,
                                      const quantity<Unit2,Y>& q2,
                                      const quantity<Unit3,Y>& q3)
{
    using namespace detail;

    typedef quantity<Unit1,Y>   type1;
    typedef quantity<Unit2,Y>   type2;
    typedef quantity<Unit3,Y>   type3;
    
    typedef typename multiply_typeof_helper<type1,type2>::type  prod_type;
    typedef typename add_typeof_helper<prod_type,type3>::type   quantity_type;
    
    return quantity_type::from_value(fma BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value(),q3.value()));
}

#endif

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
fmax BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                       const quantity<Unit,Y>& q2)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(fmax BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
fmin BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                       const quantity<Unit,Y>& q2)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(fmin BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
int 
fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::fpclassify;

    return fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
typename root_typeof_helper<
    typename add_typeof_helper<
        typename power_typeof_helper<quantity<Unit,Y>,
                                     static_rational<2> >::type,
        typename power_typeof_helper<quantity<Unit,Y>,
                                     static_rational<2> >::type>::type,
        static_rational<2> >::type
hypot BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,const quantity<Unit,Y>& q2)
{
    using boost::math::hypot;

    typedef quantity<Unit,Y>    type1;
    
    typedef typename power_typeof_helper<type1,static_rational<2> >::type   pow_type;
    typedef typename add_typeof_helper<pow_type,pow_type>::type             add_type;
    typedef typename root_typeof_helper<add_type,static_rational<2> >::type quantity_type;
        
    return quantity_type::from_value(hypot BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

// does ISO C++ support long long? g++ claims not
//template<class Unit,class Y>
//inline 
//BOOST_CONSTEXPR
//quantity<Unit,long long> 
//llrint BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
//{
//    using namespace detail;
//
//    typedef quantity<Unit,long long>    quantity_type;
//    
//    return quantity_type::from_value(llrint BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
//}

// does ISO C++ support long long? g++ claims not
//template<class Unit,class Y>
//inline 
//BOOST_CONSTEXPR
//quantity<Unit,long long> 
//llround BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
//{
//    using namespace detail;
//
//    typedef quantity<Unit,long long>    quantity_type;
//    
//    return quantity_type::from_value(llround BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
//}

#if 0

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
nearbyint BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(nearbyint BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

#endif

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> nextafter BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                                             const quantity<Unit,Y>& q2)
{
    using boost::math::nextafter;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(nextafter BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}
template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q1,
                                                              const quantity<Unit,Y>& q2)
{
    // the only difference between nextafter and nexttowards is
    // in the argument types.  Since we are requiring identical
    // argument types, there is no difference.
    using boost::math::nextafter;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(nextafter BOOST_PREVENT_MACRO_SUBSTITUTION (q1.value(),q2.value()));
}

#if 0

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
rint BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(rint BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

#endif

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
round BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using boost::math::round;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(round BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
int 
signbit BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{ 
    using boost::math::signbit;

    return signbit BOOST_PREVENT_MACRO_SUBSTITUTION (q.value());
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
quantity<Unit,Y> 
trunc BOOST_PREVENT_MACRO_SUBSTITUTION (const quantity<Unit,Y>& q)
{
    using namespace detail;

    typedef quantity<Unit,Y>    quantity_type;
    
    return quantity_type::from_value(trunc BOOST_PREVENT_MACRO_SUBSTITUTION (q.value()));
}

template<class Unit,class Y>
inline
BOOST_CONSTEXPR
quantity<Unit, Y>
fmod(const quantity<Unit,Y>& q1, const quantity<Unit,Y>& q2)
{
    using std::fmod;
    
    typedef quantity<Unit,Y> quantity_type;

    return quantity_type::from_value(fmod(q1.value(), q2.value()));
}

template<class Unit, class Y>
inline
BOOST_CONSTEXPR
quantity<Unit, Y>
modf(const quantity<Unit, Y>& q1, quantity<Unit, Y>* q2)
{
    using std::modf;

    typedef quantity<Unit,Y> quantity_type;

    return quantity_type::from_value(modf(q1.value(), &quantity_cast<Y&>(*q2)));
}

template<class Unit, class Y, class Int>
inline
BOOST_CONSTEXPR
quantity<Unit, Y>
frexp(const quantity<Unit, Y>& q,Int* ex)
{
    using std::frexp;

    typedef quantity<Unit,Y> quantity_type;

    return quantity_type::from_value(frexp(q.value(),ex));
}

/// For non-dimensionless quantities, integral and rational powers 
/// and roots can be computed by @c pow<Ex> and @c root<Rt> respectively.
template<class S, class Y>
inline
BOOST_CONSTEXPR
quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>
pow(const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>& q1,
    const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>& q2)
{
    using std::pow;

    typedef quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S),Y> quantity_type;

    return quantity_type::from_value(pow(q1.value(), q2.value()));
}

template<class S, class Y>
inline
BOOST_CONSTEXPR
quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>
exp(const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>& q)
{
    using std::exp;

    typedef quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y> quantity_type;

    return quantity_type::from_value(exp(q.value()));
}

template<class Unit, class Y, class Int>
inline
BOOST_CONSTEXPR
quantity<Unit, Y>
ldexp(const quantity<Unit, Y>& q,const Int& ex)
{
    using std::ldexp;

    typedef quantity<Unit,Y> quantity_type;

    return quantity_type::from_value(ldexp(q.value(), ex));
}

template<class S, class Y>
inline
BOOST_CONSTEXPR
quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>
log(const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>& q)
{
    using std::log;

    typedef quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y> quantity_type;

    return quantity_type::from_value(log(q.value()));
}

template<class S, class Y>
inline
BOOST_CONSTEXPR
quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>
log10(const quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y>& q)
{
    using std::log10;

    typedef quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(S), Y> quantity_type;

    return quantity_type::from_value(log10(q.value()));
}

template<class Unit,class Y>
inline 
BOOST_CONSTEXPR
typename root_typeof_helper<
                            quantity<Unit,Y>,
                            static_rational<2>
                           >::type
sqrt(const quantity<Unit,Y>& q)
{
    using std::sqrt;

    typedef typename root_typeof_helper<
                                        quantity<Unit,Y>,
                                        static_rational<2>
                                       >::type quantity_type;

    return quantity_type::from_value(sqrt(q.value()));
}

} // namespace units

} // namespace boost

namespace boost {

namespace units {

// trig functions with si argument/return types

/// cos of theta in radians
template<class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<si::system,Y>::type 
cos(const quantity<si::plane_angle,Y>& theta)
{
    using std::cos;
    return cos(theta.value());
}

/// sin of theta in radians
template<class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<si::system,Y>::type 
sin(const quantity<si::plane_angle,Y>& theta)
{
    using std::sin;
    return sin(theta.value());
}

/// tan of theta in radians
template<class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<si::system,Y>::type 
tan(const quantity<si::plane_angle,Y>& theta)
{
    using std::tan;
    return tan(theta.value());
}

/// cos of theta in other angular units 
template<class System,class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<System,Y>::type 
cos(const quantity<unit<plane_angle_dimension,System>,Y>& theta)
{
    return cos(quantity<si::plane_angle,Y>(theta));
}

/// sin of theta in other angular units 
template<class System,class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<System,Y>::type 
sin(const quantity<unit<plane_angle_dimension,System>,Y>& theta)
{
    return sin(quantity<si::plane_angle,Y>(theta));
}

/// tan of theta in other angular units 
template<class System,class Y>
BOOST_CONSTEXPR
typename dimensionless_quantity<System,Y>::type 
tan(const quantity<unit<plane_angle_dimension,System>,Y>& theta)
{
    return tan(quantity<si::plane_angle,Y>(theta));
}

/// acos of dimensionless quantity returning angle in same system
template<class Y,class System>
BOOST_CONSTEXPR
quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>
acos(const quantity<unit<dimensionless_type, homogeneous_system<System> >,Y>& val)
{
    using std::acos;
    return quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>(acos(val.value())*si::radians);
}

/// acos of dimensionless quantity returning angle in radians
template<class Y>
BOOST_CONSTEXPR
quantity<angle::radian_base_unit::unit_type,Y>
acos(const quantity<unit<dimensionless_type, heterogeneous_dimensionless_system>,Y>& val)
{
    using std::acos;
    return quantity<angle::radian_base_unit::unit_type,Y>::from_value(acos(val.value()));
}

/// asin of dimensionless quantity returning angle in same system
template<class Y,class System>
BOOST_CONSTEXPR
quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>
asin(const quantity<unit<dimensionless_type, homogeneous_system<System> >,Y>& val)
{
    using std::asin;
    return quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>(asin(val.value())*si::radians);
}

/// asin of dimensionless quantity returning angle in radians
template<class Y>
BOOST_CONSTEXPR
quantity<angle::radian_base_unit::unit_type,Y>
asin(const quantity<unit<dimensionless_type, heterogeneous_dimensionless_system>,Y>& val)
{
    using std::asin;
    return quantity<angle::radian_base_unit::unit_type,Y>::from_value(asin(val.value()));
}

/// atan of dimensionless quantity returning angle in same system
template<class Y,class System>
BOOST_CONSTEXPR
quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>
atan(const quantity<unit<dimensionless_type, homogeneous_system<System> >, Y>& val)
{
    using std::atan;
    return quantity<unit<plane_angle_dimension, homogeneous_system<System> >,Y>(atan(val.value())*si::radians);
}

/// atan of dimensionless quantity returning angle in radians
template<class Y>
BOOST_CONSTEXPR
quantity<angle::radian_base_unit::unit_type,Y>
atan(const quantity<unit<dimensionless_type, heterogeneous_dimensionless_system>, Y>& val)
{
    using std::atan;
    return quantity<angle::radian_base_unit::unit_type,Y>::from_value(atan(val.value()));
}

/// atan2 of @c value_type returning angle in radians
template<class Y, class Dimension, class System>
BOOST_CONSTEXPR
quantity<unit<plane_angle_dimension, homogeneous_system<System> >, Y>
atan2(const quantity<unit<Dimension, homogeneous_system<System> >, Y>& y,
      const quantity<unit<Dimension, homogeneous_system<System> >, Y>& x)
{
    using std::atan2;
    return quantity<unit<plane_angle_dimension, homogeneous_system<System> >, Y>(atan2(y.value(),x.value())*si::radians);
}

/// atan2 of @c value_type returning angle in radians
template<class Y, class Dimension, class System>
BOOST_CONSTEXPR
quantity<angle::radian_base_unit::unit_type,Y>
atan2(const quantity<unit<Dimension, heterogeneous_system<System> >, Y>& y,
      const quantity<unit<Dimension, heterogeneous_system<System> >, Y>& x)
{
    using std::atan2;
    return quantity<angle::radian_base_unit::unit_type,Y>::from_value(atan2(y.value(),x.value()));
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CMATH_HPP
