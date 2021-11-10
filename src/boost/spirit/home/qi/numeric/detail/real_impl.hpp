/*=============================================================================
    Copyright (c) 2001-2019 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_NUMERIC_DETAIL_REAL_IMPL_HPP
#define BOOST_SPIRIT_QI_NUMERIC_DETAIL_REAL_IMPL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <cmath>
#include <boost/limits.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/detail/pow10.hpp>
#include <boost/integer.hpp>
#include <boost/assert.hpp>

#include <boost/core/cmath.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(push)
# pragma warning(disable: 4100)   // 'p': unreferenced formal parameter
# pragma warning(disable: 4127)   // conditional expression is constant
#endif

namespace boost { namespace spirit { namespace traits
{
    using spirit::traits::pow10;

    namespace detail
    {
        template <typename T, typename AccT>
        void compensate_roundoff(T& n, AccT acc_n, mpl::true_)
        {
            // at the lowest extremes, we compensate for floating point
            // roundoff errors by doing imprecise computation using T
            int const comp = 10;
            n = T((acc_n / comp) * comp);
            n += T(acc_n % comp);
        }

        template <typename T, typename AccT>
        void compensate_roundoff(T& n, AccT acc_n, mpl::false_)
        {
            // no need to compensate
            n = acc_n;
        }

        template <typename T, typename AccT>
        void compensate_roundoff(T& n, AccT acc_n)
        {
            compensate_roundoff(n, acc_n, is_integral<AccT>());
        }
    }

    template <typename T, typename AccT>
    inline bool
    scale(int exp, T& n, AccT acc_n)
    {
        if (exp >= 0)
        {
            int const max_exp = std::numeric_limits<T>::max_exponent10;

            // return false if exp exceeds the max_exp
            // do this check only for primitive types!
            if (is_floating_point<T>() && (exp > max_exp))
                return false;
            n = acc_n * pow10<T>(exp);
        }
        else
        {
            if (exp < std::numeric_limits<T>::min_exponent10)
            {
                int const min_exp = std::numeric_limits<T>::min_exponent10;
                detail::compensate_roundoff(n, acc_n);
                n /= pow10<T>(-min_exp);

                // return false if exp still exceeds the min_exp
                // do this check only for primitive types!
                exp += -min_exp;
                if (is_floating_point<T>() && exp < min_exp)
                    return false;

                n /= pow10<T>(-exp);
            }
            else
            {
                n = T(acc_n) / pow10<T>(-exp);
            }
        }
        return true;
    }

    inline bool
    scale(int /*exp*/, unused_type /*n*/, unused_type /*acc_n*/)
    {
        // no-op for unused_type
        return true;
    }

    template <typename T, typename AccT>
    inline bool
    scale(int exp, int frac, T& n, AccT acc_n)
    {
        return scale(exp - frac, n, acc_n);
    }

    inline bool
    scale(int /*exp*/, int /*frac*/, unused_type /*n*/)
    {
        // no-op for unused_type
        return true;
    }

    inline float
    negate(bool neg, float n)
    {
        return neg ? (core::copysign)(n, -1.f) : n;
    }

    inline double
    negate(bool neg, double n)
    {
        return neg ? (core::copysign)(n, -1.) : n;
    }

    inline long double
    negate(bool neg, long double n)
    {
        return neg ? (core::copysign)(n, static_cast<long double>(-1)) : n;
    }

    template <typename T>
    inline T
    negate(bool neg, T const& n)
    {
        return neg ? -n : n;
    }

    inline unused_type
    negate(bool /*neg*/, unused_type n)
    {
        // no-op for unused_type
        return n;
    }

    template <typename T>
    struct real_accumulator : mpl::identity<T> {};

    template <>
    struct real_accumulator<float>
        : mpl::identity<uint_t<(sizeof(float)*CHAR_BIT)>::least> {};

    template <>
    struct real_accumulator<double>
        : mpl::identity<uint_t<(sizeof(double)*CHAR_BIT)>::least> {};
}}}

namespace boost { namespace spirit { namespace qi  { namespace detail
{
    BOOST_MPL_HAS_XXX_TRAIT_DEF(version)

    template <typename T, typename RealPolicies>
    struct real_impl
    {
        template <typename Iterator>
        static std::size_t
        ignore_excess_digits(Iterator& /* first */, Iterator const& /* last */, mpl::false_)
        {
            return 0;
        }

