/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_IS_NUMERIC_HPP_JOFA_100322
#define BOOST_ICL_TYPE_TRAITS_IS_NUMERIC_HPP_JOFA_100322

#include <limits>
#include <complex>
#include <functional>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace boost{ namespace icl
{

template <class Type> struct is_fixed_numeric
{
    typedef is_fixed_numeric type;
    BOOST_STATIC_CONSTANT(bool, value = (0 < std::numeric_limits<Type>::digits));
};

template <class Type> struct is_std_numeric
{
    typedef is_std_numeric type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (std::numeric_limits<Type>::is_specialized));
};

template <class Type> struct is_std_integral
{
    typedef is_std_integral type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (std::numeric_limits<Type>::is_integer));
};

template <class Type> struct is_numeric
{
    typedef is_numeric type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_< is_std_numeric<Type>
                 , boost::is_integral<Type> 
                 , is_std_integral<Type> >::value) );
};

template <class Type> 
struct is_numeric<std::complex<Type> >
{
    typedef is_numeric type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

//--------------------------------------------------------------------------
template<class Type, class Compare, bool Enable = false>
struct numeric_minimum
{
    static bool is_less_than(Type){ return true; }
    static bool is_less_than_or(Type, bool){ return true; }
};

template<class Type> 
struct numeric_minimum<Type, std::less<Type>, true>
{
    static bool is_less_than(Type value)
    { return std::less<Type>()((std::numeric_limits<Type>::min)(), value); }

    static bool is_less_than_or(Type value, bool cond)
    { return cond || is_less_than(value); }
};

template<class Type> 
struct numeric_minimum<Type, std::greater<Type>, true>
{
    static bool is_less_than(Type value)
    { return std::greater<Type>()((std::numeric_limits<Type>::max)(), value); }

    static bool is_less_than_or(Type value, bool cond)
    { return cond || is_less_than(value); }
};

//--------------------------------------------------------------------------
template<class Type> 
struct is_non_floating_point
{
    typedef is_non_floating_point type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::not_< is_floating_point<Type> >::value));
};

}} // namespace boost icl

#endif


