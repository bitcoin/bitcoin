//  Copyright (c) 2001-2020 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_REAL_UTILS_FEB_23_2007_0841PM)
#define BOOST_SPIRIT_KARMA_REAL_UTILS_FEB_23_2007_0841PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/limits.hpp>

#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/detail/pow10.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/string_generate.hpp>
#include <boost/spirit/home/karma/numeric/detail/numeric_utils.hpp>

namespace boost { namespace spirit { namespace karma 
{ 
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The real_inserter template takes care of the floating point number to 
    //  string conversion. The Policies template parameter is used to allow
    //  customization of the formatting process
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct real_policies;

    template <typename T
      , typename Policies = real_policies<T>
      , typename CharEncoding = unused_type
      , typename Tag = unused_type>
    struct real_inserter
    {
        template <typename OutputIterator, typename U>
        static bool
        call (OutputIterator& sink, U n, Policies const& p = Policies())
        {
            if (traits::test_nan(n)) {
                return p.template nan<CharEncoding, Tag>(
                    sink, n, p.force_sign(n));
            }
            else if (traits::test_infinite(n)) {
                return p.template inf<CharEncoding, Tag>(
                    sink, n, p.force_sign(n));
            }
            return p.template call<real_inserter>(sink, n, p);
        }

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)  
# pragma warning(push)
# pragma warning(disable: 4100)   // 'p': unreferenced formal parameter  
# pragma warning(disable: 4127)   // conditional expression is constant
# pragma warning(disable: 4267)   // conversion from 'size_t' to 'unsigned int', possible loss of data
#endif 
        ///////////////////////////////////////////////////////////////////////
        //  This is the workhorse behind the real generator
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator, typename U>
        static bool
        call_n (OutputIterator& sink, U n, Policies const& p)
        {
        // prepare sign and get output format
            bool force_sign = p.force_sign(n);
            bool sign_val = false;
            int flags = p.floatfield(n);
            if (traits::test_negative(n)) 
            {
                n = -n;
                sign_val = true;
            }

        // The scientific representation requires the normalization of the 
        // value to convert.

            // get correct precision for generated number
            unsigned precision = p.precision(n);

            // allow for ADL to find the correct overloads for log10 et.al.
            using namespace std;

            bool precexp_offset = false;
            U dim = 0;
            if (0 == (Policies::fmtflags::fixed & flags) && !traits::test_zero(n))
            {
                dim = log10(n);
                if (dim > 0)
                    n /= spirit::traits::pow10<U>(traits::truncate_to_long::call(dim));
                else if (n < 1.) {
                    long exp = traits::truncate_to_long::call(-dim);

                    dim = static_cast<U>(-exp);

                    // detect and handle denormalized numbers to prevent overflow in pow10
                    if (exp > std::numeric_limits<U>::max_exponent10)
                    {
                        n *= spirit::traits::pow10<U>(std::numeric_limits<U>::max_exponent10);
                        n *= spirit::traits::pow10<U>(exp - std::numeric_limits<U>::max_exponent10);
                    }
                    else
                        n *= spirit::traits::pow10<U>(exp);

                    if (n < 1.)
                    {
                        n *= 10.;
                        --dim;
                        precexp_offset = true;
                    }
                }
            }

        // prepare numbers (sign, integer and fraction part)
            U integer_part;
            U precexp = spirit::traits::pow10<U>(precision);
            U fractional_part = modf(n, &integer_part);

            if (precexp_offset)
            {
                fractional_part =
                    floor((fractional_part * precexp + U(0.5)) * U(10.)) / U(10.);
            }
            else
            {
                fractional_part = floor(fractional_part * precexp + U(0.5));
            }

            if (fractional_part >= precexp)
            {
                fractional_part = floor(fractional_part - precexp);
                integer_part += 1;    // handle rounding overflow
                if (integer_part >= 10.)
                {
                    integer_part /= 10.;
                    ++dim;
                }
            }

        // if trailing zeros are to be omitted, normalize the precision and``
        // fractional part
            U long_int_part = floor(integer_part);
            U long_frac_part = fractional_part;
            unsigned prec = precision;
            if (!p.trailing_zeros(n))
            {
                U frac_part_floor = long_frac_part;
                if (0 != long_frac_part) {
                    // remove the trailing zeros
                    while (0 != prec && 
                           0 == traits::remainder<10>::call(long_frac_part)) 
                    {
                        long_frac_part = traits::divide<10>::call(long_frac_part);
                        --prec;
                    }
                }
                else {
                    // if the fractional part is zero, we don't need to output 
                    // any additional digits
                    prec = 0;
                }

                if (precision != prec)
                {
                    long_frac_part = frac_part_floor / 
                        spirit::traits::pow10<U>(precision-prec);
                }
            }

        // call the actual generating functions to output the different parts
            if ((force_sign || sign_val) &&
                traits::test_zero(long_int_part) &&
                traits::test_zero(long_frac_part))
            {
                sign_val = false;     // result is zero, no sign please
                force_sign = false;
            }

        // generate integer part
            bool r = p.integer_part(sink, long_int_part, sign_val, force_sign);

        // generate decimal point
            r = r && p.dot(sink, long_frac_part, precision);

        // generate fractional part with the desired precision
            r = r && p.fraction_part(sink, long_frac_part, prec, precision);

            if (r && 0 == (Policies::fmtflags::fixed & flags)) {
                return p.template exponent<CharEncoding, Tag>(sink, 
                    traits::truncate_to_long::call(dim));
            }
            return r;
        }

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif 

    };
}}}

#endif