        template <typename Iterator>
        static std::size_t
        ignore_excess_digits(Iterator& first, Iterator const& last, mpl::true_)
        {
            return RealPolicies::ignore_excess_digits(first, last);
        }

        template <typename Iterator>
        static std::size_t
        ignore_excess_digits(Iterator& first, Iterator const& last)
        {
            typedef mpl::bool_<has_version<RealPolicies>::value> has_version;
            return ignore_excess_digits(first, last, has_version());
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse(Iterator& first, Iterator const& last, Attribute& attr,
            RealPolicies const& p)
        {
            if (first == last)
                return false;
            Iterator save = first;

            // Start by parsing the sign. neg will be true if
            // we got a "-" sign, false otherwise.
            bool neg = p.parse_sign(first, last);

            // Now attempt to parse an integer
            T n;

            typename traits::real_accumulator<T>::type acc_n = 0;
            bool got_a_number = p.parse_n(first, last, acc_n);
            int excess_n = 0;

            // If we did not get a number it might be a NaN, Inf or a leading
            // dot.
            if (!got_a_number)
            {
                // Check whether the number to parse is a NaN or Inf
                if (p.parse_nan(first, last, n) ||
                    p.parse_inf(first, last, n))
                {
                    // If we got a negative sign, negate the number
                    traits::assign_to(traits::negate(neg, n), attr);
                    return true;    // got a NaN or Inf, return early
                }

                // If we did not get a number and our policies do not
                // allow a leading dot, fail and return early (no-match)
                if (!p.allow_leading_dot)
                {
                    first = save;
                    return false;
                }
            }
            else
            {
                // We got a number and we still see digits. This happens if acc_n (an integer)
                // exceeds the integer's capacity. Collect the excess digits.
                excess_n = static_cast<int>(ignore_excess_digits(first, last));
            }

            bool e_hit = false;
            Iterator e_pos;
            int frac_digits = 0;

            // Try to parse the dot ('.' decimal point)
            if (p.parse_dot(first, last))
            {
                // We got the decimal point. Now we will try to parse
                // the fraction if it is there. If not, it defaults
                // to zero (0) only if we already got a number.
                if (excess_n != 0)
                {
                    // We skip the fractions if we already exceeded our digits capacity
                    ignore_excess_digits(first, last);
                }
                else if (p.parse_frac_n(first, last, acc_n, frac_digits))
                {
                    BOOST_ASSERT(frac_digits >= 0);
                }
                else if (!got_a_number || !p.allow_trailing_dot)
                {
                    // We did not get a fraction. If we still haven't got a
                    // number and our policies do not allow a trailing dot,
                    // return no-match.
                    first = save;
                    return false;
                }

                // Now, let's see if we can parse the exponent prefix
                e_pos = first;
                e_hit = p.parse_exp(first, last);
            }
            else
            {
                // No dot and no number! Return no-match.
                if (!got_a_number)
                {
                    first = save;
                    return false;
                }

                // If we must expect a dot and we didn't see an exponent
                // prefix, return no-match.
                e_pos = first;
                e_hit = p.parse_exp(first, last);
                if (p.expect_dot && !e_hit)
                {
                    first = save;
                    return false;
                }
            }

            if (e_hit)
            {
                // We got the exponent prefix. Now we will try to parse the
                // actual exponent.
                int exp = 0;
                if (p.parse_exp_n(first, last, exp))
                {
                    // Got the exponent value. Scale the number by
                    // exp + excess_n - frac_digits.
                    if (!traits::scale(exp + excess_n, frac_digits, n, acc_n))
                        return false;
                }
                else
                {
                    // If there is no number, disregard the exponent altogether.
                    // by resetting 'first' prior to the exponent prefix (e|E)
                    first = e_pos;
                    // Scale the number by -frac_digits.
                    bool r = traits::scale(-frac_digits, n, acc_n);
                    BOOST_VERIFY(r);
                }
            }
            else if (frac_digits)
            {
                // No exponent found. Scale the number by -frac_digits.
                bool r = traits::scale(-frac_digits, n, acc_n);
                BOOST_VERIFY(r);
            }
            else
            {
                if (excess_n)
                {
                    if (!traits::scale(excess_n, n, acc_n))
                        return false;
                }
                else
                {
                    n = static_cast<T>(acc_n);
                }
            }

            // If we got a negative sign, negate the number
            traits::assign_to(traits::negate(neg, n), attr);

            // Success!!!
            return true;
        }
    };

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif

}}}}

#endif
