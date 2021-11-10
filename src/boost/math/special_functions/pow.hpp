//   Boost pow.hpp header file
//   Computes a power with exponent known at compile-time

//  (C) Copyright Bruno Lalande 2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.


#ifndef BOOST_MATH_POW_HPP
#define BOOST_MATH_POW_HPP


#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/policies/policy.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/tools/promotion.hpp>


namespace boost {
namespace math {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702) // Unreachable code, only triggered in release mode and /W4
#endif

namespace detail {


template <int N, int M = N%2>
struct positive_power
{
    template <typename T>
    static BOOST_CXX14_CONSTEXPR T result(T base)
    {
        T power = positive_power<N/2>::result(base);
        return power * power;
    }
};

template <int N>
struct positive_power<N, 1>
{
    template <typename T>
    static BOOST_CXX14_CONSTEXPR T result(T base)
    {
        T power = positive_power<N/2>::result(base);
        return base * power * power;
    }
};

template <>
struct positive_power<1, 1>
{
    template <typename T>
    static BOOST_CXX14_CONSTEXPR T result(T base){ return base; }
};


template <int N, bool>
struct power_if_positive
{
    template <typename T, class Policy>
    static BOOST_CXX14_CONSTEXPR T result(T base, const Policy&)
    { return positive_power<N>::result(base); }
};

template <int N>
struct power_if_positive<N, false>
{
    template <typename T, class Policy>
    static BOOST_CXX14_CONSTEXPR T result(T base, const Policy& policy)
    {
        if (base == 0)
        {
            return policies::raise_overflow_error<T>(
                       "boost::math::pow(%1%)",
                       "Attempted to compute a negative power of 0",
                       policy
                   );
        }

        return T(1) / positive_power<-N>::result(base);
    }
};

template <>
struct power_if_positive<0, true>
{
    template <typename T, class Policy>
    static BOOST_CXX14_CONSTEXPR T result(T base, const Policy& policy)
    {
        if (base == 0)
        {
            return policies::raise_indeterminate_result_error<T>(
                       "boost::math::pow(%1%)",
                       "The result of pow<0>(%1%) is undetermined",
                       base,
                       T(1),
                       policy
                   );
        }

        return T(1);
    }
};


template <int N>
struct select_power_if_positive
{
    using type = power_if_positive<N, (N >= 0)>;
};


}  // namespace detail


template <int N, typename T, class Policy>
BOOST_CXX14_CONSTEXPR inline typename tools::promote_args<T>::type pow(T base, const Policy& policy)
{ 
   using result_type = typename tools::promote_args<T>::type;
   return detail::select_power_if_positive<N>::type::result(static_cast<result_type>(base), policy); 
}

template <int N, typename T>
BOOST_CXX14_CONSTEXPR inline typename tools::promote_args<T>::type pow(T base)
{ return pow<N>(base, policies::policy<>()); }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace math
}  // namespace boost


#endif
