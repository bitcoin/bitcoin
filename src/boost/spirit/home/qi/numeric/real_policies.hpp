/*=============================================================================
    Copyright (c) 2001-2019 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_QI_NUMERIC_REAL_POLICIES_HPP
#define BOOST_SPIRIT_QI_NUMERIC_REAL_POLICIES_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/numeric/numeric_utils.hpp>
#include <boost/spirit/home/qi/detail/string_parse.hpp>
#include <boost/type_traits/is_floating_point.hpp>

namespace boost { namespace spirit { namespace traits
{
    // So that we won't exceed the capacity of the underlying type T,
    // we limit the number of digits parsed to its max_digits10.
    // By default, the value is -1 which tells spirit to parse an
    // unbounded number of digits.

    template <typename T, typename Enable = void>
    struct max_digits10
    {
        static int const value = -1;  // unbounded
    };

    template <typename T>
    struct max_digits10<T
      , typename enable_if_c<(is_floating_point<T>::value)>::type>
    {
        static int const digits = std::numeric_limits<T>::digits;
        static int const value = 2 + (digits * 30103l) / 100000l;
    };
}}}

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    //  Default (unsigned) real number policies
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct ureal_policies
    {
        // Versioning
        typedef mpl::int_<2> version;

        // trailing dot policy suggested by Gustavo Guerra
        static bool const allow_leading_dot = true;
        static bool const allow_trailing_dot = true;
        static bool const expect_dot = false;

        template <typename Iterator>
        static bool
        parse_sign(Iterator& /*first*/, Iterator const& /*last*/)
        {
            return false;
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse_n(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            typedef extract_uint<Attribute, 10, 1
            , traits::max_digits10<T>::value // See notes on max_digits10 above
            , false, true>
            extract_uint;
            return extract_uint::call(first, last, attr_);
        }

        // ignore_excess_digits (required for version > 1 API)
        template <typename Iterator>
        static std::size_t
        ignore_excess_digits(Iterator& first, Iterator const& last)
        {
            Iterator save = first;
            if (extract_uint<unused_type, 10, 1, -1>::call(first, last, unused))
                return static_cast<std::size_t>(std::distance(save, first));
            return 0;
        }

        template <typename Iterator>
        static bool
        parse_dot(Iterator& first, Iterator const& last)
        {
            if (first == last || *first != '.')
                return false;
            ++first;
            return true;
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse_frac_n(Iterator& first, Iterator const& last, Attribute& attr_, int& frac_digits)
        {
            Iterator savef = first;
            bool r = extract_uint<Attribute, 10, 1, -1, true, true>::call(first, last, attr_);
            if (r)
            {
#if defined(_MSC_VER) && _MSC_VER < 1900
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif
                // Optimization note: don't compute frac_digits if T is
                // an unused_type. This should be optimized away by the compiler.
                if (!is_same<T, unused_type>::value)
                    frac_digits =
                        static_cast<int>(std::distance(savef, first));
#if defined(_MSC_VER) && _MSC_VER < 1900
# pragma warning(pop)
#endif
                // ignore extra (non-significant digits)
                extract_uint<unused_type, 10, 1, -1>::call(first, last, unused);
            }
            return r;
        }

        template <typename Iterator>
        static bool
        parse_exp(Iterator& first, Iterator const& last)
        {
            if (first == last || (*first != 'e' && *first != 'E'))
                return false;
            ++first;
            return true;
        }

        template <typename Iterator>
        static bool
        parse_exp_n(Iterator& first, Iterator const& last, int& attr_)
        {
            return extract_int<int, 10, 1, -1>::call(first, last, attr_);
        }

        ///////////////////////////////////////////////////////////////////////
        //  The parse_nan() and parse_inf() functions get called whenever
        //  a number to parse does not start with a digit (after having
        //  successfully parsed an optional sign).
        //
        //  The functions should return true if a Nan or Inf has been found. In
        //  this case the attr should be set to the matched value (NaN or
        //  Inf). The optional sign will be automatically applied afterwards.
        //
        //  The default implementation below recognizes representations of NaN
        //  and Inf as mandated by the C99 Standard and as proposed for
        //  inclusion into the C++0x Standard: nan, nan(...), inf and infinity
        //  (the matching is performed case-insensitively).
        ///////////////////////////////////////////////////////////////////////
        template <typename Iterator, typename Attribute>
        static bool
        parse_nan(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            if (first == last)
                return false;   // end of input reached

            if (*first != 'n' && *first != 'N')
                return false;   // not "nan"

            // nan[(...)] ?
            if (detail::string_parse("nan", "NAN", first, last, unused))
            {
                if (first != last && *first == '(')
                {
                    // skip trailing (...) part
                    Iterator i = first;

                    while (++i != last && *i != ')')
                        ;
                    if (i == last)
                        return false;     // no trailing ')' found, give up

                    first = ++i;
                }
                attr_ = std::numeric_limits<T>::quiet_NaN();
                return true;
            }
            return false;
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse_inf(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            if (first == last)
                return false;   // end of input reached

            if (*first != 'i' && *first != 'I')
                return false;   // not "inf"

            // inf or infinity ?
            if (detail::string_parse("inf", "INF", first, last, unused))
            {
                // skip allowed 'inity' part of infinity
                detail::string_parse("inity", "INITY", first, last, unused);
                attr_ = std::numeric_limits<T>::infinity();
                return true;
            }
            return false;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Default (signed) real number policies
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct real_policies : ureal_policies<T>
    {
        template <typename Iterator>
        static bool
        parse_sign(Iterator& first, Iterator const& last)
        {
            return extract_sign(first, last);
        }
    };

    template <typename T>
    struct strict_ureal_policies : ureal_policies<T>
    {
        static bool const expect_dot = true;
    };

    template <typename T>
    struct strict_real_policies : real_policies<T>
    {
        static bool const expect_dot = true;
    };
}}}

#endif
