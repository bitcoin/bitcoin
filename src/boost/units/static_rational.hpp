// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_STATIC_RATIONAL_HPP 
#define BOOST_UNITS_STATIC_RATIONAL_HPP

#include <boost/integer/common_factor_ct.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/arithmetic.hpp>

#ifdef BOOST_BORLANDC
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/identity.hpp>
#endif

#include <boost/units/config.hpp>
#include <boost/units/operators.hpp>

/// \file 
/// \brief Compile-time rational numbers and operators.

namespace boost {

namespace units { 

namespace detail {

struct static_rational_tag {};

}

typedef long   integer_type;

/// Compile time absolute value.
template<integer_type Value>
struct static_abs
{
    BOOST_STATIC_CONSTANT(integer_type,value = Value < 0 ? -Value : Value);
};

// Compile time rational number.
/** 
This is an implementation of a compile time rational number, where @c static_rational<N,D> represents
a rational number with numerator @c N and denominator @c D. Because of the potential for ambiguity arising 
from multiple equivalent values of @c static_rational (e.g. @c static_rational<6,2>==static_rational<3>), 
static rationals should always be accessed through @c static_rational<N,D>::type. Template specialization 
prevents instantiation of zero denominators (i.e. @c static_rational<N,0>). The following compile-time 
arithmetic operators are provided for static_rational variables only (no operators are defined between 
long and static_rational):
    - @c mpl::negate
    - @c mpl::plus
    - @c mpl::minus
    - @c mpl::times
    - @c mpl::divides

Neither @c static_power nor @c static_root are defined for @c static_rational. This is because template types 
may not be floating point values, while powers and roots of rational numbers can produce floating point 
values. 
*/
#ifdef BOOST_BORLANDC

template<integer_type X>
struct make_integral_c {
    typedef boost::mpl::integral_c<integer_type, X> type;
};

template<integer_type N,integer_type D = 1>
class static_rational
{
    public:

        typedef static_rational this_type;

        typedef boost::mpl::integral_c<integer_type, N> N_type;
        typedef boost::mpl::integral_c<integer_type, D> D_type;

        typedef typename make_integral_c<
            (::boost::integer::static_gcd<
                ::boost::units::static_abs<N>::value,
                ::boost::units::static_abs<D>::value
            >::value)>::type gcd_type;
        typedef typename boost::mpl::eval_if<
            boost::mpl::less<
                D_type,
                boost::mpl::integral_c<integer_type, 0>
            >,
            boost::mpl::negate<gcd_type>,
            gcd_type
        >::type den_type;
        
    public: 
        // for mpl arithmetic support
        typedef detail::static_rational_tag tag;
        
        BOOST_STATIC_CONSTANT(integer_type, Numerator =
            (::boost::mpl::divides<N_type, den_type>::value));
        BOOST_STATIC_CONSTANT(integer_type, Denominator =
            (::boost::mpl::divides<D_type, den_type>::value));
        
        /// INTERNAL ONLY
        typedef static_rational<N,D>    this_type;
        
        /// static_rational<N,D> reduced by GCD
        typedef static_rational<
            (::boost::mpl::divides<N_type, den_type>::value),
            (::boost::mpl::divides<D_type, den_type>::value)
        >  type;
                                 
        static BOOST_CONSTEXPR integer_type numerator()     { return Numerator; }
        static BOOST_CONSTEXPR integer_type denominator()   { return Denominator; }
        
        // INTERNAL ONLY
        BOOST_CONSTEXPR static_rational() { }
        //~static_rational() { }
};
#else
template<integer_type N,integer_type D = 1>
class static_rational
{
    private:

        BOOST_STATIC_CONSTEXPR integer_type nabs = static_abs<N>::value,
                                            dabs = static_abs<D>::value;
        
        /// greatest common divisor of N and D
        // need cast to signed because static_gcd returns unsigned long
        BOOST_STATIC_CONSTEXPR integer_type den = 
            static_cast<integer_type>(boost::integer::static_gcd<nabs,dabs>::value) * ((D < 0) ? -1 : 1);
        
