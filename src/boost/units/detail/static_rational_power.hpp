// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_STATIC_RATIONAL_POWER_HPP
#define BOOST_UNITS_DETAIL_STATIC_RATIONAL_POWER_HPP

#include <boost/config/no_tr1/cmath.hpp>

#include <boost/units/detail/one.hpp>
#include <boost/units/operators.hpp>

namespace boost {

namespace units {

template<long N,long D>
class static_rational;

namespace detail {

namespace typeof_pow_adl_barrier {

using std::pow;

template<class Y>
struct typeof_pow
{
#if defined(BOOST_UNITS_HAS_BOOST_TYPEOF)
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, pow(typeof_::make<Y>(), 0.0))
    typedef typename nested::type type;
#elif defined(BOOST_UNITS_HAS_MWERKS_TYPEOF)
    typedef __typeof__(pow(typeof_::make<Y>(), 0.0)) type;
#elif defined(BOOST_UNITS_HAS_GNU_TYPEOF)
    typedef typeof(pow(typeof_::make<Y>(), 0.0)) type;
#else
    typedef Y type;
#endif
};

}

template<class R, class Y>
struct static_rational_power_impl
{
    typedef typename typeof_pow_adl_barrier::typeof_pow<Y>::type type;
    static BOOST_CONSTEXPR type call(const Y& y)
    {
        using std::pow;
        return(pow(y, static_cast<double>(R::Numerator) / static_cast<double>(R::Denominator)));
    }
};

template<class R>
struct static_rational_power_impl<R, one>
{
    typedef one type;
    static BOOST_CONSTEXPR one call(const one&)
    {
        return(one());
    }
};

template<long N>
struct static_rational_power_impl<static_rational<N, 1>, one>
{
    typedef one type;
    static BOOST_CONSTEXPR one call(const one&)
    {
        return(one());
    }
};

template<long N, bool = (N % 2 == 0)>
struct static_int_power_impl;

template<long N>
struct static_int_power_impl<N, true>
{
    template<class Y, class R>
    struct apply
    {
        typedef typename multiply_typeof_helper<Y, Y>::type square_type;
        typedef typename static_int_power_impl<(N >> 1)>::template apply<square_type, R> next;
        typedef typename next::type type;
        static BOOST_CONSTEXPR type call(const Y& y, const R& r)
        {
            return(next::call(static_cast<square_type>(y * y), r));
        }
    };
};

template<long N>
struct static_int_power_impl<N, false>
{
    template<class Y, class R>
    struct apply
    {
        typedef typename multiply_typeof_helper<Y, Y>::type square_type;
        typedef typename multiply_typeof_helper<Y, R>::type new_r;
        typedef typename static_int_power_impl<(N >> 1)>::template apply<square_type, new_r> next;
        typedef typename next::type type;
        static BOOST_CONSTEXPR type call(const Y& y, const R& r)
        {
            return(next::call(static_cast<Y>(y * y), y * r));
        }
    };
};

template<>
struct static_int_power_impl<1, false>
{
    template<class Y, class R>
    struct apply
    {
        typedef typename multiply_typeof_helper<Y, R>::type type;
        static BOOST_CONSTEXPR type call(const Y& y, const R& r)
        {
            return(y * r);
        }
    };
};

template<>
struct static_int_power_impl<0, true>
{
    template<class Y, class R>
    struct apply
    {
        typedef R type;
        static BOOST_CONSTEXPR R call(const Y&, const R& r)
        {
            return(r);
        }
    };
};

template<int N, bool = (N < 0)>
struct static_int_power_sign_impl;

template<int N>
struct static_int_power_sign_impl<N, false>
{
    template<class Y>
    struct apply
    {
        typedef typename static_int_power_impl<N>::template apply<Y, one> impl;
        typedef typename impl::type type;
        static BOOST_CONSTEXPR type call(const Y& y)
        {
            return(impl::call(y, one()));
        }
    };
};

template<int N>
struct static_int_power_sign_impl<N, true>
{
    template<class Y>
    struct apply
    {
        typedef typename static_int_power_impl<-N>::template apply<Y, one> impl;
        typedef typename divide_typeof_helper<one, typename impl::type>::type type;
        static BOOST_CONSTEXPR type call(const Y& y)
        {
            return(one()/impl::call(y, one()));
        }
    };
};

template<long N, class Y>
struct static_rational_power_impl<static_rational<N, 1>, Y>
{
    typedef typename static_int_power_sign_impl<N>::template apply<Y> impl;
    typedef typename impl::type type;
    static BOOST_CONSTEXPR type call(const Y& y)
    {
        return(impl::call(y));
    }
};

template<class R, class Y>
BOOST_CONSTEXPR
typename detail::static_rational_power_impl<R, Y>::type static_rational_power(const Y& y)
{
    return(detail::static_rational_power_impl<R, Y>::call(y));
}

} // namespace detail

} // namespace units

} // namespace boost

#endif
