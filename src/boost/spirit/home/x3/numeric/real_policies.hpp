/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_REAL_POLICIES_APRIL_17_2006_1158PM)
#define BOOST_SPIRIT_X3_REAL_POLICIES_APRIL_17_2006_1158PM

#include <boost/spirit/home/x3/string/detail/string_parse.hpp>
#include <boost/spirit/home/x3/support/numeric_utils/extract_int.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    //  Default (unsigned) real number policies
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct ureal_policies
    {
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
            return extract_uint<T, 10, 1, -1>::call(first, last, attr_);
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
        parse_frac_n(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            return extract_uint<T, 10, 1, -1, true>::call(first, last, attr_);
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