    public: 
        // for mpl arithmetic support
        typedef detail::static_rational_tag tag;
        
        BOOST_STATIC_CONSTEXPR integer_type Numerator = N/den,
            Denominator = D/den;
        
        /// INTERNAL ONLY
        typedef static_rational<N,D>    this_type;
        
        /// static_rational<N,D> reduced by GCD
        typedef static_rational<Numerator,Denominator>  type;
                                 
        static BOOST_CONSTEXPR integer_type numerator()     { return Numerator; }
        static BOOST_CONSTEXPR integer_type denominator()   { return Denominator; }
        
        // INTERNAL ONLY
        BOOST_CONSTEXPR static_rational() { }
        //~static_rational() { }   
};
#endif

}

}

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::static_rational, (long)(long))

#endif

namespace boost {

namespace units {

// prohibit zero denominator
template<integer_type N> class static_rational<N,0>;

/// get decimal value of @c static_rational
template<class T,integer_type N,integer_type D>
inline BOOST_CONSTEXPR typename divide_typeof_helper<T,T>::type 
value(const static_rational<N,D>&)
{
    return T(N)/T(D);
}

} // namespace units

#ifndef BOOST_UNITS_DOXYGEN

namespace mpl {

#ifdef BOOST_BORLANDC

template<>
struct plus_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            ::boost::mpl::plus<
                boost::mpl::times<typename T0::N_type, typename T1::D_type>,
                boost::mpl::times<typename T1::N_type, typename T0::D_type>
            >::value,
            ::boost::mpl::times<typename T0::D_type, typename T1::D_type>::value
        >::type type;
    };
};

template<>
struct minus_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            ::boost::mpl::minus<
                boost::mpl::times<typename T0::N_type, typename T1::D_type>,
                boost::mpl::times<typename T1::N_type, typename T0::D_type>
            >::value,
            ::boost::mpl::times<typename T0::D_type, typename T1::D_type>::value
        >::type type;
    };
};

template<>
struct times_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            ::boost::mpl::times<typename T0::N_type, typename T1::N_type>::value,
            ::boost::mpl::times<typename T0::D_type, typename T1::D_type>::value
        >::type type;
    };
};

template<>
struct divides_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            ::boost::mpl::times<typename T0::N_type, typename T1::D_type>::value,
            ::boost::mpl::times<typename T0::D_type, typename T1::N_type>::value
        >::type type;
    };
};

template<>
struct negate_impl<boost::units::detail::static_rational_tag>
{
    template<class T0>
    struct apply {
        typedef typename boost::units::static_rational<
            ::boost::mpl::negate<typename T0::N_type>::value,
            ::boost::mpl::identity<T0>::type::Denominator
        >::type type;
    };
};

template<>
struct less_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef mpl::bool_<((mpl::minus<T0, T1>::type::Numerator) < 0)> type;
    };
};

#else

template<>
struct plus_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            T0::Numerator*T1::Denominator+T1::Numerator*T0::Denominator,
            T0::Denominator*T1::Denominator
        >::type type;
    };
};

template<>
struct minus_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            T0::Numerator*T1::Denominator-T1::Numerator*T0::Denominator,
            T0::Denominator*T1::Denominator
        >::type type;
    };
};

template<>
struct times_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            T0::Numerator*T1::Numerator,
            T0::Denominator*T1::Denominator
        >::type type;
    };
};

template<>
struct divides_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply {
        typedef typename boost::units::static_rational<
            T0::Numerator*T1::Denominator,
            T0::Denominator*T1::Numerator
        >::type type;
    };
};

template<>
struct negate_impl<boost::units::detail::static_rational_tag>
{
    template<class T0>
    struct apply {
        typedef typename boost::units::static_rational<-T0::Numerator,T0::Denominator>::type type;
    };
};

template<>
struct less_impl<boost::units::detail::static_rational_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef mpl::bool_<((mpl::minus<T0, T1>::type::Numerator) < 0)> type;
    };
};

#endif


}

#endif

} // namespace boost

#endif // BOOST_UNITS_STATIC_RATIONAL_HPP
