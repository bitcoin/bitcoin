// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_POW_HPP 
#define BOOST_UNITS_POW_HPP

#include <boost/type_traits/is_integral.hpp>

#include <boost/units/operators.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/detail/static_rational_power.hpp>

/// \file 
/// \brief Raise values to exponents known at compile-time.

namespace boost {

namespace units {

/// raise a value to a @c static_rational power.
template<class Rat,class Y>
BOOST_CONSTEXPR
inline typename power_typeof_helper<Y,Rat>::type
pow(const Y& x)
{
    return power_typeof_helper<Y,Rat>::value(x);
}

/// raise a value to an integer power.
template<long N,class Y>
BOOST_CONSTEXPR
inline typename power_typeof_helper<Y,static_rational<N> >::type
pow(const Y& x)
{
    return power_typeof_helper<Y,static_rational<N> >::value(x);
}

#ifndef BOOST_UNITS_DOXYGEN

/// raise @c T to a @c static_rational power.
template<class T, long N,long D> 
struct power_typeof_helper<T, static_rational<N,D> >                
{ 
    typedef typename mpl::if_<boost::is_integral<T>, double, T>::type internal_type;
    typedef detail::static_rational_power_impl<static_rational<N, D>, internal_type> impl;
    typedef typename impl::type type; 
    
    static BOOST_CONSTEXPR type value(const T& x)  
    {
        return impl::call(x);
    }
};

/// raise @c float to a @c static_rational power.
template<long N,long D> 
struct power_typeof_helper<float, static_rational<N,D> >
{
    // N.B.  pathscale doesn't accept inheritance for some reason.
    typedef power_typeof_helper<double, static_rational<N,D> > base;
    typedef typename base::type type;
    static BOOST_CONSTEXPR type value(const double& x)
    {
        return base::value(x);
    }
};

#endif

/// take the @c static_rational root of a value.
template<class Rat,class Y>
BOOST_CONSTEXPR
typename root_typeof_helper<Y,Rat>::type
root(const Y& x)
{
    return root_typeof_helper<Y,Rat>::value(x);
}

/// take the integer root of a value.
template<long N,class Y>
BOOST_CONSTEXPR
typename root_typeof_helper<Y,static_rational<N> >::type
root(const Y& x)
{
    return root_typeof_helper<Y,static_rational<N> >::value(x);
}

#ifndef BOOST_UNITS_DOXYGEN

/// take @c static_rational root of an @c T
template<class T, long N,long D> 
struct root_typeof_helper<T,static_rational<N,D> >
{
    // N.B.  pathscale doesn't accept inheritance for some reason.
    typedef power_typeof_helper<T, static_rational<D,N> > base;
    typedef typename base::type type;
    static BOOST_CONSTEXPR type value(const T& x)
    {
        return(base::value(x));
    }
};

#endif

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_STATIC_RATIONAL_HPP
